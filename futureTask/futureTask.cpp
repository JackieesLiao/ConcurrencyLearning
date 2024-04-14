// futureTask.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <future>
#include <string>
#include <thread>
#include <chrono>
//async异步处理
static std::string fetchDataFromDB(std::string query) {
	std::this_thread::sleep_for(std::chrono::seconds(5));
	return "Data:" + query;
}

static void use_async() {
	std::future<std::string> result = std::async(std::launch::async | std::launch::deferred, fetchDataFromDB,"Data");
	auto data = result.get();
	std::cout << data << "\n";
}

static int my_task() {
	std::this_thread::sleep_for(std::chrono::seconds(5));
	std::cout << "my task run 5 s\n";
	return 43;
}

static void use_package() {
	//创建一个包装了任务的std::packaged_task对象
	std::packaged_task<int()> task(my_task);
	//获取与任务关联的std::future的对象
	std::future<int> result = task.get_future();
	//在另一个线程上执行任务
	std::thread t(std::move(task));
	//将子线程与主线程分离
	t.detach();
	//等待任务完成
	auto value = result.get();
	std::cout << "The result is：" << value << std::endl;
}

//promise 用法
//std::promise用于在某一线程中设置某个值或异常
//std::future则用于在另一个线程获取这个值或异常
static void set_value(std::promise<int> prom) {
	std::this_thread::sleep_for(std::chrono::seconds(3));
	prom.set_value(100);
	std::cout << "promise set value success\n";
}

static void use_promise() {
	//创建一个promise对象
	std::promise<int> prom;
	//获取与promise相关联的future对象
	std::future<int> result = prom.get_future();
	std::thread t1(set_value, std::move(prom));
	//在主线程中获取future的值
	std::cout << "Waiting for the thread to set the value...\n";
	//等待子线程返回值
	std::cout << "Value set by the thread:" << result.get() << "\n";
	t1.join();
}

static void set_exception(std::promise<void> prom) {
	//子线程里如果写了set_exception必须加try catch，否则会崩溃
	try {
		throw std::runtime_error("An error occurred");
	}
	catch (...) {
		prom.set_exception(std::current_exception());
	}
}
static void use_promise_exception() {
	std::promise<void> prom;
	std::future<void> fut = prom.get_future();
	std::thread t1(set_exception, std::move(prom));
	//在主线程里获取future的异常
	try {
		std::cout << "Waiting for the thread to set the exception...\n";
		fut.get();
	}
	catch (const std::exception& e) {
		std::cout << "Exception set by the thread " << e.what() << std::endl;
	}
	t1.join();
}

//共享类型的future
static void myFunction(std::promise<int>&& promise) {
	std::this_thread::sleep_for(std::chrono::seconds(2));
	promise.set_value(45);
}

static void threadFunction(std::shared_future<int> future) {
	try {
		auto result = future.get();
		std::cout << "Result : " << result << "\n";
	}
	catch (const std::future_error&e) {
		std::cout << "Error: " << e.what() << std::endl;
	}
}

static void use_shared_future() {
	std::promise<int> promise;
	//隐式转换
	std::shared_future<int> fut = promise.get_future();
	std::thread t1(myFunction, std::move(promise));
	std::thread t2(threadFunction, fut);
	std::thread t3(threadFunction, fut);

	t1.join();
	t2.join();
	t3.join();
}
//1.任务有序不要用线程池
//2.逻辑强关联不要使用
int main()
{
	//主线程中做其他事情
	std::cout << "doing something else...\n";
	//use_async();
	//use_package();
	//use_promise();
	//use_promise_exception();
	use_shared_future();
}