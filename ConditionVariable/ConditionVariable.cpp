// ConditionVariable.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <condition_variable>
#include <iostream>
#include <memory>
#include <queue>
#include <string>
#include <thread>
int globalNum = 1;
std::mutex mtxNum;
//条件变量
std::condition_variable cvA;
std::condition_variable cvB;

static void conditionTest() {
	std::thread t1([]() {
		for (;;) {
			std::unique_lock<std::mutex> lock(mtxNum);
			cvA.wait(lock, []() {
				//如果num不等于1 就一直阻塞在这里
				return globalNum == 1;
				});
			std::cout << "thread A print " << globalNum << "...\n";;
			globalNum++;
			cvB.notify_one();//唤醒
		}
		});
	std::thread t2([]() {
		for (;;) {
			std::unique_lock<std::mutex> lock(mtxNum);
			cvB.wait(lock, []() {
				//如果num不等于2 就一直阻塞在这里
				return globalNum == 2;
				});
			std::cout << "thread B print " << globalNum << "...\n";;
			globalNum--;
			cvA.notify_one();//唤醒
		}
		});

	t1.join();
	t2.join();
}
//利用条件变量实现线程安全队列
template<typename T>
class threadsafe_queue {
public:
	threadsafe_queue(){}
	threadsafe_queue(const threadsafe_queue& other) {
		std::lock_guard<std::mutex> lock(other.mtx);
		data_queue = other.data_queue;
	}
	void push(T new_val) {
		std::lock_guard<std::mutex> lock(mtx);
		data_queue.push(new_val);
		data_condi.notify_all(); //有数据之后唤醒pop
	}
	//如果队列为空一直等待，直到队列中有数据
	void wait_and_pop(T& value) {
		std::unique_lock<std::mutex> lock(mtx);
		data_condi.wait(lock, [this]() {
			return !data_queue.empty();
			});
		value = data_queue.front();
		data_queue.pop();
	}

	std::shared_ptr<T> wait_and_pop() {
		std::unique_lock<std::mutex> lock(mtx);
		data_condi.wait(lock, [this]() {
			return !data_queue.empty();
			});
		std::shared_ptr<T> res{ std::make_shared<T>(data_queue.front()) };
		data_queue.pop();
		return res;
	}

	bool try_pop(T& value) {
		std::lock_guard<std::mutex> lock(mtx);
		if (data_queue.empty()) {
			return false;
		}
		value = data_queue.front();
		data_queue.pop();
		return true;
	}

	std::shared_ptr<T> try_pop() {
		std::lock_guard<std::mutex> lock(mtx);
		if (data_queue.empty()) {
			return std::shared_ptr<T>();
		}
		std::shared_ptr<T> res{ std::make_shared<T>(data_queue.front()) };
		data_queue.pop();
		return res;
	}

	//判空操作
	bool empty() const {
		std::lock_guard<std::mutex> lock(mtx);
		return data_queue.empty();
	}
private:
	mutable std::mutex mtx;
	std::queue<T> data_queue;
	std::condition_variable data_condi;
};

static void testSafeQueue() {
	threadsafe_queue<int> queue;
	std::mutex mtx;
	std::thread producer(
		[&]() {
			for (int i{ 0 };; i++) {
				queue.push(i);
				{
					std::lock_guard<std::mutex> lock(mtx);
					std::cout << "producer push data is\t" << i << std::endl;
				}
				std::this_thread::sleep_for(std::chrono::milliseconds(200));
			}
		}
	);

	std::thread consumer1(
		[&]() {
			for (;;) {
				auto data = queue.wait_and_pop();
				if (data != nullptr)
				{
					std::lock_guard<std::mutex> lock(mtx);
					std::cout << "consumer1 wait and pop data is\t" << *data << std::endl;
				}
				std::this_thread::sleep_for(std::chrono::milliseconds(400));
			}
		}
	);

	std::thread consumer2(
		[&]() {
			for (;;) {
				auto data = queue.try_pop();
				if (data != nullptr)
				{
					std::lock_guard<std::mutex> lock(mtx);
					std::cout << "consumer2 try_pop data is\t" << *data << std::endl;
				}
				std::this_thread::sleep_for(std::chrono::milliseconds(400));
			}
		}
	);
	producer.join();
	consumer1.join();
	consumer2.join();
}
int main()
{
	//conditionTest();
	testSafeQueue();
}