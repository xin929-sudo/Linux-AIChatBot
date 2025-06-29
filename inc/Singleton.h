#ifndef _SIGLETON_H
#define _SIGLETON_H
#include <memory>
#include<iostream>
#include<mutex>
template <typename T>
class Singleton{
    protected: // 由于是继承，所以子类在调用构造函数的时候，会调用父类的构造，所以需要protected，而不是private。 
        Singleton() = default;
        Singleton(const Singleton<T>&) =delete;
        Singleton& operator=(const Singleton<T>& st) = delete;
        // 静态成员变量只能在全局作用域下初始化
        static std::shared_ptr<T> _instance;
    public:
       
        ~Singleton(){
            
            std::cout<<"this is singleton destruct "<<  _instance.get() <<  std::endl;
        }
        static std::shared_ptr<T> GetInstance(){
            //防止多线程环境下多次初始化 _instance
            //使用std::call_once,确保代码初始化仅执行一次，代替显示锁
            static std::once_flag s_flag;
            std::call_once(s_flag,[&](){
                _instance = std::shared_ptr<T>(new T);
            });
            return _instance;
        }
        void PrintAddress(){
            std::cout<< _instance.get() <<std::endl;
        }
};

template <typename T>
std::shared_ptr<T> Singleton<T>::_instance = nullptr;






#endif