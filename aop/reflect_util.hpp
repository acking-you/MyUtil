#ifndef SPLITE_FUNC_H
#define SPLITE_FUNC_H
#include <array>
#include <functional>
#include <memory>
#include <utility>
#include <vector>

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

HAS_MEMBER(before)
HAS_MEMBER(after)

#define ST_ASSERT                                                              \
  static_assert(                                                               \
      has_member_before<T, Args...>::value ||                                  \
          has_member_after<T, Args...>::value,                                 \
      "class need T::before(args...) or T::after(args...) member function!");

namespace reflect::details {
#if __cplusplus >= 201703L
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

#else
template <typename T, typename... Args>
typename std::enable_if<
    has_member_before<T, Args...>::value && has_member_after<T, Args...>::value,
    std::pair<std::function<void(Args...)>, std::function<void(Args...)>>>::type
GetMemberFunc() {
  auto fun = std::make_shared<std::decay_t<T>>();
  return std::make_pair(
      [self = fun](Args &&...args) {
        self->before(std::forward<Args>(args)...);
      },
      [self = fun](Args &&...args) {
        self->after(std::forward<Args>(args)...);
      });
}

template <typename T, typename... Args>
typename std::enable_if<
    has_member_before<T, Args...>::value &&
        !has_member_after<T, Args...>::value,
    std::pair<std::function<void(Args...)>, std::function<void(Args...)>>>::type
GetMemberFunc() {
  auto fun = std::make_shared<std::decay_t<T>>();
  return std::make_pair(
      [self = fun](Args &&...args) {
        self->before(std::forward<Args>(args)...);
      },
      nullptr);
}

template <typename T, typename... Args>
typename std::enable_if<
    !has_member_before<T, Args...>::value &&
        has_member_after<T, Args...>::value,
    std::pair<std::function<void(Args...)>, std::function<void(Args...)>>>::type
GetMemberFunc() {
  auto fun = std::make_shared<std::decay_t<T>>();
  return std::make_pair(nullptr, [self = fun](Args &&...args) {
    self->after(std::forward<Args>(args)...);
  });
}
#endif

}; // namespace reflect::details

namespace reflect {
template <typename... Args> class MemberFunc {
public:
  using func_t = std::function<void(Args...)>;
  using func_pair_t = std::pair<func_t, func_t>;
  explicit MemberFunc(std::vector<func_pair_t> &output) : m_output(output) {}

  void Get() {}

  template <typename T, typename... Tail> void Get(T &&head, Tail &&...tails) {
    ST_ASSERT
    auto &&p = details::GetMemberFunc<T, Args...>();
    m_output.push_back(p);
    Get(tails...);
  }

private:
  std::vector<func_pair_t> &m_output;
};
} // namespace reflect

#undef ST_ASSERT
#undef HAS_MEMBER

#endif