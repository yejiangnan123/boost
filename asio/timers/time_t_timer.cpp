#include <boost/asio.hpp>
#include <ctime>
#include <chrono>
#include <iostream>

struct time_t_clock
{
  typedef std::chrono::steady_clock::duration duration;
  typedef duration::rep rep;
  typedef duration::period period;
  typedef std::chrono::time_point<time_t_clock> time_point;
  static constexpr bool is_steady = false;

  static time_point now() noexcept
  {
    return time_point() + std::chrono::seconds(std::time(0));
  }
};

struct time_t_wait_traits
{
  static time_t_clock::duration to_wait_duration(
      const time_t_clock::duration& d)
  {
    if (d > std::chrono::seconds(1))
      return d - std::chrono::seconds(1);
    else if (d > std::chrono::seconds(0))
      return std::chrono::milliseconds(10);
    else
      return std::chrono::seconds(0);
  }

  static time_t_clock::duration to_wait_duration(
      const time_t_clock::time_point& t)
  {
    return to_wait_duration(t - time_t_clock::now());
  }
};

typedef boost::asio::basic_waitable_timer<
  time_t_clock, time_t_wait_traits> time_t_timer;

int main()
{
  try
  {
    boost::asio::io_context io_context;

    time_t_timer timer(io_context);

    timer.expires_after(std::chrono::seconds(5));
    std::cout << "Starting synchronous wait\n";
    timer.wait();
    std::cout << "Finished synchronous wait\n";

    timer.expires_after(std::chrono::seconds(5));
    std::cout << "Starting asynchronous wait\n";
    timer.async_wait(
        [](const boost::system::error_code& /*error*/)
        {
          std::cout << "timeout\n";
        });
    io_context.run();
    std::cout << "Finished asynchronous wait\n";
  }
  catch (std::exception& e)
  {
    std::cout << "Exception: " << e.what() << "\n";
  }

  return 0;
}
