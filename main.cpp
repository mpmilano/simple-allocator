#include <array>
#include <cassert>
#include <functional>
#include <list>
#include <malloc.h>
#include <map>
#include <memory>
#include <type_traits>

struct allocator {

  struct _i {

    char *serial_region{0};
    std::size_t serial_offset{0};
    const std::size_t serial_size;
    std::map<void *, std::function<void()>> allocated_serializers;

    _i(void *serial_region, std::size_t serial_size)
        : serial_region((char *)serial_region), serial_size(serial_size) {}

    void *alloc_serial(std::size_t chunk_size) {
      void *ret = (((char *)serial_region) + serial_offset);
      serial_offset += chunk_size;
      assert(serial_offset < serial_size);
      return ret;
    }

    template <typename T> void free(T *t) {
      // ensure not in serial region (n.b. will change to noop once debugged)
      assert(serial_region > ((char *)t) ||
             (serial_region + serial_size) < ((char *)t));
      allocated_serializers.erase(t);
      ::free(t);
    }

    template <typename T> T *dyn_alloc() {
      auto *ret = malloc(sizeof(T));
      allocated_serializers[ret] = [] {};
      return (T *)ret;
    }

    template <typename T> T *_alloc() {
      if constexpr (std::is_trivially_copyable_v<T>) {
        return alloc_serial(sizeof(T));
      } else {
        return dyn_alloc<T>();
      }
    }

    template <typename T> void del(T *t) {
      t->~T();
      free(t);
    }

    template <typename T, typename... U> T *alloc_and_init(U &&... u) {
      auto *ret = _alloc<T>();
      new (ret) T{std::forward<U>(u)...};
      return ret;
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
  allocator(void *serial_region, std::size_t serial_size)
      : i(new _i{serial_region, serial_size}) {}
};

constexpr const std::size_t size = 1024;
int main() {
  std::array<char, size> one;
  allocator a{one.data(), sizeof(one)};
  auto *i = a.alloc_and_init<std::list<int>>(0);
  a.del(i);
}