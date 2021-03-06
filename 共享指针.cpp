http://zh.highscore.de/cpp/boost/    boost手册

作用域指针	boost::scoped_ptr	
一个作用域指针不能传递它所包含的对象的所有权到另一个作用域指针。 

作用域数组	boost::scoped_array	
作用域数组的析构函数使用 delete[] 操作符来释放所包含的对象。

共享指针	boost::shared_ptr	
它可以和其他 boost::shared_ptr 类型的智能指针共享所有权。 在这种情况下，当引用对象的最后一个智能指针销毁后，对象才会被释放。

共享数组	boost::shared_array	
共享数组的行为类似于共享指针。 关键不同在于共享数组在析构时，默认使用 delete[] 操作符来释放所含的对象。

弱指针	boost::weak_ptr	
当函数需要一个由共享指针所管理的对象，而这个对象的生存期又不依赖于这个函数时，就可以使用弱指针。

介入式指针	boost::intrusive_ptr	 
boost::shared_ptr 在内部记录着引用到某个对象的共享指针的数量，可是对介入式指针来说，程序员就得自己来做记录。

指针容器	boost::ptr_vector 。。。	
boost::ptr_vector 独占它所包含的对象，因而容器之外的共享指针不能共享所有权，这跟 std::vector<boost::shared_ptr<int> > 相反。
