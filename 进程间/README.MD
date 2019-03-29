# 进程间通信 interprocess
[boost.interprocess](https://www.boost.org/doc/libs/1_69_0/doc/html/interprocess.html)
# 编译配置
```
需要连接-lrt库，cmake配置添加 rt库
```
## 对象是一个简单的buf
```
#include <boost/interprocess/managed_shared_memory.hpp>
#include <cstdlib> //std::system
#include <sstream>
#include <iostream>
#include <memory>
#include <string>
using namespace std;
int main (int argc, char *argv[])
{
   using namespace boost::interprocess;
   if(argc == 1){  //Parent process
      //Remove shared memory on construction and destruction
      struct shm_remove
      {
         shm_remove() {  shared_memory_object::remove("MySharedMemory"); }
         ~shm_remove(){  shared_memory_object::remove("MySharedMemory"); }
      } remover;

      //Create a managed shared memory segment
      managed_shared_memory segment(create_only, "MySharedMemory", 65536);

      //Allocate a portion of the segment (raw memory)
      managed_shared_memory::size_type free_memory = segment.get_free_memory();
      void * shptr = segment.allocate(1024/*bytes to allocate*/);
      string test("MySharedMemory");
      memcpy((char *)shptr,test.c_str(),test.size());
      //Check invariant
      if(free_memory <= segment.get_free_memory())
         return 1;

      //An handle from the base address can identify any byte of the shared
      //memory segment even if it is mapped in different base addresses
      managed_shared_memory::handle_t handle = segment.get_handle_from_address(shptr);
      std::stringstream s;
      s << argv[0] << " " << handle;
      s << std::ends;
      //Launch child process
      if(0 != std::system(s.str().c_str()))
         return 1;
      //Check memory has been freed
      if(free_memory != segment.get_free_memory())
         return 1;
   }
   else{
      //Open managed segment
      managed_shared_memory segment(open_only, "MySharedMemory");

      //An handle from the base address can identify any byte of the shared
      //memory segment even if it is mapped in different base addresses
      managed_shared_memory::handle_t handle = 0;

      //Obtain handle value
      std::stringstream s; s << argv[1]; s >> handle;

      //Get buffer local address from handle
      void *msg = segment.get_address_from_handle(handle);
      string test((char *)msg);
      cout << test << "\n";

      //Deallocate previously allocated memory
      segment.deallocate(msg);
   }
   return 0;
}
```
## 对象可以有名称
```
#include <boost/interprocess/managed_shared_memory.hpp>
#include <cstdlib> //std::system
#include <cstddef>
#include <cassert>
#include <utility>
#include <iostream>
using namespace std;
int main(int argc, char *argv[])
{
   using namespace boost::interprocess;
   typedef std::pair<double, int> MyType;

   if(argc == 1){  //Parent process
      //Remove shared memory on construction and destruction
      struct shm_remove
      {
         shm_remove() { shared_memory_object::remove("MySharedMemory"); }
         ~shm_remove(){ shared_memory_object::remove("MySharedMemory"); }
      } remover;

      //Construct managed shared memory
      managed_shared_memory segment(create_only, "MySharedMemory", 65536);

      //Create an object of MyType initialized to {0.0, 0}
      MyType *instance = segment.construct<MyType>
         ("MyType instance")  //name of the object
         (0.0, 0);            //ctor first argument

      //Create an array of 10 elements of MyType initialized to {0.0, 0}
      MyType *array = segment.construct<MyType>
         ("MyType array")     //name of the object
         [10]                 //number of elements
         (0.0, 0);            //Same two ctor arguments for all objects

      //Create an array of 3 elements of MyType initializing each one
      //to a different value {0.0, 0}, {1.0, 1}, {2.0, 2}...
      float float_initializer[3] = { 0.0, 1.0, 2.0 };
      int   int_initializer[3]   = { 0, 1, 2 };

      MyType *array_it = segment.construct_it<MyType>
         ("MyType array from it")   //name of the object
         [3]                        //number of elements
         ( &float_initializer[0]    //Iterator for the 1st ctor argument
         , &int_initializer[0]);    //Iterator for the 2nd ctor argument

      //Launch child process
      std::string s(argv[0]); s += " child ";
      if(0 != std::system(s.c_str()))
         return 1;


      //Check child has destroyed all objects
      if(segment.find<MyType>("MyType array").first ||
         segment.find<MyType>("MyType instance").first ||
         segment.find<MyType>("MyType array from it").first)
           return 1;
      cout << "parent ok\n";
   }
   else{
      //Open managed shared memory
      managed_shared_memory segment(open_only, "MySharedMemory");

      std::pair<MyType*, managed_shared_memory::size_type> res;

      //Find the array
      res = segment.find<MyType> ("MyType array");
      //Length should be 10
      if(res.second != 10) return 1;

      //Find the object
      res = segment.find<MyType> ("MyType instance");
      //Length should be 1
      if(res.second != 1) return 1;

      //Find the array constructed from iterators
      res = segment.find<MyType> ("MyType array from it");
      //Length should be 3
      if(res.second != 3) return 1;

      //We're done, delete all the objects
      segment.destroy<MyType>("MyType array");
      segment.destroy<MyType>("MyType instance");
      segment.destroy<MyType>("MyType array from it");
      cout << "chlid ok\n";
   }
   return 0;
}
```
## 指针，对象可以是自定义链表
```
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/offset_ptr.hpp>

using namespace boost::interprocess;

//Shared memory linked list node
struct list_node
{
   offset_ptr<list_node> next;
   int                   value;
};

int main ()
{
   //Remove shared memory on construction and destruction
   struct shm_remove
   {
      shm_remove() { shared_memory_object::remove("MySharedMemory"); }
      ~shm_remove(){ shared_memory_object::remove("MySharedMemory"); }
   } remover;

   //Create shared memory
   managed_shared_memory segment(create_only,
                                 "MySharedMemory",  //segment name
                                 65536);

   //Create linked list with 10 nodes in shared memory
   offset_ptr<list_node> prev = 0, current, first;

   int i;
   for(i = 0; i < 10; ++i, prev = current){
      current = static_cast<list_node*>(segment.allocate(sizeof(list_node)));
      current->value = i;
      current->next  = 0;

      if(!prev)
         first = current;
      else
         prev->next = current;
   }

   //Communicate list to other processes
   //. . .
   //When done, destroy list
   for(current = first; current; /**/){
      prev = current;
      current = current->next;
      segment.deallocate(prev.get());
   }
   return 0;
}
```
## 对象是vector容器
```
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <string>
#include <cstdlib> //std::system

using namespace boost::interprocess;

//Define an STL compatible allocator of ints that allocates from the managed_shared_memory.
//This allocator will allow placing containers in the segment
typedef allocator<int, managed_shared_memory::segment_manager>  ShmemAllocator;

//Alias a vector that uses the previous STL-like allocator so that allocates
//its values from the segment
typedef vector<int, ShmemAllocator> MyVector;

//Main function. For parent process argc == 1, for child process argc == 2
int main(int argc, char *argv[])
{
   if(argc == 1){ //Parent process
      //Remove shared memory on construction and destruction
      struct shm_remove
      {
         shm_remove() { shared_memory_object::remove("MySharedMemory"); }
         ~shm_remove(){ shared_memory_object::remove("MySharedMemory"); }
      } remover;

      //Create a new segment with given name and size
      managed_shared_memory segment(create_only, "MySharedMemory", 65536);

      //Initialize shared memory STL-compatible allocator
      const ShmemAllocator alloc_inst (segment.get_segment_manager());

      //Construct a vector named "MyVector" in shared memory with argument alloc_inst
      MyVector *myvector = segment.construct<MyVector>("MyVector")(alloc_inst);

      for(int i = 0; i < 100; ++i)  //Insert data in the vector
         myvector->push_back(i);

      //Launch child process
      std::string s(argv[0]); s += " child ";
      if(0 != std::system(s.c_str()))
         return 1;

      //Check child has destroyed the vector
      if(segment.find<MyVector>("MyVector").first)
         return 1;
   }
   else{ //Child process
      //Open the managed segment
      managed_shared_memory segment(open_only, "MySharedMemory");

      //Find the vector using the c-string name
      MyVector *myvector = segment.find<MyVector>("MyVector").first;

      //Use vector in reverse order
      std::sort(myvector->rbegin(), myvector->rend());

      //When done, destroy the vector from the segment
      segment.destroy<MyVector>("MyVector");
   }

   return 0;
}
```
## 对象是map容器
```
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/containers/map.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <functional>
#include <utility>

int main ()
{
   using namespace boost::interprocess;

   //Remove shared memory on construction and destruction
   struct shm_remove
   {
      shm_remove() { shared_memory_object::remove("MySharedMemory"); }
      ~shm_remove(){ shared_memory_object::remove("MySharedMemory"); }
   } remover;

   //Shared memory front-end that is able to construct objects
   //associated with a c-string. Erase previous shared memory with the name
   //to be used and create the memory segment at the specified address and initialize resources
   managed_shared_memory segment
      (create_only
      ,"MySharedMemory" //segment name
      ,65536);          //segment size in bytes

   //Note that map<Key, MappedType>'s value_type is std::pair<const Key, MappedType>,
   //so the allocator must allocate that pair.
   typedef int    KeyType;
   typedef float  MappedType;
   typedef std::pair<const int, float> ValueType;

   //Alias an STL compatible allocator of for the map.
   //This allocator will allow to place containers
   //in managed shared memory segments
   typedef allocator<ValueType, managed_shared_memory::segment_manager>
      ShmemAllocator;

   //Alias a map of ints that uses the previous STL-like allocator.
   //Note that the third parameter argument is the ordering function
   //of the map, just like with std::map, used to compare the keys.
   typedef map<KeyType, MappedType, std::less<KeyType>, ShmemAllocator> MyMap;

   //Initialize the shared memory STL-compatible allocator
   ShmemAllocator alloc_inst (segment.get_segment_manager());

   //Construct a shared memory map.
   //Note that the first parameter is the comparison function,
   //and the second one the allocator.
   //This the same signature as std::map's constructor taking an allocator
   MyMap *mymap =
      segment.construct<MyMap>("MyMap")      //object name
                                 (std::less<int>() //first  ctor parameter
                                 ,alloc_inst);     //second ctor parameter

   //Insert data in the map
   for(int i = 0; i < 100; ++i){
      mymap->insert(std::pair<const int, float>(i, (float)i));
   }
   return 0;
}
```
## 映射到本地空间
```
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <cstring>
#include <cstdlib>
#include <string>

int main(int argc, char *argv[])
{
   using namespace boost::interprocess;

   if(argc == 1){  //Parent process
      //Remove shared memory on construction and destruction
      struct shm_remove
      {
         shm_remove() { shared_memory_object::remove("MySharedMemory"); }
         ~shm_remove(){ shared_memory_object::remove("MySharedMemory"); }
      } remover;

      //Create a shared memory object.
      shared_memory_object shm (create_only, "MySharedMemory", read_write);

      //Set size
      shm.truncate(1000);

      //Map the whole shared memory in this process
      mapped_region region(shm, read_write);

      //Write all the memory to 1
      std::memset(region.get_address(), 1, region.get_size());

      //Launch child process
      std::string s(argv[0]); s += " child ";
      if(0 != std::system(s.c_str()))
         return 1;
   }
   else{
      //Open already created shared memory object.
      shared_memory_object shm (open_only, "MySharedMemory", read_only);

      //Map the whole shared memory in this process
      mapped_region region(shm, read_only);

      //Check that memory was initialized to 1
      char *mem = static_cast<char*>(region.get_address());
      for(std::size_t i = 0; i < region.get_size(); ++i)
         if(*mem++ != 1)
            return 1;   //Error checking memory
   }
   return 0;
}
```
