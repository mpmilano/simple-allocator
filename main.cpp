#include <array>
#include <cassert>
#include <list>
#include <memory>
#include <type_traits>

struct allocator {

  struct _i {

    struct chunk {
      std::size_t size;
    };

    char *serial_region{0};
    std::size_t serial_offset{0};
    const std::size_t serial_size;
    std::list<chunk *> free_list;
    std::size_t dynamic_offset{0};
    const std::size_t dynamic_size;
    char *dynamic_region;

    _i(void *serial_region, std::size_t serial_size, void *dynamic_region,
       std::size_t dynamic_size)
        : serial_region((char *)serial_region), serial_size(serial_size),
          dynamic_size(dynamic_size), dynamic_region((char *)dynamic_region) {}

    template <typename T> struct chunk_t : public chunk { T value; };

    template <typename T> constexpr std::size_t chunk_value_offset() const {
      return offsetof(chunk_t<T>, value);
    }

    template <typename T> T *alloc_serial() {
      T *ret = (T *)(((char *)serial_region) + serial_offset);
      serial_offset += sizeof(chunk_t<T>);
      assert(serial_offset < serial_size);
      return ret;
    }

    template <typename T> T *alloc_dynamic() {
      chunk_t<T> *ret =
          (chunk_t<T> *)(((char *)dynamic_region) + dynamic_offset);
      dynamic_offset += sizeof(chunk_t<T>);
      ret->size = sizeof(chunk_t<T>);
      assert(dynamic_offset < dynamic_size);
      return &ret->value;
    }

    template <typename T> T *find_dynamic_slot() {
      for (auto it = free_list.begin(); it != free_list.end(); ++it) {
        if ((*it)->size >= sizeof(chunk_t<T>)) {
          auto *p = *it;
          free_list.erase(it);
          return &((chunk_t<T> *)p)->value;
        }
      }
      return alloc_dynamic<T>();
    }

    template <typename T> void free(T *t) {
      // ensure not in serial region (n.b. will change to noop once debugged)
      assert(serial_region > ((char *)t) ||
             (serial_region + serial_size) < ((char *)t));
      // ensure actually in dynamic region.
      assert(dynamic_region < ((char *)t) &&
             (dynamic_region + dynamic_size) > ((char *)t));
      auto *chunk_val = (chunk_t<T> *)(((char *)t) - chunk_value_offset<T>());
      free_list.emplace_back(chunk_val);
    }

    template <typename T> T *_alloc() {
      if constexpr (std::is_trivially_copyable_v<T>) {
        return alloc_serial<T>();
      } else {
        return find_dynamic_slot<T>();
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
  allocator(void *serial_region, std::size_t serial_size, void *dynamic_region,
            std::size_t dynamic_size)
      : i(new _i{serial_region, serial_size, dynamic_region, dynamic_size}) {}
};

constexpr const std::size_t size = 1024;
int main() {
  std::array<char, size> one;
  std::array<char, size> two;
  allocator a{one.data(), sizeof(one), two.data(), sizeof(two)};
  auto *i = a.alloc_and_init<std::list<int>>(0);
  a.del(i);
}