#pragma once

#include <chrono>

#include <experimental/resumable>

#define BOOST_THREAD_PROVIDES_FUTURE
#define BOOST_THREAD_PROVIDES_FUTURE_CONTINUATION
#define BOOST_THREAD_PROVIDES_FUTURE_WHEN_ALL_WHEN_ANY
#define BOOST_THREAD_PROVIDES_EXECUTORS
#include <boost/thread.hpp>

auto sleep_for(std::chrono::system_clock::duration duration)
{
  class awaiter
  {
    static void TimerCallback(PTP_CALLBACK_INSTANCE, void* Context, PTP_TIMER)
    {
      std::experimental::coroutine_handle<>::from_address(Context)();
    }

    PTP_TIMER timer = nullptr;
    std::chrono::system_clock::duration duration;

  public:
    awaiter(std::chrono::system_clock::duration d) : duration(d) {}

    bool await_ready() const { return duration.count() <= 0; }

    void await_suspend(std::experimental::coroutine_handle<> resume_cb)
    {
      int64_t relative_count = -duration.count();
      timer = CreateThreadpoolTimer(TimerCallback, resume_cb.address(), nullptr);
      if (timer == 0) throw std::system_error(GetLastError(), std::system_category());
      SetThreadpoolTimer(timer, (PFILETIME)&relative_count, 0, 0);
    }

    void await_resume() {}

    ~awaiter()
    {
      if (timer) CloseThreadpoolTimer(timer);
    }
  };

  return awaiter{ duration };
}

namespace boost {

  template <typename T>
  bool await_ready(future<T> const & f)
  {
    return f.is_ready();
  }

  template <typename T>
  void await_suspend(future<T> & f, std::experimental::coroutine_handle<> resume)
  {
    f.then([resume](future<T> const &)
    {
      resume();
    });
  }

  template <typename T>
  T await_resume(future<T> & f)
  {
    return f.get();
  }

} // namespace boost

namespace std {
  namespace experimental {

    template <class T, class... Args>
    struct coroutine_traits<boost::future<T>, Args...>
    {
      struct promise_type
      {
        boost::promise<T> promise;

        boost::future<T> get_return_object() { return promise.get_future(); }
        bool initial_suspend() { return false; }
        bool final_suspend() { return false; }

        void set_exception(std::exception_ptr e) {
          promise.set_exception(std::move(e));
        }
        /*
        void set_result(T & v) {
          promise.set_value(v);
        }
        void set_result(T && v) {
          promise.set_value(std::move(v));
        }
        */
        void return_value(T & v) {
          promise.set_value(v);
        }
        void return_value(T && v) {
          promise.set_value(std::move(v));
        }
      };
    };

    template <class... Args>
    struct coroutine_traits<boost::future<void>, Args...>
    {
      struct promise_type
      {
        boost::promise<void> promise;

        boost::future<void> get_return_object() { return promise.get_future(); }
        bool initial_suspend() { return false; }
        bool final_suspend() { return false; }

        void set_exception(std::exception_ptr e) {
          promise.set_exception(std::move(e));
        }
        /*
        void set_result() {
          promise.set_value();
        }
        */
        void return_void() {
          promise.set_value();
        }
      };
    };

  } // namespace experimental
} // namespace std

