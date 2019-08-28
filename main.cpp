#include "indexed_varargs.hpp"
#include "mutils-serialization/SerializationSupport.hpp"
#include <array>
#include <cassert>
#include <functional>
#include <list>
#include <malloc.h>
#include <map>
#include <memory>
#include <type_traits>

template <typename... Reserved> struct argument_allocator {
  static_assert((std::is_trivially_copyable_v<Reserved> && ... && true),
                "Error: Can only reserve POD types");
  // Design idea:  reserve a portion of the serialized space to represent the
  // offsets in which the arguments are being sent.   [12,36,42,2; objects...]
  // Use a ushort for each number in the argument order. Fill this in at
  // serialization time, on deserialization follow order as specified.

  // Actual idea we're using in this draft: reserve the arguments explicitly, in
  // the order you desire, via template args.
  // Fill them in / allocate them however you'd like.

  struct _i {
    short reserved_offsets[sizeof...(Reserved)] = {(sizeof(Reserved) * 0)...};
    bool reserved_used[sizeof...(Reserved)] = {(sizeof(Reserved) * 0)...};
    char *serial_region{0};
    std::size_t serial_offset{(sizeof(Reserved) + ... + 0)};
    const std::size_t serial_size;

    _i(void *serial_region, std::size_t serial_size)
        : serial_region((char *)serial_region), serial_size(serial_size) {
      assert(serial_size >= serial_offset);
      const constexpr short reserved_sizes[sizeof...(Reserved)] = {
          sizeof(Reserved)...};
      for (uint i = 1; i < sizeof...(Reserved); ++i) {
        reserved_offsets[i] = reserved_offsets[i - 1] + reserved_sizes[i - 1];
      }
    }

    void *alloc_serial(std::size_t arg_indx) {
      void *ret = (((char *)serial_region) + reserved_offsets[arg_indx]);
      reserved_used[arg_indx] = true;
      return ret;
    }

    template <typename T> void free(T *t) {
      // ensure not in serial region (n.b. will change to noop once debugged)
      assert(serial_region > ((char *)t) ||
             (serial_region + serial_size) < ((char *)t));
      ::free(t);
    }

    template <typename T> T *dyn_alloc() {
      auto *ret = (T *)malloc(sizeof(T));
      return ret;
    }

    template <typename T>
    T *_alloc(std::size_t arg_index = sizeof...(Reserved)) {
      if (std::is_trivially_copyable_v<T> && arg_index < sizeof...(Reserved) &&
          !reserved_used[arg_index]) {
        assert((type_has_index<T, Reserved...>(arg_index)));
        return (T *)alloc_serial(arg_index);
      } else {
        return dyn_alloc<T>();
      }
    }

    template <typename T> void del(T *t) {
      t->~T();
      free(t);
    }

    template <typename T, typename... U> T *alloc_and_init(U &&... u) {
      return alloc_and_init_reserved<T>(sizeof...(Reserved),
                                        std::forward<U>(u)...);
    }

    template <typename T, typename... U>
    T *alloc_and_init_reserved(std::size_t s, U &&... u) {
      auto *ret = _alloc<T>(s);
      new (ret) T{std::forward<U>(u)...};
      return ret;
    }

    template <typename... Dynamics>
    void *prep_send(const Reserved &..., const Dynamics &... d) {
      for (auto b : reserved_used)
        assert(b);
      for (auto f : {[&] {
             serial_offset += to_bytes(d, serial_offset + serial_region);
           }...})
        f();
    }
  };

  template <typename T, typename... U>
  decltype(auto) alloc_and_init(U &&... u) {
    return i->template alloc_and_init<T>(std::forward<U>(u)...);
  }

  template <typename... T> decltype(auto) del(T &&... t) {
    return i->template del(std::forward<T>(t)...);
  }

  std::shared_ptr<struct _i> i;
  argument_allocator(void *serial_region, std::size_t serial_size)
      : i(new _i{serial_region, serial_size}) {}
};

constexpr const std::size_t size = 1024;
int main() {
  std::array<char, size> one;
  argument_allocator<> a{one.data(), sizeof(one)};
  auto *i = a.alloc_and_init<std::list<int>>(0);
  a.del(i);
}