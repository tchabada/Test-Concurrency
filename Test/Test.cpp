#include "stdafx.h"

#define _ENABLE_ATOMIC_ALIGNMENT_FIX

#include <windows.h>

#include <iostream>
#include <vector>
#include <future>
#include <unordered_map>

#include <experimental/generator>
#include <experimental/resumable>

#define BOOST_THREAD_PROVIDES_FUTURE
#define BOOST_THREAD_PROVIDES_FUTURE_CONTINUATION
#define BOOST_THREAD_PROVIDES_FUTURE_WHEN_ALL_WHEN_ANY
#define BOOST_THREAD_PROVIDES_EXECUTORS
#include <boost/thread.hpp>
#include <boost/thread/executors/basic_thread_pool.hpp>
#include <boost/thread/executors/executor.hpp>

#include <boost/lockfree/queue.hpp>

#include <ppltasks.h>
#include <pplawait.h>

/*
namespace boost
{
  template <typename T>
  bool await_ready(boost::future<T> const & t)
  {
    return t.is_ready();
  }

  template <typename T, typename Callback>
  void await_suspend(boost::future<T> & t, Callback resume)
  {
    boost::thread th([&t, resume]
    {
      t.wait();
      resume();
    });
    th.detach();
    
//     t.then([resume](boost::future<T> const &)
//     {
//       resume();
//     });
  }

  template <typename T>
  auto await_resume(boost::future<T> & t)
  {
    return t.get();
  }
}

namespace std {
  namespace experimental {

    template <class T, class... Args>
    struct coroutine_traits<boost::future<T>, Args...>
    {
      struct promise_type
      {
        boost::promise<T> promise;

        boost::future<T> get_return_object()
        {
          return promise.get_future();
        }

        bool initial_suspend() { return false; }

        bool final_suspend() { return false; }

        void set_exception(std::exception_ptr e)
        {
          promise.set_exception(std::move(e));
        }

        void return_value(T & t)
        {
          promise.set_value(t);
        }

        void return_value(T && t)
        {
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

        boost::future<void> get_return_object()
        {
          return promise.get_future();
        }

        bool initial_suspend() { return false; }

        bool final_suspend() { return false; }

        void set_exception(std::exception_ptr e)
        {
          promise.set_exception(std::move(e));
        }

        void return_void()
        {
          promise.set_value();
        }
      };
    };

  }
}

std::future<size_t> calculate(size_t i)
{
  return std::async(std::launch::async, [i] { return i * i; });
}

std::future<std::string> convert(size_t i)
{
  return std::async(std::launch::async, [i] { return std::to_string(i); });
}

std::future<void> test(size_t i)
{
  std::string result = await convert(await calculate(i));
  std::cout << result << " ";
}

boost::future<size_t> calculate(size_t i)
{
  return boost::async([=] { return i * i; });
}

boost::future<std::string> convert(size_t i)
{
  return boost::async([=] { return std::to_string(i); });
}

boost::future<void> test(size_t i)
{
  std::string result = await convert(await calculate(i));
  std::cout << result << " ";
}
*/

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
      timer = CreateThreadpoolTimer(TimerCallback, resume_cb.to_address(), nullptr);
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

Concurrency::task<size_t> calculate(size_t i)
{
  return Concurrency::create_task([i]
  {
    return i * i;
  });
}

Concurrency::task<std::string> convert(size_t i)
{
  return Concurrency::create_task([i]
  {
    return std::to_string(i);
  });
}

Concurrency::task<void> test(size_t x)
{
  await sleep_for(std::chrono::milliseconds(100 * x));
  size_t v = await calculate(x);
  std::string s = await convert(v);

  std::cout << s << " " << std::flush;
}

std::experimental::generator<int> fib()
{
  int a = 0;
  int b = 1;
  while (true)
  {
    yield a;
    auto next = a + b;
    a = b;
    b = next;
  }
}

/*
std::experimental::recursive_generator<int> range(int a, int b)
{
  auto n = b - a;
  if (n <= 0)
    return;
  if (n == 1)
  {
    yield a;
    return;
  }
  auto mid = a + n / 2;
  yield range(a, mid);
  yield range(mid, b);
}
*/

class Point
{
public:
  Point() : Point(0, 0, 0) {}
  Point(int x, int y, int z) : x_(x), y_(y), z_(z) {}
  
private:
  int x_;
  int y_;
  int z_;
};

class String
{
public:
  String() : s_("default") { }

  String(const std::string& s) : s_(s) { std::cout << "constructor\n"; }

  String(const String& other) : s_(other.s_) { std::cout << "copy constructor\n"; }

  String(String&& other) : s_(std::move(other.s_)) { std::cout << "move constructor\n"; }

  String& operator=(const String& other)
  {
    std::cout << "copy assigned\n";
    s_ = other.s_;
    return *this;
  }

  String& operator=(String&& other)
  {
    std::cout << "move assigned\n";
    s_ = std::move(other.s_);
    return *this;
  }

private:
  std::string s_;
};

template <typename T> void println(const T & t)
{
  std::cout << t << std::endl;
}

template <typename First, typename... Rest> void println(const First& first, const Rest&... rest)
{
  std::cout << first << ", ";
  
  println(rest...);
}

int _tmain(int argc, _TCHAR* argv[])
{
  println(1, 1, std::string("a"), std::string("a"));

  std::vector<String> v;
  v.reserve(6);
  
  auto indexes = { 0, 1, 2, 3, 4, 5 };
  for (auto i : indexes)
  {
    v.push_back(std::to_string(i));
    std::cout << std::endl;
  }

  v.clear();
  for (auto i : indexes)
  {
    v.emplace_back(std::to_string(i));
    std::cout << std::endl;
  }

  std::unordered_map<std::string, Point> pm;
  pm.emplace(std::piecewise_construct, std::make_tuple("first"), std::make_tuple(1, 2, 3));

  std::unordered_map<std::string, String> sm;
  sm.emplace(std::piecewise_construct, std::make_tuple("first"), std::make_tuple("test"));

  boost::lockfree::queue<Point> pq(10);
  for (auto i : indexes)
  {
    pq.push(Point(1, 2, 3));
    Point p;
    pq.pop(p);
  }

  {
    for (auto v : fib())
    {
      if (v > 10)
      {
        break;
      }
      std::cout << v << " ";
    }
  }

  std::cout << std::flush << std::endl;

  {
    size_t i = 100;
    std::vector<Concurrency::task<void>> futures;
    while (--i)
    {
      futures.emplace_back(test(i));
    }
    Concurrency::when_all(futures.begin(), futures.end()).wait();
  }

  std::cout << std::flush << std::endl;

  {
    boost::basic_thread_pool tp(4);
    
    size_t i = 100;
    std::vector<boost::future<void>> futures;
    while (--i)
    {
      futures.emplace_back(boost::async(tp, [i]()
      {
        std::this_thread::sleep_for(std::chrono::milliseconds(100 * i));
      }).then(tp, [i](auto f)
      {
        return i * i;
      }).then(tp, [](auto f)
      {
        std::string s = std::to_string(f.get());
        std::cout << s << " " << std::flush;
      }));
    }
    boost::when_all(futures.begin(), futures.end()).wait();
  }

  std::cout << std::flush << std::endl;

  return 0;
}