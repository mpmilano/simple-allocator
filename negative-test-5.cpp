#include "message-builder.hpp"
#include "mutils-serialization/SerializationSupport.hpp"
#include <array>
#include <list>
#include <string>

using namespace derecho_allocator;

void test1() {
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

void test2() {
  std::array<unsigned char, 1024> mem;
  message_builder<int, char> mb(mem.data(), sizeof(mem));
  arg_ptr<int> i = mb.build_arg<0>(15);
  arg_ptr<char> c = mb.build_arg<1>('e');
  auto buf = mb.serialize(i, c);
  mutils::deserialize_and_run(nullptr, buf, [](const int &i, const char &c) {
    assert(i == 15);
    assert(c == 'e');
  });
}

void test3() {
  std::array<unsigned char, 1024> mem;
  message_builder<std::string, std::list<char>> mb(mem.data(), sizeof(mem));
  arg_ptr<std::string> s = mb.build_arg<0>("str");
  arg_ptr<std::list<char>> l = mb.build_arg<1>();
  auto buf = mb.serialize(s, l);
  mutils::deserialize_and_run(
      nullptr, buf, [](const std::string &s, const std::list<char> &l) {
        assert(s == "str");
        assert(l == std::list<char>{});
      });
}

void test4() {
  std::array<unsigned char, 1024> mem;
  message_builder<int, char, std::string, std::list<char>> mb(mem.data(),
                                                              sizeof(mem));
  arg_ptr<int> i = mb.build_arg<0>(15);
  arg_ptr<char> c = mb.build_arg<1>('e');
  arg_ptr<std::string> s = mb.build_arg<2>("str");
  std::list<char> reference_l;
  arg_ptr<std::list<char>> l = mb.build_arg<3>();
  for (char i = 0; i < 'Z'; ++i) {
    l->push_back(i);
    reference_l.push_back(i);
  }
  auto buf = mb.serialize(i, c, s, l);
  mutils::deserialize_and_run(nullptr, buf,
                              [reference_l](const int &i, const char &c,
                                            const std::string &s,
                                            const std::list<char> &l) {
                                assert(i == 15);
                                assert(c == 'e');
                                assert(s == "str");
                                assert(l == reference_l);
                              });
}

void test5() {

  struct beguile {
    std::array<char, 43> data1;
    int data2;
    double data3;
  };

  std::array<unsigned char, 1024> mem;
  message_builder<int, beguile, char, std::string, std::list<char>> mb(
      mem.data(), sizeof(mem));
  arg_ptr<int> i = mb.build_arg<0>(15);
  arg_ptr<beguile> bg = mb.build_arg<1>();
  arg_ptr<char> c = mb.build_arg<2>('e');
  arg_ptr<std::string> s = mb.build_arg<3>("str");
  std::list<char> reference_l;
  arg_ptr<std::list<char>> l = mb.build_arg<4>();
  for (char i = 0; i < 'Z'; ++i) {
    l->push_back(i);
    reference_l.push_back(i);
  }
  auto buf = mb.serialize(i, c, s, l);
  mutils::deserialize_and_run(nullptr, buf,
                              [reference_l](const int &i, const char &c,
                                            const std::string &s,
                                            const std::list<char> &l) {
                                assert(i == 15);
                                assert(c == 'e');
                                assert(s == "str");
                                assert(l == reference_l);
                              });
}

int main() {
  test1();
  test2();
  test3();
  test4();
}
