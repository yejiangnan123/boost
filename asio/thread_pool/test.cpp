#include <boost/asio/thread_pool.hpp>
#include <boost/asio.hpp>

void my_task() {
}

void test() {
  boost::asio::thread_pool pool(4);
  boost::asio::post(pool, my_task);
  boost::asio::post(pool,
      []()
      {
      });
  pool.join();
}
