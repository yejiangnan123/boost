# boost锁 锁
```
互斥量，读写锁，独占锁，共享锁。智能锁，递归锁，超时锁，条件变量，原子操作，自旋锁
递归锁：同一个线程可以多次上锁
```
# mutex
```
#include <iostream>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>

boost::mutex mutex;
int count = 0;

void Counter() {
  mutex.lock();

  int i = ++count;
  std::cout << "count == " << i << std::endl;

  // 前面代码如有异常，unlock 就调不到了。
  mutex.unlock();
}

int main() {
  boost::thread_group threads;
  for (int i = 0; i < 4; ++i) {
    threads.create_thread(&Counter);
  }

  threads.join_all();
  return 0;
}
```
# lock_guard 
```
#include <iostream>
#include <boost/thread/lock_guard.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>

boost::mutex mutex;
int count = 0;

void Counter() {
  // lock_guard 在构造函数里加锁，在析构函数里解锁。
  boost::lock_guard<boost::mutex> lock(mutex);

  int i = ++count;
  std::cout << "count == " << i << std::endl;
}

int main() {
  boost::thread_group threads;
  for (int i = 0; i < 4; ++i) {
    threads.create_thread(&Counter);
  }

  threads.join_all();
  return 0;
}
```
# unique_lock 
```
#include <iostream>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>

boost::mutex mutex;
int count = 0;

void Counter() {
  boost::unique_lock<boost::mutex> lock(mutex);

  int i = ++count;
  std::cout << "count == " << i << std::endl;
}

int main() {
  boost::thread_group threads;
  for (int i = 0; i < 4; ++i) {
    threads.create_thread(&Counter);
  }

  threads.join_all();
  return 0;
}
```
# stl 条件变量  （boost条件变量类似）
```
// condition_variable example
#include <iostream>           // std::cout
#include <thread>             // std::thread
#include <mutex>              // std::mutex, std::unique_lock
#include <condition_variable> // std::condition_variable

std::mutex mtx;
std::condition_variable cv;
bool ready = false;

void print_id (int id) {
  std::unique_lock<std::mutex> lck(mtx);
  while (!ready) cv.wait(lck);
  // ...
  std::cout << "thread " << id << '\n';
}

void go() {
  std::unique_lock<std::mutex> lck(mtx);
  ready = true;
  cv.notify_all();  //  notify_one();
}

int main ()
{
  std::thread threads[10];
  // spawn 10 threads:
  for (int i=0; i<10; ++i)
    threads[i] = std::thread(print_id,i);

  std::cout << "10 threads ready to race...\n";
  go();                       // go!

  for (auto& th : threads) th.join();

  return 0;
}
```
