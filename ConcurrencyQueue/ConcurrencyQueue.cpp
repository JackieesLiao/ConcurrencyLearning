// ConcurrencyQueue.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//
#include "FixStackEmptyErr.hpp"
#include "Stack.hpp"
#include "ThreadSafeQueue.hpp"
#include "ThreadSafeQueue2.hpp"
#include "ThreadSafeUltimateWithVirNode.hpp"
#include <thread>
#include <iostream>

namespace StackEmpty {
    void test_push() {
        threadsafe_stack_waitable<int> stack;
        stack.push(10);
        stack.push(20);
        
        //assert(!stack.empty());
    }

    static void testStackEmptyErr() {
        test_push();
    }

}//namespace StackEmpty

static void producer(threadsafe_queue_node<int>& q, int num_items) {
    for (std::size_t i = 0; i < num_items; i++) {
        q.push(i);
        std::cout << "Produced: " << i << "\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
}
static void consumer(threadsafe_queue_node<int>& q, int num_items) {
    for (std::size_t i = 0; i < num_items; i++) {
        int value{};
        q.wait_and_pop(value);
        std::cout << "Consumed: " << value << "\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
}
int main()
{
    //StackEmpty::testStackEmptyErr();
    threadsafe_queue_node<int> node{};
    constexpr int num_producer_items = 5;
    constexpr int num_consumer_items = 5;
    //std::ref为了达到线程同步
    std::thread producer_thread(producer, std::ref(node), num_producer_items);
    std::thread consumer_thread(consumer, std::ref(node), num_consumer_items);
    producer_thread.join();
    consumer_thread.join();
    return 0;
}

