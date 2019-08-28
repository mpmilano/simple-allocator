
#include <type_traits>
/*
#include <tuple>
template <std::size_t, typename...> struct sequence;

template <std::size_t s, typename Single> struct sequence<s, Single> {
  using type = std::tuple<std::integral_constant<std::size_t, s>>;
};

template <std::size_t so_far, typename First, typename... Rest>
struct sequence<so_far, First, Rest...> {
  using type =
      std::tuple_cat<std::tuple<std::integral_constant<std::size_t, so_far>>,
                     typename sequence<so_far + 1, Rest...>::type>;
};

template <typename, std::size_t> struct indexed_type;

template <template <typename...> class Target, typename... types>
struct build_with_sequence {
  template <std::size_t... numbers> struct indices {
    using type = Target<index_pair<types, numbers>...>;
  };
  template <std::size_t... numbers>
  constexpr decltype(auto) from_tuple(
      std::tuple<std::integral_constant<std::size_t, numbers>...>) const {
    return indices<numbers...>
  }

  using type =
      typename decltype(from_tuple(typename sequence<0, types...>))::type;
};*/

template <std::size_t, typename...> struct _type_at_index;

template <typename T> struct _type_at_index<0, T> { using type = T; };

template <std::size_t index, typename T1, typename... TN>
struct _type_at_index<index, T1, TN...> {
  using type = std::conditional_t<index == 0, T1,
                                  typename _type_at_index<index, TN...>::type>;
};

template <std::size_t s, typename... T>
using type_at_index = typename _type_at_index<s, T...>::type;

template <typename T, typename... U>
constexpr bool type_has_index(const std::size_t desired_index) {

#define switch_case_2378428394(N)                                              \
  if constexpr (sizeof...(U) > N) {                                            \
    if (desired_index == N)                                                    \
      return std::is_same_v<type_at_index<N, U...>, T>;                        \
  }

  switch_case_2378428394(1);
  switch_case_2378428394(2);
  switch_case_2378428394(3);
  switch_case_2378428394(4);
  switch_case_2378428394(5);
  switch_case_2378428394(6);
  switch_case_2378428394(7);
  switch_case_2378428394(8);
  switch_case_2378428394(9);
  switch_case_2378428394(10);
  switch_case_2378428394(11);
  switch_case_2378428394(12);
  switch_case_2378428394(13);
  switch_case_2378428394(14);
  switch_case_2378428394(15);
  switch_case_2378428394(16);
  switch_case_2378428394(17);
  switch_case_2378428394(18);
  switch_case_2378428394(19);
  switch_case_2378428394(20);
  switch_case_2378428394(21);
  switch_case_2378428394(22);
  switch_case_2378428394(23);
  switch_case_2378428394(24);
  switch_case_2378428394(25);
  switch_case_2378428394(0);
  return false;
}