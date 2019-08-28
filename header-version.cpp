#include "mutils-serialization/SerializationSupport.hpp"
#include <array>
#include <cassert>
#include <functional>
#include <list>
#include <malloc.h>
#include <map>
#include <memory>
#include <type_traits>

template <std::size_t header_size> struct allocator {

  struct _i {

    char *serial_region{0};
    std::size_t serial_offset{header_size};
    const std::size_t serial_size;
    std::map<void const *const, std::function<char *()>> allocated_serializers;

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
      auto *ret = (T *)malloc(sizeof(T));
      allocated_serializers[ret] = [ret, this] {
        char *obj_loc = serial_region + serial_offset;
        serial_offset += mutils::to_bytes(*ret, obj_loc);
        return obj_loc;
      };
      return ret;
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

    template <typename... Args> void *prep_send(const Args &... a) {
      static_assert(sizeof...(Args) * sizeof(unsigned short) <= header_size,
                    "Error: header too small for these args");
      std::size_t offsets[sizeof...(Args)] = {
          (std::is_trivially_copyable_v<Args> ? ((char *)&a) - serial_region
                                              : 0)...};
      void const *const dynamic_args[sizeof...(Args)] = {
          (std::is_trivially_copyable_v<Args> ? nullptr : &a)...};
      {
        std::size_t indx = 0;
        for (auto *arg : dynamic_args) {
          if (arg) {
            assert(allocated_serializers.count(arg) > 0);
            offsets[indx] = allocated_serializers.at(arg)() - serial_region;
          }
          ++indx;
        }
      }
      {
        unsigned short *header = (unsigned short *)serial_region;
        for (std::size_t indx = 0; indx < sizeof...(Args); ++indx) {
          assert(offsets[indx] <= std::numeric_limits<unsigned short>::max());
          header[indx] = (unsigned short)offsets[indx];
        }
      }
      return serial_region;
    }
  };

  template <typename... Args> void *prep_send(const Args &... d) {
    return i->prep_send(d...);
  }

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
  allocator<1024> a{one.data(), sizeof(one)};
  auto *i = a.alloc_and_init<std::list<int>>(0);
  a.prep_send(*i);
  a.del(i);
}