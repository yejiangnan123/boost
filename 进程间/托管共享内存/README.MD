# 目录
```
分配释放内存段
对象名称类型长度
内存段可以增长
分配时指定对齐
一次分配多个缓冲区
可以重新调整内存大小
写时复制模式，写的时候相当于创建一个副本
静态缓冲区
堆缓冲区
```
## 分配释放内存段
```cpp
#include <boost/interprocess/managed_shared_memory.hpp>

int main()
{
   using namespace boost::interprocess;

   //Remove shared memory on construction and destruction
   struct shm_remove
   {
      shm_remove() { shared_memory_object::remove("MySharedMemory"); }
      ~shm_remove(){ shared_memory_object::remove("MySharedMemory"); }
   } remover;

   //Managed memory segment that allocates portions of a shared memory
   //segment with the default management algorithm
   managed_shared_memory managed_shm(create_only,"MySharedMemory", 65536);

   //Allocate 100 bytes of memory from segment, throwing version
   void *ptr = managed_shm.allocate(100);

   //Deallocate it
   managed_shm.deallocate(ptr);

   //Non throwing version
   ptr = managed_shm.allocate(100, std::nothrow);

   //Deallocate it
   managed_shm.deallocate(ptr);
   return 0;
}
```
# 对象名称类型长度
```cpp
#include <boost/interprocess/managed_shared_memory.hpp>
#include <cassert>
#include <cstring>

class my_class
{
   //...
};

int main()
{
   using namespace boost::interprocess;

   //Remove shared memory on construction and destruction
   struct shm_remove
   {
      shm_remove() { shared_memory_object::remove("MySharedMemory"); }
      ~shm_remove(){ shared_memory_object::remove("MySharedMemory"); }
   } remover;

   managed_shared_memory managed_shm(create_only, "MySharedMemory", 10000*sizeof(std::size_t));

   //Construct objects
   my_class *named_object  = managed_shm.construct<my_class>("Object name")[1]();
   my_class *unique_object = managed_shm.construct<my_class>(unique_instance)[2]();
   my_class *anon_object   = managed_shm.construct<my_class>(anonymous_instance)[3]();

   //Now test "get_instance_name" function.
   assert(0 == std::strcmp(managed_shared_memory::get_instance_name(named_object), "Object name"));
   assert(0 == std::strcmp(managed_shared_memory::get_instance_name(unique_object), typeid(my_class).name()));
   assert(0 == managed_shared_memory::get_instance_name(anon_object));

   //Now test "get_instance_type" function.
   assert(named_type     == managed_shared_memory::get_instance_type(named_object));
   assert(unique_type    == managed_shared_memory::get_instance_type(unique_object));
   assert(anonymous_type == managed_shared_memory::get_instance_type(anon_object));

   //Now test "get_instance_length" function.
   assert(1 == managed_shared_memory::get_instance_length(named_object));
   assert(2 == managed_shared_memory::get_instance_length(unique_object));
   assert(3 == managed_shared_memory::get_instance_length(anon_object));

   managed_shm.destroy_ptr(named_object);
   managed_shm.destroy_ptr(unique_object);
   managed_shm.destroy_ptr(anon_object);
   return 0;
}
```
## 内存段可以增长
```cpp
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/managed_mapped_file.hpp>
#include <cassert>

class MyClass
{
   //...
};

int main()
{
   using namespace boost::interprocess;
   //Remove shared memory on construction and destruction
   struct shm_remove
   {
      shm_remove() { shared_memory_object::remove("MySharedMemory"); }
      ~shm_remove(){ shared_memory_object::remove("MySharedMemory"); }
   } remover;

   {
      //Create a managed shared memory
      managed_shared_memory shm(create_only, "MySharedMemory", 1000);

      //Check size
      assert(shm.get_size() == 1000);
      //Construct a named object
      MyClass *myclass = shm.construct<MyClass>("MyClass")();
      //The managed segment is unmapped here
   }
   {
      //Now that the segment is not mapped grow it adding extra 500 bytes
      managed_shared_memory::grow("MySharedMemory", 500);

      //Map it again
      managed_shared_memory shm(open_only, "MySharedMemory");
      //Check size
      assert(shm.get_size() == 1500);
      //Check "MyClass" is still there
      MyClass *myclass = shm.find<MyClass>("MyClass").first;
      assert(myclass != 0);
      //The managed segment is unmapped here
   }
   {
      //Now minimize the size of the segment
      managed_shared_memory::shrink_to_fit("MySharedMemory");

      //Map it again
      managed_shared_memory shm(open_only, "MySharedMemory");
      //Check size
      assert(shm.get_size() < 1000);
      //Check "MyClass" is still there
      MyClass *myclass = shm.find<MyClass>("MyClass").first;
      assert(myclass != 0);
      //The managed segment is unmapped here
   }
   return 0;
}
```
## 分配时指定对齐
```cpp
#include <boost/interprocess/managed_shared_memory.hpp>
#include <cassert>

int main()
{
   using namespace boost::interprocess;

   //Remove shared memory on construction and destruction
   struct shm_remove
   {
      shm_remove() { shared_memory_object::remove("MySharedMemory"); }
      ~shm_remove(){ shared_memory_object::remove("MySharedMemory"); }
   } remover;

   //Managed memory segment that allocates portions of a shared memory
   //segment with the default management algorithm
   managed_shared_memory managed_shm(create_only, "MySharedMemory", 65536);

   const std::size_t Alignment = 128;

   //Allocate 100 bytes aligned to Alignment from segment, throwing version
   void *ptr = managed_shm.allocate_aligned(100, Alignment);

   //Check alignment
   assert((static_cast<char*>(ptr)-static_cast<char*>(0)) % Alignment == 0);

   //Deallocate it
   managed_shm.deallocate(ptr);

   //Non throwing version
   ptr = managed_shm.allocate_aligned(100, Alignment, std::nothrow);

   //Check alignment
   assert((static_cast<char*>(ptr)-static_cast<char*>(0)) % Alignment == 0);

   //Deallocate it
   managed_shm.deallocate(ptr);

   //If we want to efficiently allocate aligned blocks of memory
   //use managed_shared_memory::PayloadPerAllocation value
   assert(Alignment > managed_shared_memory::PayloadPerAllocation);

   //This allocation will maximize the size of the aligned memory
   //and will increase the possibility of finding more aligned memory
   ptr = managed_shm.allocate_aligned
      (3*Alignment - managed_shared_memory::PayloadPerAllocation, Alignment);

   //Check alignment
   assert((static_cast<char*>(ptr)-static_cast<char*>(0)) % Alignment == 0);

   //Deallocate it
   managed_shm.deallocate(ptr);

   return 0;
}
```
## 一次分配多个缓冲区
```cpp
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/move/utility_core.hpp> //boost::move
#include <cassert>//assert
#include <cstring>//std::memset
#include <new>    //std::nothrow
#include <vector> //std::vector

int main()
{
   using namespace boost::interprocess;
   typedef managed_shared_memory::multiallocation_chain multiallocation_chain;

   //Remove shared memory on construction and destruction
   struct shm_remove
   {
      shm_remove() { shared_memory_object::remove("MySharedMemory"); }
      ~shm_remove(){ shared_memory_object::remove("MySharedMemory"); }
   } remover;

   managed_shared_memory managed_shm(create_only,"MySharedMemory", 65536);

   //Allocate 16 elements of 100 bytes in a single call. Non-throwing version.
   multiallocation_chain chain;
   managed_shm.allocate_many(std::nothrow, 100, 16, chain);

   //Check if the memory allocation was successful
   if(chain.empty()) return 1;

   //Allocated buffers
   std::vector<void*> allocated_buffers;

   //Initialize our data
   while(!chain.empty()){
      void *buf = chain.pop_front();
      allocated_buffers.push_back(buf);
      //The iterator must be incremented before overwriting memory
      //because otherwise, the iterator is invalidated.
      std::memset(buf, 0, 100);
   }

   //Now deallocate
   while(!allocated_buffers.empty()){
      managed_shm.deallocate(allocated_buffers.back());
      allocated_buffers.pop_back();
   }

   //Allocate 10 buffers of different sizes in a single call. Throwing version
   managed_shared_memory::size_type sizes[10];
   for(std::size_t i = 0; i < 10; ++i)
      sizes[i] = i*3;

   managed_shm.allocate_many(sizes, 10, 1, chain);
   managed_shm.deallocate_many(chain);
   return 0;
}
```
## 可以重新调整内存大小
```cpp
#include <boost/interprocess/managed_shared_memory.hpp>
#include <cassert>

int main()
{
   using namespace boost::interprocess;

   //Remove shared memory on construction and destruction
   struct shm_remove
   {
      shm_remove() { shared_memory_object::remove("MySharedMemory"); }
      ~shm_remove(){ shared_memory_object::remove("MySharedMemory"); }
   } remover;

   //Managed memory segment that allocates portions of a shared memory
   //segment with the default management algorithm
   managed_shared_memory managed_shm(create_only, "MySharedMemory", 10000*sizeof(std::size_t));

   //Allocate at least 100 bytes, 1000 bytes if possible
   managed_shared_memory::size_type min_size = 100;
   managed_shared_memory::size_type first_received_size = 1000;
   std::size_t *hint = 0;
   std::size_t *ptr = managed_shm.allocation_command<std::size_t>
      (boost::interprocess::allocate_new, min_size, first_received_size, hint);

   //Received size must be bigger than min_size
   assert(first_received_size >= min_size);

   //Get free memory
   managed_shared_memory::size_type free_memory_after_allocation = managed_shm.get_free_memory();

   //Now write the data
   for(std::size_t i = 0; i < first_received_size; ++i) ptr[i] = i;

   //Now try to triplicate the buffer. We won't admit an expansion
   //lower to the double of the original buffer.
   //This "should" be successful since no other class is allocating
   //memory from the segment
   min_size = first_received_size*2;
   managed_shared_memory::size_type expanded_size = first_received_size*3;
   std::size_t * ret = managed_shm.allocation_command
      (boost::interprocess::expand_fwd, min_size, expanded_size, ptr);
   //Check invariants
   assert(ptr != 0);
   assert(ret == ptr);
   assert(expanded_size >= first_received_size*2);

   //Get free memory and compare
   managed_shared_memory::size_type free_memory_after_expansion = managed_shm.get_free_memory();
   assert(free_memory_after_expansion < free_memory_after_allocation);

   //Write new values
   for(std::size_t i = first_received_size; i < expanded_size; ++i)  ptr[i] = i;

   //Try to shrink approximately to min_size, but the new size
   //should be smaller than min_size*2.
   //This "should" be successful since no other class is allocating
   //memory from the segment
   managed_shared_memory::size_type shrunk_size = min_size;
   ret = managed_shm.allocation_command
      (boost::interprocess::shrink_in_place, min_size*2, shrunk_size, ptr);

   //Check invariants
   assert(ptr != 0);
   assert(ret == ptr);
   assert(shrunk_size <= min_size*2);
   assert(shrunk_size >= min_size);

   //Get free memory and compare
   managed_shared_memory::size_type free_memory_after_shrinking = managed_shm.get_free_memory();
   assert(free_memory_after_shrinking > free_memory_after_expansion);

   //Deallocate the buffer
   managed_shm.deallocate(ptr);
   return 0;
}
```
## 写时复制模式，写的时候相当于创建一个副本
```cpp
#include <boost/interprocess/managed_mapped_file.hpp>
#include <fstream> //std::fstream
#include <iterator>//std::distance


int main()
{
   using namespace boost::interprocess;

   //Define file names
   const char *ManagedFile  = "MyManagedFile";
   const char *ManagedFile2 = "MyManagedFile2";

   //Try to erase any previous managed segment with the same name
   file_mapping::remove(ManagedFile);
   file_mapping::remove(ManagedFile2);
   remove_file_on_destroy destroyer1(ManagedFile);
   remove_file_on_destroy destroyer2(ManagedFile2);

   {
      //Create an named integer in a managed mapped file
      managed_mapped_file managed_file(create_only, ManagedFile, 65536);
      managed_file.construct<int>("MyInt")(0u);

      //Now create a copy on write version
      managed_mapped_file managed_file_cow(open_copy_on_write, ManagedFile);

      //Erase the int and create a new one
      if(!managed_file_cow.destroy<int>("MyInt"))
         throw int(0);
      managed_file_cow.construct<int>("MyInt2");

      //Check changes
      if(managed_file_cow.find<int>("MyInt").first && !managed_file_cow.find<int>("MyInt2").first)
         throw int(0);

      //Check the original is intact
      if(!managed_file.find<int>("MyInt").first && managed_file.find<int>("MyInt2").first)
         throw int(0);

      {  //Dump the modified copy on write segment to a file
         std::fstream file(ManagedFile2, std::ios_base::out | std::ios_base::binary);
         if(!file)
            throw int(0);
       file.write(static_cast<const char *>(managed_file_cow.get_address()), (std::streamsize)managed_file_cow.get_size());
      }

      //Now open the modified file and test changes
      managed_mapped_file managed_file_cow2(open_only, ManagedFile2);
      if(managed_file_cow2.find<int>("MyInt").first && !managed_file_cow2.find<int>("MyInt2").first)
         throw int(0);
   }
   {
      //Now create a read-only version
      managed_mapped_file managed_file_ro(open_read_only, ManagedFile);

      //Check the original is intact
      if(!managed_file_ro.find<int>("MyInt").first && managed_file_ro.find<int>("MyInt2").first)
         throw int(0);

      //Check the number of named objects using the iterators
      if(std::distance(managed_file_ro.named_begin(),  managed_file_ro.named_end())  != 1 &&
         std::distance(managed_file_ro.unique_begin(), managed_file_ro.unique_end()) != 0 )
         throw int(0);
   }
   return 0;
}
```
## 静态缓冲区
```cpp
#include <boost/interprocess/managed_external_buffer.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/containers/list.hpp>
#include <cstring>
#include <boost/aligned_storage.hpp>

int main()
{
   using namespace boost::interprocess;

   //Create the static memory who will store all objects
   const int memsize = 65536;

   static boost::aligned_storage<memsize>::type static_buffer;

   //This managed memory will construct objects associated with
   //a wide string in the static buffer
   wmanaged_external_buffer objects_in_static_memory
      (create_only, &static_buffer, memsize);

   //We optimize resources to create 100 named objects in the static buffer
   objects_in_static_memory.reserve_named_objects(100);

   //Alias an integer node allocator type
   //This allocator will allocate memory inside the static buffer
   typedef allocator<int, wmanaged_external_buffer::segment_manager>
      allocator_t;

   //Alias a STL compatible list to be constructed in the static buffer
   typedef list<int, allocator_t>    MyBufferList;

   //The list must be initialized with the allocator
   //All objects created with objects_in_static_memory will
   //be stored in the static_buffer!
   MyBufferList *list = objects_in_static_memory.construct<MyBufferList>(L"MyList")
                           (objects_in_static_memory.get_segment_manager());
   //Since the allocation algorithm from wmanaged_external_buffer uses relative
   //pointers and all the pointers constructed int the static memory point
   //to objects in the same segment,  we can create another static buffer
   //from the first one and duplicate all the data.
   static boost::aligned_storage<memsize>::type static_buffer2;
   std::memcpy(&static_buffer2, &static_buffer, memsize);

   //Now open the duplicated managed memory passing the memory as argument
   wmanaged_external_buffer objects_in_static_memory2
      (open_only, &static_buffer2, memsize);

   //Check that "MyList" has been duplicated in the second buffer
   if(!objects_in_static_memory2.find<MyBufferList>(L"MyList").first)
      return 1;

   //Destroy the lists from the static buffers
   objects_in_static_memory.destroy<MyBufferList>(L"MyList");
   objects_in_static_memory2.destroy<MyBufferList>(L"MyList");
   return 0;
}
```
## 堆缓冲区
```cpp
#include <boost/interprocess/containers/list.hpp>
#include <boost/interprocess/managed_heap_memory.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <cstddef>

using namespace boost::interprocess;
typedef list<int, allocator<int, managed_heap_memory::segment_manager> >
   MyList;

int main ()
{
   //We will create a buffer of 1000 bytes to store a list
   managed_heap_memory heap_memory(1000);

   MyList * mylist = heap_memory.construct<MyList>("MyList")
                        (heap_memory.get_segment_manager());

   //Obtain handle, that identifies the list in the buffer
   managed_heap_memory::handle_t list_handle = heap_memory.get_handle_from_address(mylist);

   //Fill list until there is no more memory in the buffer
   try{
      while(1) {
         mylist->insert(mylist->begin(), 0);
      }
   }
   catch(const bad_alloc &){
      //memory is full
   }
   //Let's obtain the size of the list
   MyList::size_type old_size = mylist->size();

   //To make the list bigger, let's increase the heap buffer
   //in 1000 bytes more.
   heap_memory.grow(1000);

   //If memory has been reallocated, the old pointer is invalid, so
   //use previously obtained handle to find the new pointer.
   mylist = static_cast<MyList *>
               (heap_memory.get_address_from_handle(list_handle));

   //Fill list until there is no more memory in the buffer
   try{
      while(1) {
         mylist->insert(mylist->begin(), 0);
      }
   }
   catch(const bad_alloc &){
      //memory is full
   }

   //Let's obtain the new size of the list
   MyList::size_type new_size = mylist->size();

   assert(new_size > old_size);

   //Destroy list
   heap_memory.destroy_ptr(mylist);

   return 0;
}
```
