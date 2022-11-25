#include "Logger.h"
#include "any-lite.hpp"
using namespace lblog;
using namespace nonstd;

struct Student {
  Student() = default;
  Student(int id, std::string name) : id_(id), name_(std::move(name)) {}
  int id_{};
  std::string name_;
};

void test_any() {
  any t = "你好";
  auto test_construct = [&]() {
    info("{}", t.type().name());
    t = 345456;
    info("{}", t.type().name());
    t = Student();
    info("{}", t.type().name());
    t = any(Student());
    info("{}", t.type().name());
    t = std::move(any("hello我k$"));
  };

  auto test_cast = [&]() {
    info("{}", any_cast<const char *>(t));
    t = std::to_string(43324324);
    info("typename:{} value:{}", t.type().name(), any_cast<std::string>(t));
  };

  auto test_other = [&]() {
    t.emplace<Student>(323, std::string("fdfsafsda"));
    info("id:{} name:{}", any_cast<Student>(t).id_, any_cast<Student>(t).name_);
    t.reset();
    info("{}", t.type().name());
    any p = "再次测试看看";
    swap(t, p);
    info("p:{},t:{},t_value{}", p.type().name(), t.type().name(),
         any_cast<const char *>(t));
    t = std::vector<std::string>{"fdsfasddfas", "fsaffafa", "fdsaadsf"};
    info("type:{} t: {}", t.type().name(),
         any_cast<std::vector<std::string>>(t));
    t = "你好"; // const修饰的常量转变量出错
    info("{}", any_cast<char *>(t));
  };
  test_construct();
  test_cast();
  test_other();
}