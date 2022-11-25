#ifndef ASPECT_H
#define ASPECT_H
#include <type_traits>
#include <utility>

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
#define ST_ASSERT                                                              \
  static_assert(                                                               \
      has_member_Before<T, Args...>::value ||                                  \
          has_member_After<T, Args...>::value,                                 \
      "class need T::before(args...) or T::after(args...) member function!");

HAS_MEMBER(Before)
HAS_MEMBER(After)

template <typename Func, typename... Args> struct Aspect {
  explicit Aspect(Func &&f) : m_func(std::forward<Func>(f)) {}

#if __cplusplus >= 201703L
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
#else
  // 最后一层递归逻辑
  template <typename T>
  typename std::enable_if<has_member_Before<T, Args...>::value &&
                          has_member_After<T, Args...>::value>::type
  Invoke(Args &&...args, T &&aspect) {
    aspect.Before(std::forward<Args>(args)...); // 核心逻辑之前的切面逻辑
    m_func(std::forward<Args>(args)...);        // 核心逻辑
    aspect.After(std::forward<Args>(args)...); // 核心逻辑之后的切面逻辑
  }

  template <typename T>
  typename std::enable_if<has_member_Before<T, Args...>::value &&
                          !has_member_After<T, Args...>::value>::type
  Invoke(Args &&...args, T &&aspect) {
    aspect.Before(std::forward<Args>(args)...); // 核心逻辑之前的切面逻辑
    m_func(std::forward<Args>(args)...);        // 核心逻辑
  }

  template <typename T>
  typename std::enable_if<!has_member_Before<T, Args...>::value &&
                          has_member_After<T, Args...>::value>::type
  Invoke(Args &&...args, T &&aspect) {
    m_func(std::forward<Args>(args)...);       // 核心逻辑
    aspect.After(std::forward<Args>(args)...); // 核心逻辑之后的切面逻辑
  }

  // 外层逻辑
  template <typename Head, typename... Tail>
  typename std::enable_if<has_member_Before<Head, Args...>::value &&
                          has_member_After<Head, Args...>::value>::type
  Invoke(Args &&...args, Head &&headAspect, Tail &&...tailAspect) {
    headAspect.Before(std::forward<Args>(args)...);
    Invoke(std::forward<Args>(args)..., std::forward<Tail>(tailAspect)...);
    headAspect.After(std::forward<Args>(args)...);
  }

  template <typename Head, typename... Tail>
  typename std::enable_if<!has_member_Before<Head, Args...>::value &&
                          has_member_After<Head, Args...>::value>::type
  Invoke(Args &&...args, Head &&headAspect, Tail &&...tailAspect) {

    Invoke(std::forward<Args>(args)..., std::forward<Tail>(tailAspect)...);
    headAspect.After(std::forward<Args>(args)...);
  }

  template <typename Head, typename... Tail>
  typename std::enable_if<has_member_Before<Head, Args...>::value &&
                          !has_member_After<Head, Args...>::value>::type
  Invoke(Args &&...args, Head &&headAspect, Tail &&...tailAspect) {
    headAspect.Before(std::forward<Args>(args)...);
    Invoke(std::forward<Args>(args)..., std::forward<Tail>(tailAspect)...);
  }
#endif

private:
  Func m_func; // 被织入的函数
};
template <typename T> using identity_t = T;

// AOP的辅助函数，简化调用
template <typename... AP, typename... Args, typename Func>
void Invoke(Func &&f, Args &&...args) {
  Aspect<Func, Args...> asp(std::forward<Func>(f));
  asp.Invoke(std::forward<Args>(args)..., identity_t<AP>()...);
}
#undef HAS_MEMBER
#undef ST_ASSERT

#endif