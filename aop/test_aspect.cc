#include "aspect.hpp"
#include "reflect_util.hpp"
#include <chrono>
#include <iostream>
#include <string_view>
#include <thread>
using namespace std;
using namespace std::chrono_literals;
using func_t = reflect::MemberFunc<int>::func_t;
using func_pair_t = reflect::MemberFunc<int>::func_pair_t;

struct TimeElapsedAspect {
  void before(int i) {
    cout << "time start...."
         << " num is " << i << endl;
    m_lastTime = time(nullptr);
  }

  void after(int i) const {
    this_thread::sleep_for(2s);
    cout << "time elapsed: " << time(nullptr) - m_lastTime << " num is " << i
         << endl;
  }
  void Before(int i) { before(i); }
  void After(int i) { after(i); }

private:
  double m_lastTime;
};

struct LoginAuth {
  void before() {}
};

struct LoggingAspect {
  void before(int i) {
    t = " before call ";
    std::cout << "entering" << t << i << std::endl;
  }
  void after(int i) {
    t = " after call ";
    std::cout << "leaving" << t << i << std::endl;
  }
  void Before(int i) { before(i); }
  void After(int i) { after(i); }
  std::string t;
};

struct LoginAspect {
  static void before(int i) { cout << "Login start " << i << endl; }
  static void after(int i) { cout << "after start " << i << endl; }
};

struct EmptyAspect {};

// 业务代码函数
void foo(int i) { cout << "start event " << i << endl; }

// 根据AOP的顺序存入out数组
void AspectOrderGet(vector<func_t> &out, vector<func_pair_t> &v,
                    const func_t &func, int index) {
  if (v.size() <= index) {
    out.push_back(func);
    return;
  }
  if (v[index].first) {
    out.push_back(v[index].first);
  }
  AspectOrderGet(out, v, func, index + 1);
  if (v[index].second) {
    out.push_back(v[index].second);
  }
}
/* -----------------------------------------------test---------------------------------------------------*/
void TestMemberFunc() {
  vector<func_pair_t> t;
  // 获取before和after方法，并通过function进行包装
  reflect::MemberFunc<int>(t).Get(TimeElapsedAspect{}, LoggingAspect{},
                                  LoginAspect{});
  // 通过中序遍历遍历整个切面得到切面的函数调用顺序
  vector<func_t> funcs;
  AspectOrderGet(funcs, t, foo, 0);
  for (int i = 0; i < funcs.size(); i++)
    funcs[i](i);
}

void TestInvoke() {
  Invoke<LoggingAspect, TimeElapsedAspect>(&foo, 1); // 织入方法
  cout << "-----------------------" << endl;
  Invoke<TimeElapsedAspect, LoggingAspect>(&foo, 1);
}

int main() {
  TestMemberFunc();
  cout << "-------------------------member_func_finish-----------------"
       << endl;
  TestInvoke();
  cout << "-------------------------invoke_finish-----------------" << endl;
  return 0;
}
