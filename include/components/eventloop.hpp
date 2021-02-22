#pragma once

#include <uv.h>

#include "common.hpp"

POLYBAR_NS

template <class H, class... Args>
struct cb_helper {
  std::function<void(Args...)> func;

  static void callback(H* context, Args... args) {
    const auto unpackedThis = static_cast<const cb_helper*>(context->data);
    return unpackedThis->func(std::forward<Args>(args)...);
  }
};

class eventloop {
 public:
  eventloop();
  ~eventloop();

  void run();

  void stop();

  /**
   * TODO make protected
   */
  uv_loop_t* get() const {
    return m_loop.get();
  }

  void signal_handler(int signum, std::function<void(int)> fun) {
    auto signal_handle = std::make_unique<uv_signal_t>();
    uv_signal_init(get(), signal_handle.get());

    auto helper = cb_helper<uv_signal_t, int>{fun};
    // FIXME &helper is pointer to invalid stack location after this function returns.
    signal_handle->data = &helper;

    uv_signal_start(signal_handle.get(), &helper.callback, signum);
    m_sig_handles.emplace_back(signal_handle);
  }

 protected:
  template <typename H, typename T, typename... Args, void (T::*F)(Args...)>
  static void generic_cb(H* handle, Args&&... args) {
    (static_cast<T*>(handle->data).*F)(std::forward<Args>(args)...);
  }

 private:
  std::unique_ptr<uv_loop_t> m_loop{nullptr};

  vector<unique_ptr<uv_signal_t>> m_sig_handles;
};

POLYBAR_NS_END
