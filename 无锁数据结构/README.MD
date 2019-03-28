# 无锁数据结构 Lockfree
[boost.Lockfree](https://www.boost.org/doc/libs/1_69_0/doc/html/lockfree.html)
```
boost::lockfree::queue
boost::lockfree::stack
boost::lockfree::spsc_queue
```
## boost::lockfree::queue 多入多出队列
```
#include <boost/thread/thread.hpp>
#include <boost/lockfree/queue.hpp>
#include <iostream>

#include <boost/atomic.hpp>

boost::atomic_int producer_count(0);
boost::atomic_int consumer_count(0);

boost::lockfree::queue<int> queue(128);

const int iterations = 10000000;
const int producer_thread_count = 4;
const int consumer_thread_count = 4;

void producer(void)
{
    for (int i = 0; i != iterations; ++i) {
        int value = ++producer_count;
        while (!queue.push(value))
            ;
    }
}

boost::atomic<bool> done (false);
void consumer(void)
{
    int value;
    while (!done) {
        while (queue.pop(value))
            ++consumer_count;
    }

    while (queue.pop(value))
        ++consumer_count;
}

int main(int argc, char* argv[])
{
    using namespace std;
    cout << "boost::lockfree::queue is ";
    if (!queue.is_lock_free())
        cout << "not ";
    cout << "lockfree" << endl;

    boost::thread_group producer_threads, consumer_threads;

    for (int i = 0; i != producer_thread_count; ++i)
        producer_threads.create_thread(producer);

    for (int i = 0; i != consumer_thread_count; ++i)
        consumer_threads.create_thread(consumer);

    producer_threads.join_all();
    done = true;

    consumer_threads.join_all();

    cout << "produced " << producer_count << " objects." << endl;
    cout << "consumed " << consumer_count << " objects." << endl;
}
```
## boost::lockfree::stack 多入多出栈
```
#include <boost/thread/thread.hpp>
#include <boost/lockfree/stack.hpp>
#include <iostream>

#include <boost/atomic.hpp>

boost::atomic_int producer_count(0);
boost::atomic_int consumer_count(0);

boost::lockfree::stack<int> stack(128);

const int iterations = 1000000;
const int producer_thread_count = 4;
const int consumer_thread_count = 4;

void producer(void)
{
    for (int i = 0; i != iterations; ++i) {
        int value = ++producer_count;
        while (!stack.push(value))
            ;
    }
}

boost::atomic<bool> done (false);

void consumer(void)
{
    int value;
    while (!done) {
        while (stack.pop(value))
            ++consumer_count;
    }

    while (stack.pop(value))
        ++consumer_count;
}

int main(int argc, char* argv[])
{
    using namespace std;
    cout << "boost::lockfree::stack is ";
    if (!stack.is_lock_free())
        cout << "not ";
    cout << "lockfree" << endl;

    boost::thread_group producer_threads, consumer_threads;

    for (int i = 0; i != producer_thread_count; ++i)
        producer_threads.create_thread(producer);

    for (int i = 0; i != consumer_thread_count; ++i)
        consumer_threads.create_thread(consumer);

    producer_threads.join_all();
    done = true;

    consumer_threads.join_all();

    cout << "produced " << producer_count << " objects." << endl;
    cout << "consumed " << consumer_count << " objects." << endl;
}
```
## boost::lockfree::spsc_queue  单入单出队列
```
#include <boost/thread/thread.hpp>
#include <boost/lockfree/spsc_queue.hpp>
#include <iostream>

#include <boost/atomic.hpp>

int producer_count = 0;
boost::atomic_int consumer_count (0);

boost::lockfree::spsc_queue<int, boost::lockfree::capacity<1024> > spsc_queue;

const int iterations = 10000000;

void producer(void)
{
    for (int i = 0; i != iterations; ++i) {
        int value = ++producer_count;
        while (!spsc_queue.push(value))
            ;
    }
}

boost::atomic<bool> done (false);

void consumer(void)
{
    int value;
    while (!done) {
        while (spsc_queue.pop(value))
            ++consumer_count;
    }

    while (spsc_queue.pop(value))
        ++consumer_count;
}

int main(int argc, char* argv[])
{
    using namespace std;
    cout << "boost::lockfree::queue is ";
    if (!spsc_queue.is_lock_free())
        cout << "not ";
    cout << "lockfree" << endl;

    boost::thread producer_thread(producer);
    boost::thread consumer_thread(consumer);

    producer_thread.join();
    done = true;
    consumer_thread.join();

    cout << "produced " << producer_count << " objects." << endl;
    cout << "consumed " << consumer_count << " objects." << endl;
}
```
