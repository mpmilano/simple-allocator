#include "arg-version2-allocator.hpp"
#include <array>
#include <list>
#include <string>

namespace derecho_allocator {

template <typename... Args> class message_builder {
  using allocator = internal::build_allocator<Args...>;
  allocator a;

public:
  message_builder(unsigned char *serial_region, std::size_t size)
      : a(serial_region, size) {}
  template <std::size_t s, typename... CArgs>
  decltype(auto) build_arg(CArgs &&... cargs) {
    return a.template build_arg<s, CArgs...>(std::forward<CArgs>(cargs)...);
  }

  char *serialize() { return a.serialize(); }
};

} // namespace derecho_allocator

using namespace derecho_allocator;

int main() {
  std::array<unsigned char, 1024> mem;
  message_builder<int, char, std::string, std::list<char>> mb(mem.data(),
                                                              sizeof(mem));
  int *i = mb.build_arg<0>(15);
  char *c = mb.build_arg<1>('e');
  std::string *s = mb.build_arg<2>("str");
  std::list<char> *l = mb.build_arg<3>();
  (void)i;
  (void)c;
  (void)s;
  (void)l;
}
