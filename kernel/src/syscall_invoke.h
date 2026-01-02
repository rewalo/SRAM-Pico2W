#pragma once
#include <functional>
#include <stdint.h>
#include <tuple>
#include <type_traits>
#include <utility>

// Maximum number of syscall arguments supported
#define MAX_SYSCALL_ARGS 30

// Type-safe syscall invocation - converts uintptr_t args back to real types
namespace detail {

template <typename>
struct fn_traits;

// Extract return type and args from function pointer
template <typename R, typename... A>
struct fn_traits<R (*)(A...)> {
  using ret = R;
  using args = std::tuple<A...>;
  static constexpr size_t arity = sizeof...(A);
};

template <typename>
struct mem_traits;

// Extract class, return type, and args from member function pointer
template <typename C, typename R, typename... A>
struct mem_traits<R (C::*)(A...)> {
  using cls = C;
  using ret = R;
  using args = std::tuple<A...>;
  static constexpr size_t arity = sizeof...(A);
};

template <typename C, typename R, typename... A>
struct mem_traits<R (C::*)(A...) const> {
  using cls = const C;
  using ret = R;
  using args = std::tuple<A...>;
  static constexpr size_t arity = sizeof...(A);
};

// Convert uintptr_t back to original type
template <typename T>
static inline T cast_arg_uintptr(uintptr_t v) {
  using U = std::remove_reference_t<T>;
  if constexpr (std::is_pointer_v<U>) {
    return reinterpret_cast<T>(v);
  } else if constexpr (std::is_enum_v<U> || std::is_integral_v<U>) {
    return static_cast<T>(v);
  } else {
    static_assert(!sizeof(U), "Unsupported syscall arg type");
  }
}

// Invoke free function with unpacked args
template <auto Fn, size_t... I>
static inline uintptr_t invoke_impl_free(const uintptr_t raw[MAX_SYSCALL_ARGS],
                                          std::index_sequence<I...>) {
  using F = fn_traits<decltype(Fn)>;
  using R = typename F::ret;
  if constexpr (std::is_void_v<R>) {
    std::invoke(Fn,
                cast_arg_uintptr<
                    std::tuple_element_t<I, typename F::args>>(raw[I])...);
    return 0;
  } else {
    auto r = std::invoke(Fn,
                         cast_arg_uintptr<
                             std::tuple_element_t<I, typename F::args>>(
                             raw[I])...);
    // Use reinterpret_cast for pointer types, static_cast for others
    if constexpr (std::is_pointer_v<typename F::ret>) {
      return reinterpret_cast<uintptr_t>(r);
    } else {
      return static_cast<uintptr_t>(r);
    }
  }
}

template <auto Fn>
static inline uintptr_t invoke(uintptr_t a0, uintptr_t a1, uintptr_t a2,
                                uintptr_t a3, uintptr_t a4, uintptr_t a5,
                                uintptr_t a6, uintptr_t a7, uintptr_t a8,
                                uintptr_t a9, uintptr_t a10, uintptr_t a11,
                                uintptr_t a12, uintptr_t a13, uintptr_t a14,
                                uintptr_t a15, uintptr_t a16, uintptr_t a17,
                                uintptr_t a18, uintptr_t a19, uintptr_t a20,
                                uintptr_t a21, uintptr_t a22, uintptr_t a23,
                                uintptr_t a24, uintptr_t a25, uintptr_t a26,
                                uintptr_t a27, uintptr_t a28, uintptr_t a29) {
  using F = fn_traits<decltype(Fn)>;
  const uintptr_t raw[MAX_SYSCALL_ARGS] = {
      a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14,
      a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29};
  return invoke_impl_free<Fn>(raw,
                               std::make_index_sequence<F::arity>{});
}

// Invoke member function on bound object instance
template <auto ObjPtr, auto MemFn, size_t... I>
static inline uintptr_t invoke_impl_method(const uintptr_t raw[MAX_SYSCALL_ARGS],
                                             std::index_sequence<I...>) {
  using M = mem_traits<decltype(MemFn)>;
  using R = typename M::ret;
  auto& obj = *ObjPtr;
  if constexpr (std::is_void_v<R>) {
    std::invoke(MemFn, obj,
                cast_arg_uintptr<
                    std::tuple_element_t<I, typename M::args>>(raw[I])...);
    return 0;
  } else {
    auto r = std::invoke(MemFn, obj,
                         cast_arg_uintptr<
                             std::tuple_element_t<I, typename M::args>>(
                             raw[I])...);
    // Use reinterpret_cast for pointer types, static_cast for others
    if constexpr (std::is_pointer_v<typename M::ret>) {
      return reinterpret_cast<uintptr_t>(r);
    } else {
      return static_cast<uintptr_t>(r);
    }
  }
}

template <auto ObjPtr, auto MemFn>
static inline uintptr_t invoke_method(uintptr_t a0, uintptr_t a1, uintptr_t a2,
                                       uintptr_t a3, uintptr_t a4, uintptr_t a5,
                                       uintptr_t a6, uintptr_t a7, uintptr_t a8,
                                       uintptr_t a9, uintptr_t a10, uintptr_t a11,
                                       uintptr_t a12, uintptr_t a13, uintptr_t a14,
                                       uintptr_t a15, uintptr_t a16, uintptr_t a17,
                                       uintptr_t a18, uintptr_t a19, uintptr_t a20,
                                       uintptr_t a21, uintptr_t a22, uintptr_t a23,
                                       uintptr_t a24, uintptr_t a25, uintptr_t a26,
                                       uintptr_t a27, uintptr_t a28, uintptr_t a29) {
  using M = mem_traits<decltype(MemFn)>;
  const uintptr_t raw[MAX_SYSCALL_ARGS] = {
      a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14,
      a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29};
  return invoke_impl_method<ObjPtr, MemFn>(
      raw, std::make_index_sequence<M::arity>{});
}

}