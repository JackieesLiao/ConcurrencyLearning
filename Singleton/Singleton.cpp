// Singleton.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <memory>
#include <mutex>
#include <thread>
class Single2 {
public:
    //局部静态变量创建单例类
    static Single2& GetInstance() {
        static Single2 ins;
        return ins;
    }
   
private:
    Single2(){} 
    Single2(const Single2&) = delete;
    Single2& operator= (const Single2&) = delete;
};

static void testSingle2() {
    std::cout << "addr1 is :" <<  & Single2::GetInstance() << "\n";
    std::cout << "addr2 is :" <<  & Single2::GetInstance() << "\n";
}

//懒汉式初始化
//需要加锁，防止资源重复利用
//缺点：回收指针资源困难
class SinglePointer {
public:
    static SinglePointer* GetIns() {
        if (single != nullptr) {
            return single;
        }

        mtx.lock();

        if (single != nullptr) {
            mtx.unlock();
            return single;
        }

        single = new SinglePointer;
        mtx.unlock();
        return single;
    }

private:
    SinglePointer() {}
    SinglePointer(const SinglePointer&) = delete;
    SinglePointer& operator= (const SinglePointer&) = delete;
private:
    static SinglePointer* single;
    static std::mutex mtx;
};

//懒汉式初始化智能指针优化
//需要加锁，防止资源重复利用
class SingleAutoPointer {
public:
    static std::shared_ptr<SingleAutoPointer> GetIns() {
        if (single != nullptr) {
            return single;
        }

        mtx.lock();

        if (single != nullptr) {
            mtx.unlock();
            return single;
        }

        single = std::shared_ptr<SingleAutoPointer>(new SingleAutoPointer);
        mtx.unlock();
        return single;
    }
    ~SingleAutoPointer(){}
private:
    SingleAutoPointer() {}
    SingleAutoPointer(const SingleAutoPointer&) = delete;
    SingleAutoPointer& operator= (const SingleAutoPointer&) = delete;
private:
    static std::shared_ptr<SingleAutoPointer> single;
    static std::mutex mtx;
};

SinglePointer* SinglePointer::single = nullptr;
std::mutex SinglePointer::mtx;
static void testSinglePointerLazy(int i) {
    std::cout << "this is lazy thread " << i << std::endl;
    std::cout << "inst is " << SinglePointer::GetIns() << std::endl;
}
static void testSingleLazy() {
    for (int i = 0; i < 3; i++) {
        std::thread tid(testSinglePointerLazy, i);
        tid.join();
    }
}
std::shared_ptr< SingleAutoPointer> SingleAutoPointer::single = nullptr;
std::mutex SingleAutoPointer::mtx;
static void testAutoSingleLazy() {
    auto sp1 = SingleAutoPointer::GetIns();
    auto sp2 = SingleAutoPointer::GetIns();
    std::cout << "sp1  is  " << sp1 << std::endl;
    std::cout << "sp2  is  " << sp2 << std::endl;
}

template<typename T>
class Singleton
{
public:
    static std::shared_ptr<T> GetInStance()
    {
        static std::once_flag s_flag;
        std::call_once(s_flag, [&]() {
            instance = std::shared_ptr<T>(new T);
            });
        return instance;
    }
    void PrintAddr() {
        std::cout << instance.get() << "\n";
    }
    ~Singleton() {
        std::cout << "this is singleton distruct\n";
    }
private:
    static std::shared_ptr<T> instance;
    Singleton() = default;
    Singleton(const Singleton<T>&) = delete;
    Singleton& operator= (const Singleton<T>&) = delete;
};
template<typename T>
std::shared_ptr<T> Singleton<T>::instance = nullptr;

class Config {
public:
    Config(int data):data(data){}
    Config(Config& config):data(config.data){}
    Config& operator= (const Config& other) {
        data = other.data;
        return *this;
    }
private:
    int data;
};
using sConfig = Singleton<Config>;
int main()
{
   // testSingle2();
   // testSingleLazy();
   // testAutoSingleLazy();
   
}