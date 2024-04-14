// Concurrency.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include "Joining_thread.h"
static void use_jointhread1()
{
    Joining_thread j1([](int maxIndex) {
        for (int i = 0; i < maxIndex; i++) {
            std::cout << "in thread id:" << std::this_thread::get_id()
                << " current index is:" << i << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    },10);

    //根据thread构造joining_thread
    Joining_thread j3(std::thread([](int maxIndex) {
        for (int i = 0; i < maxIndex; i++) {
            std::cout << "in thread id:" << std::this_thread::get_id()
                << " current index is:" << i << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }), 10);

    j1 = std::move(j3);
}
static void use_jointhread2()
{
    //根据thread构造joining_thread
    Joining_thread j2(std::thread([](int maxIndex) {
        for (int i = 0; i < maxIndex; i++) {
            std::cout << "in thread id:" << std::this_thread::get_id()
                << " current index is:" << i << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }), 10);
}
int main()
{
    use_jointhread1();
}
