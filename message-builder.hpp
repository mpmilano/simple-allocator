#pragma once
#include "build-allocator.hpp"

namespace derecho_allocator {

template <typename... Args> class message_builder {
  using allocator = internal::build_allocator<Args...>;
  allocator a;

public:
  message_builder(unsigned char *serial_region, std::size_t size)
      : a(serial_region, size) {}
  template <std::size_t s, typename... CArgs>
  decltype(auto) build_arg(CArgs &&... cargs) {
    return a.template build_arg<s, CArgs...>(std::forward<CArgs>(cargs)...);
  }

  char *serialize(const arg_ptr<Args> &...) { return a.serialize(); }
};

} // namespace derecho_allocator
