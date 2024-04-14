// CSP.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <condition_variable>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>
//并发设计模式CSP
template<typename T>
class Channel {
public:
    Channel(std::size_t capacity = 0) :capacity(capacity) {}
    //生产者往channel中放数据
    bool send(T value) {
        std::unique_lock<std::mutex> lock(mtx_);
        cv_producer_.wait(lock, [this]() {
            //对于无缓冲的channel，应该等待直到有消费者准备好
            return (capacity == 0 && que_.empty() || que_.size() < capacity || closed)
            });

        //如果发出关闭信号则停止发送
        if (closed) {
            return false;
        }

        que_.push(value);
        cv_consumer_.notify_one();
        return true;
    }

    //消费者往channel中取数据
    bool receive(T& value) {
        std::unique_lock<std::mutex> lock(mtx_);
        cv_consumer_.wait(lock, [this]() {
            return !que_.empty() || closed;//如果channel为空或者已经关闭则阻塞
            });

        if (que_.empty() && closed) {
            return false;
        }

        value = que_.front();
        que_.pop();
        //通知生产者可以继续往channel中生产数据
        cv_producer_.notify_one();
        return true;
    }
    //关闭操作
    void close() {
        std::unique_lock<std::mutex> lock(mtx_);
        closed = true;
        //通知所有生产者和消费者将要退出关闭
        cv_producer_.notify_one();
        cv_consumer_.notify_one();
    }
private:
    std::queue<T> que_;
    std::mutex mtx_;
    //往channel中生产数据的生产者
    std::condition_variable cv_producer_;
    //从channel中消费数据的消费 者
    std::condition_variable cv_consumer_;
    std::size_t capacity;
    bool closed = false;

};

static void test_consumer_producer() {
    Channel<int> ch(10);
    std::thread producer([&]() {
        for (int i = 0; i < 5; i++) {
            ch.send(i);
            std::cout << "Have send: " << i << "\n";
        }
        ch.close();
    });
    std::thread consumer([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(600));
        int val{};
        while (ch.receive(val)) {
            std::cout << "Received : " << val << std::endl;
        }
    });
    producer.join();
    consumer.join();
}
int main()
{
    test_consumer_producer();
}