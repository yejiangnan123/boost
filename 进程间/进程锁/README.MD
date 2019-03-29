# 进程锁
## named_mutex有名互斥量
```cpp
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <fstream>
#include <iostream>
#include <cstdio>


int main ()
{
   using namespace boost::interprocess;
   try{
      struct file_remove
      {
         file_remove() { std::remove("file_name"); }
         ~file_remove(){ std::remove("file_name"); }
      } file_remover;
      struct mutex_remove
      {
         mutex_remove() { named_mutex::remove("fstream_named_mutex"); }
         ~mutex_remove(){ named_mutex::remove("fstream_named_mutex"); }
      } remover;

      //Open or create the named mutex
      named_mutex mutex(open_or_create, "fstream_named_mutex");

      std::ofstream file("file_name");

      for(int i = 0; i < 10; ++i){

         //Do some operations...

         //Write to file atomically
         scoped_lock<named_mutex> lock(mutex);
         file << "Process name, ";
         file << "This is iteration #" << i;
         file << std::endl;
      }
   }
   catch(interprocess_exception &ex){
      std::cout << ex.what() << std::endl;
      return 1;
   }
   return 0;
}
```
