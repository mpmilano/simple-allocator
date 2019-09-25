#include "message-builder.hpp"
#include "mutils-serialization/SerializationSupport.hpp"
#include <array>
#include <list>
#include <string>

using namespace derecho_allocator;

int main() {
  std::array<unsigned char, 1024> mem;
  message_builder<int, char, std::string, std::list<char>> mb(mem.data(),
                                                              sizeof(mem));
  arg_ptr<int> i = mb.build_arg<0>(15);
  arg_ptr<char> c = mb.build_arg<1>('e');
  arg_ptr<std::string> s = mb.build_arg<2>("str");
  arg_ptr<std::list<char>> l = mb.build_arg<3>();
  auto buf = mb.serialize(i, c, s, l);
  mutils::deserialize_and_run(nullptr, buf,
                              [](const int &i, const char &c,
                                 const std::string &s,
                                 const std::list<char> &l) {
                                assert(i == 15);
                                assert(c == 'e');
                                assert(s == "str");
                                assert(l == std::list<char>{});
                              });
}
