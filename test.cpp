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

struct beguile {
  std::array<char, 43> data1;
  int data2;
  double data3;
};

bool operator==(const beguile &l, const beguile &r) {
  bool same = true;
  for (uint i = 0; i < 43; ++i) {
    if (l.data1[i] != r.data1[i])
      same = false;
  }
  return same && (l.data2 == r.data2) && (l.data3 == r.data3);
}

void test5() {
  std::array<unsigned char, 1024> mem;
  message_builder<int, beguile, char, std::string, std::list<char>> mb(
      mem.data(), sizeof(mem));
  arg_ptr<int> i = mb.build_arg<0>(15);
  arg_ptr<beguile> bg = mb.build_arg<1>();
  bg->data1 = {'t', 'h', 'i', 's', ' ', 'i', 's', ' ', 'a',
               ' ', 's', 't', 'r', 'i', 'n', 'g', 0};
  bg->data2 = 42;
  bg->data3 = 3.243;
  arg_ptr<char> c = mb.build_arg<2>('e');
  arg_ptr<std::string> s = mb.build_arg<3>("str");
  std::list<char> reference_l;
  arg_ptr<std::list<char>> l = mb.build_arg<4>();
  for (char i = 0; i < 'Z'; ++i) {
    l->push_back(i);
    reference_l.push_back(i);
  }
  auto buf = mb.serialize(i, bg, c, s, l);
  mutils::deserialize_and_run(
      nullptr, buf,
      [reference_l](const int &i, const beguile &bg, const char &c,
                    const std::string &s, const std::list<char> &l) {
        assert(i == 15);
        assert(bg.data2 == 42);
        assert(bg.data3 == 3.243);
        assert(std::string{bg.data1.data()} == "this is a string");
        assert(c == 'e');
        assert(s == "str");
        assert(l == reference_l);
      });
}

void test6() {
  std::array<unsigned char, 1024> mem;
  message_builder<int, beguile, char, std::string, std::list<beguile>> mb(
      mem.data(), sizeof(mem));
  arg_ptr<int> i = mb.build_arg<0>(15);
  arg_ptr<beguile> bg = mb.build_arg<1>();
  bg->data1 = {'t', 'h', 'i', 's', ' ', 'i', 's', ' ', 'a',
               ' ', 's', 't', 'r', 'i', 'n', 'g', 0};
  bg->data2 = 42;
  bg->data3 = 3.243;
  arg_ptr<char> c = mb.build_arg<2>('e');
  arg_ptr<std::string> s = mb.build_arg<3>("str");
  std::list<beguile> reference_l;
  arg_ptr<std::list<beguile>> l = mb.build_arg<4>();
  for (char i = 0; i < 'Z'; ++i) {
    beguile b;
    b.data1 = {i, i, i, i, 0};
    b.data2 = i;
    b.data3 = i + 1 / i;
    l->push_back(b);
    reference_l.push_back(b);
  }
  auto buf = mb.serialize(i, bg, c, s, l);
  mutils::deserialize_and_run(
      nullptr, buf,
      [reference_l](const int &i, const beguile &bg, const char &c,
                    const std::string &s, const std::list<beguile> &l) {
        assert(i == 15);
        assert(bg.data2 == 42);
        assert(bg.data3 == 3.243);
        assert(std::string{bg.data1.data()} == "this is a string");
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
  test5();
}
