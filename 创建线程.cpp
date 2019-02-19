创建线程有5种方法：
第一种 最简单方法 普通函数
第二种 复杂类型对象作为参数来创建线程：
第三种 在类内部创建线程；静态static函数
第四种 在类内部创建线程；一般成员函数
第五种 用类内部函数在类外部创建线程；

第一种 最简单方法 
void hello() 
{ 
        std::cout << 
        "Hello world, I''m a thread!" 
        << std::endl; 
} 
  
int main(int argc, char* argv[]) 
{ 
        boost::thread thrd(&hello); 
        thrd.join(); 
        return 0; 
}
第二种 复杂类型对象作为参数来创建线程： 
boost::mutex io_mutex; 
struct count 
{ 
        count(int id) : id(id) { } 
        
        void operator()() 
        { 
                for (int i = 0; i < 10; ++i) 
                { 
                        boost::mutex::scoped_lock 
                        lock(io_mutex); 
                        std::cout << id << ": " 
                        << i << std::endl; 
                } 
        } 
        
        int id; 
}; 
  
int main(int argc, char* argv[]) 
{ 
        boost::thread thrd1(count(1)); 
        boost::thread thrd2(count(2)); 
        thrd1.join(); 
        thrd2.join(); 
        return 0; 
}
第三种 在类内部创建线程；1
class HelloWorld
{
public:
 static void hello()
 {
      std::cout <<
      "Hello world, I''m a thread!"
      << std::endl;
 }
 static void start()
 {
  
  boost::thread thrd( hello );
  thrd.join();
 }
 
}; 
int main(int argc, char* argv[])
{
 HelloWorld::start();
 
 return 0;
} 
第四种 在类内部创建线程；2
class HelloWorld
{
public:
 void hello()
 {
    std::cout <<
    "Hello world, I''m a thread!"
    << std::endl;
 }
 void start()
 {
  boost::function0< void> f =  boost::bind(&HelloWorld::hello,this);
  boost::thread thrd( f );
  thrd.join();
 }
 
}; 
int main(int argc, char* argv[])
{
 HelloWorld hello;
 hello.start();
 return 0;
}
第五种 用类内部函数在类外部创建线程；
class HelloWorld
{
public:
 void hello(const std::string& str)
 {
        std::cout < }
}; 
  
int main(int argc, char* argv[])
{ 
 HelloWorld obj;
 boost::thread thrd( boost::bind(&HelloWorld::hello,&obj,"Hello 
                               world, I''m a thread!" ) ) ;
 thrd.join();
 return 0;
}
