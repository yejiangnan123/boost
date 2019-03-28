# boost 序列化
* [csdn原文](https://blog.csdn.net/chenaqiao/article/details/48371597)
* [boost](https://www.boost.org/doc/libs/1_51_0/libs/serialization/doc/tutorial.html)
## 侵入式
```
在要序列化的类中增加一个友元类，然后在内部序列化数据成员即可
friend class boost::serialization::access;  
  
    template<class Archive>  
    void serialize(Archive& ar, const unsigned int version)  
    {  
        ar & _tag;  
        ar & _text;  
    }  
```
## 非侵入式
```
在类的外部进行序列化，不修改原来的类就可以序列化，但是要序列化的数据成员必须是public，在外部序列化需要访问类的数据成员
namespace boost {  
    namespace serialization {  
  
        template<class Archive>  
        void serialize(Archive & ar, CMyData2 & d, const unsigned int version)  
        {  
            ar & d._tag;  
            ar & d._text;  
        }  
  
    } // namespace serialization  
} // namespace boost
```
## 序列化和反序列化的使用
```
void TestArchive1()  
{  
    CMyData d1(2012, "China, good luck");  
    std::ostringstream os;  
    boost::archive::binary_oarchive oa(os);  
    oa << d1;//序列化到一个ostringstream里面  
  
    std::string content = os.str();//content保存了序列化后的数据。  
  
    CMyData d2;  
    std::istringstream is(content);  
    boost::archive::binary_iarchive ia(is);  
    ia >> d2;//从一个保存序列化数据的string里面反序列化，从而得到原来的对象。  
  
    std::cout << "CMyData tag: " << d2.GetTag() << ", text: " << d2.GetText() << "\n";  
}  
```
## 可以序列化继承类
```
先序列化基类，再序列化子类的数据成员
namespace boost {  
    namespace serialization {  
  
        template<class Archive>  
        void serialize(Archive & ar, CMyData2_Child & d, const unsigned int version)  
        {  
            // serialize base class information  
            ar & boost::serialization::base_object<CMyData2>(d);  
            ar & d._number;  
        }  
  
    } // namespace serialization  
} // namespace boost  
```
## 序列化多态，多态类指针序列化需要特殊处理
```
BOOST_CLASS_EXPORT_GUID(CMyData_Child, "CMyData_Child")  
  
class CMyData_Container  
{  
private:  
    friend class boost::serialization::access;  
  
    template<class Archive>  
    void serialize(Archive& ar, const unsigned int version)  
    {  
        for(int i = 0; i < 3; i++)  
        {  
            ar & pointers[i];  
        }  
    }  
public:  
    CMyData* pointers[3];  
};  
```
## 序列化和反序列化可以分开
```
save和load分开
#include <boost/serialization/list.hpp>  
#include <boost/serialization/string.hpp>  
#include <boost/serialization/version.hpp>  
#include <boost/serialization/split_member.hpp>  
  
class bus_route  
{  
    friend class boost::serialization::access;  
    std::list<bus_stop *> stops;  
    std::string driver_name;  
    template<class Archive>  
    void save(Archive & ar, const unsigned int version) const  
    {  
        // note, version is always the latest when saving  
        ar  & driver_name;  
        ar  & stops;  
    }  
    template<class Archive>  
    void load(Archive & ar, const unsigned int version)  
    {  
        if(version > 0)  
            ar & driver_name;  
        ar  & stops;  
    }  
    BOOST_SERIALIZATION_SPLIT_MEMBER()  
public:  
    bus_route(){}  
};  
  
BOOST_CLASS_VERSION(bus_route, 1)  
```
## 以下为容器实例测试
```
root@ubuntu:~/boost-test/serialization# cat main.cpp 

#include <cstddef> // NULL
#include <iomanip>
#include <iostream>
#include <fstream>
#include <string>

#include <boost/archive/tmpdir.hpp>

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

#include <boost/serialization/base_object.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/serialization/list.hpp>
#include <boost/serialization/assume_abstract.hpp>
using namespace std;
class bus_route
{
public:
    friend class boost::serialization::access;
    std::list<std::string> stops;
    std::string name;
    template<class Archive>
    void serialize(Archive &ar, const unsigned int version)
    {
        ar & name;
        ar & stops;
    }
public:
    bus_route(){}
    void print()
    {
        cout << name;
        for(auto it = stops.begin(); it != stops.end(); it++){
            std::cout << " " << *it;
        }
        cout << "\n";
    }
};

int main(int argc, char *argv[])
{
    bus_route bus1,bus2;
    bus1.name = "bus1";
    bus1.stops.push_back("a");
    bus1.stops.push_back("b");
    bus1.stops.push_back("c");
    std::ostringstream os;
    boost::archive::text_oarchive oa(os);
    oa << bus1;
    std::string content = os.str();
    std::istringstream is(content);
    boost::archive::text_iarchive ia(is);
    ia >> bus2;
    bus1.print();
    bus2.print();
    return 0;
}
```
## cmake 配置需要添加 serialization 库
```
root@ubuntu:~/boost-test/serialization# cat CMakeLists.txt 
cmake_minimum_required(VERSION 2.8.3)
project(test)
find_package(Boost REQUIRED COMPONENTS system serialization thread)
include_directories(${Boost_INCLUDE_DIRS})
AUX_SOURCE_DIRECTORY(. SRC_DIR)
add_executable(test ${SRC_DIR} )
target_link_libraries(test ${Boost_LIBRARIES} pthread)
```
#测试结果
```
root@ubuntu:~/boost-test/serialization# ./test 
bus1 a b c
bus1 a b c
```
