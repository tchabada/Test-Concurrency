#pragma once

#define BOOST_THREAD_PROVIDES_FUTURE
#define BOOST_THREAD_PROVIDES_FUTURE_CONTINUATION

#include "boost/thread/future.hpp"
#include <experimental/resumable>

namespace boost {

  template <typename T>
  bool await_ready(future<T> const & t)
  {
    return t.is_ready();
  }

  template <typename T, typename Callback>
  void await_suspend(future<T> & t, Callback resume)
  {
    t.then([resume](future<T> const &)
    {
      resume();
    });
  }

  template <typename T>
  T await_resume(future<T> & t)
  {
    return t.get();
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
        void set_result(T & t) {
          promise.set_value(t);
        }
        void set_result(T && t) {
          promise.set_value(std::move(t));
        }
        */
        void return_value(T & t) {
          promise.set_value(t);
        }
        void return_value(T && t) {
          promise.set_value(std::move(t));
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

