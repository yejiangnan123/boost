------------------------
Boost.Bind   绑定函数和参数
Boost.Ref     指定参数为引用传递
Boost.Function  函数指针
Boost.Lambda   匿名函数
-------------------------

Boost.Bind-----------------
#include <boost/bind.hpp> 
#include <iostream> 
#include <vector> 
#include <algorithm> 

void add(int i, int j) 
{ 
  std::cout << i + j << std::endl; 
} 

int main() 
{ 
  std::vector<int> v; 
  v.push_back(1); 
  v.push_back(3); 
  v.push_back(2); 

  std::for_each(v.begin(), v.end(), boost::bind(add, 10, _1)); 
} 

Boost.Ref-----------------------
要以引用方式传递常量对象，可以使用模板函数 boost::cref()。
#include <boost/bind.hpp> 
#include <iostream> 
#include <vector> 
#include <algorithm> 

void add(int i, int j, std::ostream &os) 
{ 
  os << i + j << std::endl; 
} 

int main() 
{ 
  std::vector<int> v; 
  v.push_back(1); 
  v.push_back(3); 
  v.push_back(2); 

  std::for_each(v.begin(), v.end(), boost::bind(add, 10, _1, boost::ref(std::cout))); 
}

Boost.Function--------------------------------
普通函数指针
#include <boost/function.hpp> 
#include <iostream> 
#include <cstdlib> 
#include <cstring> 

int main() 
{ 
  boost::function<int (const char*)> f = std::atoi; 
  std::cout << f("1609") << std::endl; 
  f = std::strlen; 
  std::cout << f("1609") << std::endl; 
} 

通过使用 Boost.Function，类成员函数也可以被赋值给类型为 boost::function 的对象。
#include <boost/function.hpp> 
#include <iostream> 

struct world 
{ 
  void hello(std::ostream &os) 
  { 
    os << "Hello, world!" << std::endl; 
  } 
}; 

int main() 
{ 
  boost::function<void (world*, std::ostream&)> f = &world::hello; 
  world w; 
  f(&w, boost::ref(std::cout)); 
} 

Boost.Lambda-------------------
匿名函数
#include <boost/lambda/lambda.hpp> 
#include <iostream> 
#include <vector> 
#include <algorithm> 

int main() 
{ 
  std::vector<int> v; 
  v.push_back(1); 
  v.push_back(3); 
  v.push_back(2); 

  std::for_each(v.begin(), v.end(), std::cout << boost::lambda::_1 << "\n"); 
}
