#include "indexed_varargs.hpp"
#include "mutils-serialization/SerializationSupport.hpp"
#include "mutils/compile-time-tuple.hpp"
#include <cassert>
#include <memory>
#include <tuple>
#include <utility>

namespace derecho_allocator {
namespace internal {
template <typename... StaticArgs> struct alloc_outer {
  template <typename T> using grow = alloc_outer<StaticArgs..., T>;
  static_assert((std::is_trivially_copyable_v<StaticArgs> && ... && true),
                "Error: outer allocator responsible for non-trivially-copyable "
                "arguments.");
  template <typename... DynamicArgs> struct alloc_inner {
    static_assert(
        ((!std::is_trivially_copyable_v<DynamicArgs>)&&... && true),
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
      // intentionally reaching the end of void function here, should hopefully
      // only be an error when the static_assert would also fail
    }

    template <std::size_t s>
    using get_arg = std::decay_t<decltype(*get_arg_nullptr<s>())>;

    using char_p = unsigned char *;
    const char_p serial_region;
    const std::size_t serial_size;

    using static_args_ptr_t = mutils::ct::tuple<StaticArgs...> *;
    // NOTE: THIS IS OBVIOUSLY BROKEN WITH CURRENT DERECHO!!!
    const static_args_ptr_t static_args = (static_args_ptr_t)serial_region;
    static const constexpr std::size_t static_arg_size = sizeof(static_args);
    static_assert(static_arg_size <= (sizeof(StaticArgs) + ... + 12));

    std::tuple<std::unique_ptr<DynamicArgs>...> allocated_dynamic_args;

    alloc_inner(char_p serial_region, std::size_t serial_size)
        : serial_region(serial_region), serial_size(serial_size) {
      assert(serial_size >= static_arg_size);
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
        mutils::ct::tuple<StaticArgs...> &sargs = *static_args;
        auto &sarg = sargs.template get<arg>();
        new (&sarg) Arg{std::forward<CArgs>(cargs)...};
        return &sarg;
      } else {
        auto &uptr =
            std::get<arg - sizeof...(StaticArgs)>(allocated_dynamic_args);
        uptr.reset(new Arg(std::forward<CArgs>(cargs)...));
        return uptr.get();
      }
    }

    char *serialize() const {
      std::size_t offset{static_arg_size};
      mutils::for_each(
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

template <typename alloc_outer, typename Fst, typename... Rst>
struct _build_allocator<alloc_outer, Fst, Rst...> {

  static decltype(auto) type_builder() {
    if constexpr (std::is_trivially_copyable_v<Fst>) {
      using ret =
          typename _build_allocator<typename alloc_outer::template grow<Fst>,
                                    Rst...>::type;
      return (ret *)nullptr;
    } else {
      using ret = typename alloc_outer::template alloc_inner<Fst, Rst...>;
      return (ret *)nullptr;
    }
  }

  using type = std::decay_t<decltype(*type_builder())>;
};

template <typename... T>
using build_allocator = typename _build_allocator<alloc_outer<>, T...>::type;
} // namespace internal
} // namespace derecho_allocator