# 编译期反射实践

自古以来，C++就一直缺少一个编程语言的重要特性——反射，但如果熟悉C++元模板编程的同学，就知道以C++的风格，肯定是不会在标准库中添加运行时的反射支持的，从最新的C++版本演进来看，倒是编译期反射可能得到更好的支持。C++11
-> C++14 -> C++17 -> C++20... 不断让元模板编程变得更简单，更规范。

本次的编译期反射实践，代码要求的最低C++版本为14，因为用到了 make_shared、decay_t。

本次实践的完整代码仓库：[MyUtil/tree/master/aop](https://github.com/ACking-you/MyUtil/tree/master/aop)

## 获取类的方法

### 判断类是否具有某方法

我们如何判断某个类是否具有某个方法呢？

要想在编译期间实现这样一个判断，我们的思路可以是这样：写两个模板，如果这个类型具有这个方法，就匹配到返回 `std::true_type()`
的模板，如果不具备则匹配到返回 `std::false_type()` 的模板，最后通过 `std::is_same` 能够判断匹配结果，也就是实现了在编译期间判断类是否有这个方法。

上述过程，利用 [SFINAE](https://en.cppreference.com/w/cpp/language/sfinae) 的原理可以轻松实现，如果不了解 SFINAE 以及对应的
enable_if 的运用，可以看看这篇文章：[C++模板进阶指南：SFINAE](https://zhuanlan.zhihu.com/p/21314708)。

我们现在就开始动手实现上述代码，假设我们需要判断一个类型是否有 `before()` 方法。

```cpp
template <typename T, typename... Args> 
struct has_member_before {         
  private:                                                                     
    template <typename U>                                                      
    static auto Check(int) 
      -> decltype(
        std::declval<U>().before(std::declval<Args>()...),std::true_type() //1
    );                                         
    template <typename U> static std::false_type Check(...); //2                   
                                                                               
  public:                                                                      
    enum {                                                                     
      value = std::is_same<decltype(Check<T>(0)), std::true_type>::value //3      
    };                                                                         
};
```

先讲下上述代码定义后如何使用吧，比如现在有个 Student 类型，我们来判断是否具有 before 成员函数，则只需要写下下面的代码：

```cpp
has_member_before<Student,int>::value //判断Student类是否有Student::before(int)方法
```

上面的代码重点有三段，已经作为标记1、2、3：

代码1处，利用了 `std::declval` 在编译期创建类型U的实例，并调用其 `before` 方法，这是在元模板中判断一个类型是否具有某个方法的常有手段，因为
SFINAE 的存在，即便该处替换出错，编译器会去继续寻找下一个替换是否能够正确，直到所有的替换都出错。

很明显这里是一定会替换成功的，因为代码2有一个包容性很强的重载，这个重载的参数不能和代码1处的重载参数一致，否则会算作重复定义，当然如果你使用 `std::enable_if`
对参数一致的模板参数进行唯一性的限制，那么重复定义的错误也可以避免。但是写成 C 的可变参数是最快的解决方式。

代码1处，有个逗号表达式的细节，如果成功被代码1处替换，那么返回值类型将会是 `decltype()`
中的表达式类型，也就是逗号表达式最后的结果 `std::true_type`。

代码3是利用enum类型在编译期得到具体的常量值。具体是通过调用  `Check<T>(0)`
获取该函数的返回值类型，这期间模板的匹配替换就会牵扯到前面的代码1、2。所以一旦模板被实例化，那么该class是否具有该方法的信息也就清楚了。

最后我们可以把该段代码提取为宏作为通用代码：

```cpp
#define HAS_MEMBER(member)                                                     \
  template <typename T, typename... Args> struct has_member_##member {         \
  private:                                                                     \
    template <typename U>                                                      \
    static auto Check(int)                                                     \
        -> decltype(std::declval<U>().member(std::declval<Args>()...),         \
                    std::true_type());                                         \
    template <typename U> static std::false_type Check(...);                   \
                                                                               \
  public:                                                                      \
    enum {                                                                     \
      value = std::is_same<decltype(Check<T>(0)), std::true_type>::value       \
    };                                                                         \
  };
```

如果想要生成判断是否有before或者其他方法的代码，则只需要调用这个宏。

```
HAS_MEMBER(before) //生成判断是否有before的代码
HAS_MEMBER(after) //生成判断是否有after的代码
```

### 将类方法转为function保存

直接上代码，再逐一讲解：

以下代码是将该类的before和after方法包装成一个function，并返回一个pair。完整代码：[reflect_util.hpp](https://github.com/ACking-you/MyUtil/blob/master/aop/reflect_util.hpp)

```cpp
template <typename T, typename... Args>
typename std::enable_if< //1
    has_member_before<T, Args...>::value && has_member_after<T, Args...>::value,
    std::pair<std::function<void(Args...)>, std::function<void(Args...)>>>::type
GetMemberFunc() {
  auto fun = std::make_shared<std::decay_t<T>>(); //2
  return std::make_pair( //3
      [self = fun](Args &&...args) {
        self->before(std::forward<Args>(args)...);
      },
      [self = fun](Args &&...args) {
        self->after(std::forward<Args>(args)...);
      });
}
```

在代码段1中，通过 `enable_if`
确保在该类型有before和after方法，同时也可以保证写其他版本的时候不会出现重复定义的错误。`enable_if`
第一个参数是需要满足的条件，第二个参数是enable_if内部的type类型，默认为void。

代码段2中，创建一个T类型的实例，并用shread_ptr管理，原因在于before方法和after方法需要共用内存，而这两个方法都要被提取为单独的function，要保证内存安全，故需要使用智能指针。其中 `std::decay_t<T>`
效果等同于 `std::decay<T>::type`，作用是消除T类型的const修饰和引用修饰。因为make_shared<>中的模板参数不能为引用类型。

代码段3中，利用lamda表达式将fun拷贝一份到其中命名为self，最后返回pair即可。

当前写的功能是不完整的，需要多几个模板的重载来实现只有before方法、以及只有after方法的情况。写法和上述代码一致，只不过 `enable_if`
中的条件稍作改变即可。前面也提到过enable_if千万不能丢，否则会报重复定义的错误，当然如果你是C++17的版本，可以直接使用 `if constexpr`
来实现更为简洁的代码而无需单独写三个函数。

如下：

```cpp
#define ST_ASSERT                                                              \
  static_assert(                                                               \
      has_member_before<T, Args...>::value ||                                  \
          has_member_after<T, Args...>::value,                                 \
      "class need T::before(args...) or T::after(args...) member function!");

template <typename T, typename... Args>
std::pair<std::function<void(Args...)>, std::function<void(Args...)>>
GetMemberFunc() {
  ST_ASSERT // 确保至少before after有其一
      auto fun = std::make_shared<std::decay_t<T>>();
  if constexpr (has_member_before<T, Args...>::value &&
                has_member_after<T, Args...>::value) { // 有before和after
    return std::make_pair(
        [self = fun](Args &&...args) {
          self->before(std::forward<Args>(args)...);
        },
        [self = fun](Args &&...args) {
          self->after(std::forward<Args>(args)...);
        });
  } else if constexpr (has_member_before<T, Args...>::value &&
                       !has_member_after<T, Args...>::value) { // 有before
    return std::make_pair(
        [self = fun](Args &&...args) {
          self->before(std::forward<Args>(args)...);
        },
        nullptr);
  } else { // 只有after
    return std::make_pair(nullptr, [self = fun](Args &&...args) {
      self->after(std::forward<Args>(args)...);
    });
  }
}
```

下面我简单解释下代码：

1. ST_ASSERT宏的作用是，通过static_assert在编译期抛出提示，T类型必须有before或after两个方法之一。
2. 通过该类型拥有的情况不同，给出不同的返回值。

很明显去除了enable_if后，我们代码清爽了许多。

## AOP的实现

关于AOP，大家可以去搜一搜，这里就不过多赘述。我的简单理解就是一个事件回调，可以嵌入到业务的执行前后，把这个事件的概念换成一个切面，把业务代码看作一个横向坐标轴上的面，那么AOP就是在这个面的前后添加其他切面来实现常用的业务复用。比如用户的身份验证，可以在业务之前添加身份验证的切面，比如需要测试该业务的性能，那么可以在业务切面的前后添加开始计时和终止计时的逻辑。

### Invoke调用实现AOP

根据上述对AOP的描述，我们要切入的代码主要是前和后两个逻辑，故每个要切入的类可以规定他必须定义Before或者After方法。然后通过可变参模板递归实现任意个参数的切面调用。

可以把整个切面调用过程看作一个洋葱圈层，比如添加s1类型的before和after作为切片，s2类型的before和after作为切片，s3类型的before作为切片。把业务代码逻辑作为foo函数。

则他们的调用过程如下：

s1->before => s2->before => s3->before => foo业务逻辑 => s1->after => s2->after。

如果稍微学过点数据结果，这个调用就能想到前中后序遍历上去了。

代码实现如下（C++11需要使用eable_if来实现，代码量很多，所以这里就直接用C++17的 `if constexpr` 来实现了）：

```cpp
 /*以下是截取的一个类的两个方法*/

// 递归的尽头
  template <typename T> void Invoke(Args &&...args, T &&aspect) {
    ST_ASSERT
    if constexpr (has_member_Before<T, Args...>::value &&
                  has_member_After<T, Args...>::value) {
      aspect.Before(std::forward<Args>(args)...); // 核心逻辑之前的切面逻辑
      m_func(std::forward<Args>(args)...);        // 核心逻辑
      aspect.After(std::forward<Args>(args)...); // 核心逻辑之后的切面逻辑
    } else if constexpr (has_member_Before<T, Args...>::value &&
                         !has_member_After<T, Args...>::value) {
      aspect.Before(std::forward<Args>(args)...); // 核心逻辑之前的切面逻辑
      m_func(std::forward<Args>(args)...);        // 核心逻辑
    } else {
      m_func(std::forward<Args>(args)...);       // 核心逻辑
      aspect.After(std::forward<Args>(args)...); // 核心逻辑之后的切面逻辑
    }
  }

  // 变参模板递归
  template <typename T, typename... Tail>
  void Invoke(Args &&...args, T &&headAspect, Tail &&...tailAspect) {
    ST_ASSERT
    if constexpr (has_member_Before<T, Args...>::value &&
                  has_member_After<T, Args...>::value) {
      headAspect.Before(std::forward<Args>(args)...);
      Invoke(std::forward<Args>(args)..., std::forward<Tail>(tailAspect)...);
      headAspect.After(std::forward<Args>(args)...);
    } else if constexpr (has_member_Before<T, Args...>::value &&
                         !has_member_After<T, Args...>::value) {
      headAspect.Before(std::forward<Args>(args)...);
      Invoke(std::forward<Args>(args)..., std::forward<Tail>(tailAspect)...);
    } else {
      Invoke(std::forward<Args>(args)..., std::forward<Tail>(tailAspect)...);
      headAspect.After(std::forward<Args>(args)...); // 核心逻辑之后的切面逻辑
    }
  }
```

上述完整代码：[aspect.hpp](https://github.com/ACking-you/MyUtil/blob/master/aop/aspect.hpp)

上述代码是根据C++变参模板实现的通用性操作，可以同时添加多个切片
，他们都是Aspect类的两个方法，具体实现逻辑就是：通过之前得到的编译期常量( `has_member_Before<T,Args...>::value` )判断 T
是否具有Before或者After方法，分三种情况：

1. 同时又Before和After：利用中序进行递归。

2. 只有Before：利用前序进行递归。

3. 只有After：利用后序进行递归。

> 为了简化调用过程，继续封装如下：

最后记得定义一个终止模板递归的最终形态。

```cpp
template <typename T> using identity_t = T;

// AOP的辅助函数，简化调用
template <typename... AP, typename... Args, typename Func>
void Invoke(Func &&f, Args &&...args) {
  Aspect<Func, Args...> asp(std::forward<Func>(f));
  asp.Invoke(std::forward<Args>(args)..., identity_t<AP>()...);
}
```

最终如果像最开始讲的要拓展s1、s2、s3的方法上去，那么简单的使用如下代码即可：

```cpp
Invoke<s1,s2,s3>(&foo,args); //s1,s2,s3为拓展逻辑，foo为业务逻辑
```

### 统一转function存储并实现AOP调用顺序

#### 统一转function存储

将任意类的before和after方法集体装箱为function的关键代码逻辑如下，完整代码请看：

```cpp
  void Get() {} //空的func，用于结束模板的递归实例化

  template <typename T, typename... Tail> void Get(T &&head, Tail &&...tails) {
    ST_ASSERT
    auto &&p = details::GetMemberFunc<T, Args...>();
    m_output.push_back(p);
    Get(tails...);
  }
```

由于所有的获取before和after的逻辑在前面**获取类的方法**已经讲到，所以单个类型直接调用 `GetMemberFunc`
函数即可得出结果，并放入vector中，最后通过模板实例化的递归将所有的类型都装箱。

具体的使用方式也很简单，如下代码：

```cpp
#include"reflect_util.hpp"
using func_t = reflect::MemberFunc<int>::func_t;
using func_pair_t = reflect::MemberFunc<int>::func_pair_t;

struct LoginAspect {
  void before(int i) { cout << "Login start " << i << endl; }
  void after(int i) { cout << "after start " << i << endl; }
};
int main(){
   vector<func_pair_t> out;
  // 获取before和after方法，并通过function进行包装
  reflect::MemberFunc<int>(out).Get(
      TimeElapsedAspect{}, 
      LoggingAspect{},
      LoginAspect{}
  );
  //将三个类型的before和after方法分离成function后以pair的形式存储在out中
  for(auto&& p : out){
      if(p.first){//如果before存在则调用
          p.first(0);
      }
      if(p.second){//如果after存在则调用
          p.second(1);
      }
  }
}
```

#### AOP的调用顺序实现

```cpp
// 根据AOP的顺序存入out数组
void AspectOrder(vector<func_t> &out, vector<func_pair_t> &v,
                    const func_t &func, int index) {
  if (v.size() <= index) {
    out.push_back(func);
    return;
  }
  if (v[index].first) {
    out.push_back(v[index].first);
  }
  AspectOrder(out, v, func, index + 1);
  if (v[index].second) {
    out.push_back(v[index].second);
  }
}
```

完整测试代码：[test_aspect.cc](https://github.com/ACking-you/MyUtil/blob/master/aop/test_aspect.cc)

