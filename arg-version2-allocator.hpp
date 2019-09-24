#include "indexed_varargs.hpp"
#include "mutils-serialization/SerializationSupport.hpp"
#include <cassert>
#include <memory>
#include <tuple>
#include <utility>

namespace derecho_allocator {

namespace internal {
template <typename T> struct deleter {
  constexpr deleter() noexcept = default;
  template <class U> deleter(const deleter<U> &) {}
  template <class U> deleter(deleter<U> &&) {}
  constexpr void operator()(T *) const {}
};
} // namespace internal

template <typename T> using arg_ptr = std::unique_ptr<T, internal::deleter<T>>;

namespace internal {
template <typename... StaticArgs> struct alloc_outer {

  static_assert((std::is_trivially_copyable_v<StaticArgs> && ...),
                "Internal error: alloc_outer args must be trivially copyable");
  template <typename... DynamicArgs> struct alloc_inner {
    static_assert(
        ((!std::is_trivially_copyable_v<DynamicArgs>)&&...),
        "Error: trivially-copyable arguments appear after "
        "dynamically-allocted arguments. This method cannot be "
        "invoked via prepare"); // or whatever "prepare" is ultimately called

    template <std::size_t indx> static decltype(auto) get_arg_nullptr() {
      static_assert(indx < sizeof...(DynamicArgs) + sizeof...(StaticArgs),
                    "Error: attempt to access argument outside of range");
      if constexpr (indx < sizeof...(StaticArgs)) {
        return (type_at_index<indx, StaticArgs...> *)nullptr;
      } else if constexpr (indx >= sizeof...(StaticArgs)) {
        constexpr const auto new_indx = indx - sizeof...(StaticArgs);
        return (type_at_index<new_indx, DynamicArgs...> *)nullptr;
      }
      // intentionally reaching the end of non-void function here, should
      // hopefully only be an error when the static_assert would also fail
    }

    template <std::size_t s>
    using get_arg = std::decay_t<decltype(*get_arg_nullptr<s>())>;

    using char_p = unsigned char *;
    const char_p serial_region;
    const std::size_t serial_size;
    static const constexpr auto static_arg_size = (sizeof(StaticArgs) + ...);

    std::tuple<StaticArgs *...> static_args;
    std::tuple<std::unique_ptr<DynamicArgs>...> allocated_dynamic_args;

    alloc_inner(char_p serial_region, std::size_t serial_size)
        : serial_region(serial_region), serial_size(serial_size) {
      assert(serial_size >= static_arg_size);
      char_p offset = serial_region;
      std::apply(
          [&](auto &... static_args) {
            std::size_t indx = 0;
            const constexpr std::size_t sizes[] = {sizeof(StaticArgs)...};
            for (char_p *ptr : {((unsigned char **)&static_args)...}) {
              *ptr = offset;
              offset += sizes[indx];
              ++indx;
            }
          },
          static_args);
    }

    template <std::size_t s> static constexpr bool is_static() {
      return s < sizeof...(StaticArgs);
    }

    template <std::size_t arg, typename... CArgs>
    decltype(auto) build_arg(CArgs &&... cargs) {
      static_assert((arg < (sizeof...(StaticArgs) + sizeof...(DynamicArgs))),
                    "Error: index out of bounds");

      using Arg = get_arg<arg>;
      if constexpr (is_static<arg>()) {
        auto *sarg = std::get<arg>(static_args);
        new (sarg) Arg{std::forward<CArgs>(cargs)...};
        return arg_ptr<Arg>{sarg};
      } else {
        auto &uptr =
            std::get<arg - sizeof...(StaticArgs)>(allocated_dynamic_args);
        uptr.reset(new Arg(std::forward<CArgs>(cargs)...));
        return arg_ptr<Arg>{uptr.get()};
      }
    }

    char *serialize() const {
      std::size_t offset{static_arg_size};
      mutils::foreach (
          [&](const auto &uptr) {
            offset += mutils::to_bytes(*uptr, offset + serial_region);
          },
          allocated_dynamic_args);
      return serial_region;
    }
  };
};

template <typename alloc_outer, typename... T> struct _build_allocator;

template <typename a> struct _build_allocator<a> {
  using type = typename alloc_outer<>::template alloc_inner<>;
};

template <typename _alloc_outer, typename Fst, typename... Rst>
struct _build_allocator<_alloc_outer, Fst, Rst...> {

  template <typename... outer_params>
  static decltype(auto) type_builder(alloc_outer<outer_params...> *) {
    if constexpr (std::is_trivially_copyable_v<Fst>) {
      using ret = typename _build_allocator<alloc_outer<outer_params..., Fst>,
                                            Rst...>::type;
      return (ret *)nullptr;
    } else {
      using ret =
          typename alloc_outer<outer_params...>::template alloc_inner<Fst,
                                                                      Rst...>;
      return (ret *)nullptr;
    }
  }

  using type = std::decay_t<decltype(*type_builder((_alloc_outer *)nullptr))>;
};

template <typename... T>
using build_allocator = typename _build_allocator<alloc_outer<>, T...>::type;
} // namespace internal
} // namespace derecho_allocator
