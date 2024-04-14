// lockmutex.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <mutex>
#include <memory>
#include <stack>
#include <thread>
#include <utility>
#include <vector>
int shardData{ 0 };
std::mutex mtx;
static void lock()
{
	while (true) {
		//这是手动加锁
		mtx.lock();
		shardData++;
		std::cout << "current thread is " << std::this_thread::get_id() << "\n";
		std::cout << "Shared data is " << shardData << "\n";
		mtx.unlock();
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
}

template <typename T>
class threadSafeStack1 {
public:
	threadSafeStack1(){}
	threadSafeStack1(const threadSafeStack1& other) {
		std::lock_guard<std::mutex> lock(other.mtx);
		data = other.data;
	}
	//为了避免复制锁，删除拷贝赋值
	threadSafeStack1& operator= (const threadSafeStack1&) = delete;
	
	void push(T newValue) {
		std::lock_guard<std::mutex> lock(mtx);
		data.push(std::move(newValue));
	}

	T pop() {
		std::lock_guard<std::mutex> lock(mtx);
		auto elem = data.top();
		data.pop();
		return elem;
	}

	bool empty() const {
		std::lock_guard<std::mutex> lock(mtx);
		return data.empty();
	}
private:
	std::stack<T> data;
	mutable std::mutex mtx;
};

struct emptyStack : std::exception
{
	const char* what() const throw();
};

template <typename T>
class threadSafeStack {
public:
	threadSafeStack() {}
	threadSafeStack(const threadSafeStack& other) {
		std::lock_guard<std::mutex> lock(other.mtx);
		data = other.data;
	}
	//为了避免复制锁，删除拷贝赋值
	threadSafeStack& operator= (const threadSafeStack&) = delete;

	void push(T newValue) {
		std::lock_guard<std::mutex> lock(mtx);
		data.push(std::move(newValue));
	}

	std::shared_ptr<T> pop() {
		std::lock_guard<std::mutex> lock(mtx);
		if (data.empty()) {
			return nullptr;
		}
		std::shared_ptr<T> const res(std::make_shared<T>(data.top()));
		data.pop();
		return res;
	}

	void pop(T& value) {
		std::lock_guard<std::mutex> lock(mtx);
		if (data.empty()) throw emptyStack();
		value = data.top();
		data.pop();
	}

	bool empty() const {
		std::lock_guard<std::mutex> lock(mtx);
		return data.empty();
	}
private:
	std::stack<T> data;
	mutable std::mutex mtx;
};

static void testSafe() {
	threadSafeStack1<int> safeStack;
	safeStack.push(3);

	std::thread t1([&safeStack]() {
		if (!safeStack.empty()) {
			std::this_thread::sleep_for(std::chrono::seconds(1));
			safeStack.pop();
		}
    });
	std::thread t2([&safeStack]() {
		if (!safeStack.empty()) {
			std::this_thread::sleep_for(std::chrono::seconds(1));
			safeStack.pop();
		}
    });

	t1.join();
	t2.join();
}

static void testSafeStack() {
	threadSafeStack<int> safeStack;
	safeStack.push(4);

	std::thread t1([&safeStack]() {
		if (!safeStack.empty()) {
			std::this_thread::sleep_for(std::chrono::seconds(1));
			safeStack.pop();
		}
		});
	std::thread t2([&safeStack]() {
		if (!safeStack.empty()) {
			std::this_thread::sleep_for(std::chrono::seconds(1));
			safeStack.pop();
		}
		});

	t1.join();
	t2.join();
}
//死锁如何造成的？
//

std::mutex t_lock1;
std::mutex t_lock2;
int m_1 = 0;
int m_2 = 0;

static void deadLock1()
{
	while (true) {
		std::cout << "dead_lock 1 begin() " << std::endl;
		t_lock1.lock();
		m_1 = 1024;
		t_lock2.lock();
		m_2 = 1024;
		t_lock2.unlock();
		t_lock1.unlock();
		std::cout << "dead_lock 1 end() " << std::endl;
	}
}
static void deadLock2()
{
	while (true) {
		std::cout << "dead_lock 2 begin() " << std::endl;
		t_lock2.lock();
		m_1 = 1024;
		t_lock1.lock();
		m_2 = 1024;
		t_lock1.unlock();
		t_lock2.unlock();
		std::cout << "dead_lock 2 end() " << std::endl;
	}
}
static void testDeadLock()
{
	std::thread t1(deadLock1);
	std::thread(deadLock2).join();
	t1.join();
}

static void atomic_lock1()
{
	std::cout << "lock1 begin lock \n";
	t_lock1.lock();
	m_1 = 1024;
	t_lock1.unlock();
	std::cout << "lock1 end lock \n";
}

static void atomic_lock2()
{
	std::cout << "lock2 begin lock \n";
	t_lock2.lock();
	m_2 = 1024;
	t_lock2.unlock();
	std::cout << "lock2 end lock \n";
}

static void safeLock1()
{
	while (true) {
		atomic_lock1();
		atomic_lock2();
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
}

static void safeLock2()
{
	while (true) {
		atomic_lock2();
		atomic_lock1();
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
}

static void testSafeLock()
{
	std::thread t1(safeLock1);
	std::thread t2(safeLock2);
	t1.join();
	t2.join();
}

//对于要使用两个互斥量，可以同时加锁
//如不同时加锁可能会死锁
class som_big_obj {
public:
	som_big_obj(int data):data_(data){}
	//拷贝构造
	som_big_obj(const som_big_obj& other)
		:data_(other.data_){}
	//移动构造
	som_big_obj(som_big_obj&& other) noexcept
		:data_(std::move(other.data_)){}

	//重载输出运算符
	friend static std::ostream& operator << (std::ostream& os, const som_big_obj& big_obj)
	{
		os << big_obj.data_;
		return os;
	}

	//重载赋值运算符
	som_big_obj& operator = (const som_big_obj& other) {
		if (this == &other) {
			return *this;
		}
		data_ = other.data_;
		return *this;
	}
	//如果定义了拷贝赋值并且未定义移动赋值运算符
	//则std::move的时候编译器默认走拷贝赋值运算符
	//否则走移动赋值运算符
	som_big_obj& operator = (som_big_obj&& other) noexcept{
		data_ = std::move(other.data_);
		return *this;
	}

	//交换数据
	static friend void swap(som_big_obj& b1, som_big_obj& b2) {
		som_big_obj temp = std::move(b1);
		b1 = std::move(b2);
		b2 = std::move(temp);
	}
private:
	int data_;
};

class big_object_mgr {
public:
	big_object_mgr(int data = 0):obj_(data){}
	void printInfo() const{
		std::cout << "current obj data is " << obj_ << "\n";
	}
	static friend void danger_swap(big_object_mgr& objm1, big_object_mgr& objm2);
	static friend void safe_swap(big_object_mgr& objm1, big_object_mgr& objm2);
	static friend void safe_swap_scope(big_object_mgr& objm1, big_object_mgr& objm2);

private:
	std::mutex mtx_;  //互斥量
	som_big_obj obj_;
};

static void danger_swap(big_object_mgr& objm1, big_object_mgr& objm2)
{
	std::cout << "thread [" << std::this_thread::get_id() << "] begin\n";
	if (&objm1 == &objm2) {
		return;
	}
	std::lock_guard<std::mutex> guard1(objm1.mtx_);
	std::this_thread::sleep_for(std::chrono::seconds(1));
	std::lock_guard<std::mutex> guard2(objm2.mtx_);
	swap(objm1.obj_, objm2.obj_);
	std::cout << "thread [" << std::this_thread::get_id() << "] end\n";
}
static void safe_swap(big_object_mgr& objm1, big_object_mgr& objm2)
{
	std::cout << "thread [" << std::this_thread::get_id() << "] begin\n";
	if (&objm1 == &objm2) {
		return;
	}
	std::lock(objm1.mtx_, objm2.mtx_);
	//领养锁管理互斥量，只负责锁的解锁
	std::lock_guard<std::mutex> guard1(objm1.mtx_, std::adopt_lock);
	std::this_thread::sleep_for(std::chrono::seconds(1));
	std::lock_guard<std::mutex> guard2(objm2.mtx_, std::adopt_lock);
	swap(objm1.obj_, objm2.obj_);
	std::cout << "thread [" << std::this_thread::get_id() << "] end\n";
}
static void safe_swap_scope(big_object_mgr& objm1, big_object_mgr& objm2)
{
	std::cout << "thread [" << std::this_thread::get_id() << "] begin\n";
	if (&objm1 == &objm2) {
		return;
	}
	
	std::scoped_lock guard(objm1.mtx_, objm2.mtx_);

	swap(objm1.obj_, objm2.obj_);
	std::cout << "thread [" << std::this_thread::get_id() << "] end\n";
}

static void testDangerSwap()
{
	big_object_mgr objm1(5);
	big_object_mgr objm2(15);
	std::thread t1(danger_swap, std::ref(objm1), std::ref(objm2));
	std::thread t2(danger_swap, std::ref(objm2), std::ref(objm1));
	t1.join();
	t2.join();
	objm1.printInfo();
	objm2.printInfo();
}
namespace lockTests {
	static void testSafeSwap()
	{
		big_object_mgr objm1(5);
		big_object_mgr objm2(15);
		//两次交换等于没交换
		std::thread t1(safe_swap, std::ref(objm1), std::ref(objm2));
		std::thread t2(safe_swap, std::ref(objm2), std::ref(objm1));
		t1.join();
		t2.join();
		objm1.printInfo();
		objm2.printInfo();
	}

	static void testSafeSwapScope()
	{
		big_object_mgr objm1(5);
		big_object_mgr objm2(15);
		//两次交换等于没交换
		std::thread t1(safe_swap, std::ref(objm1), std::ref(objm2));
		std::thread t2(safe_swap, std::ref(objm2), std::ref(objm1));
		t1.join();
		t2.join();
		objm1.printInfo();
		objm2.printInfo();
	}
}//namespace lockTests


//层级锁
//
class hierarchical_mutex {
public:
	explicit hierarchical_mutex(unsigned long value):
		hierarchy_value(value), previous_hierarchy_value(0) {}

	//禁止对锁进行拷贝和赋值
	hierarchical_mutex(const hierarchical_mutex&) = delete;
	hierarchical_mutex& operator=(const hierarchical_mutex&) = delete;

	//加锁
	void lock() {
		//首先检查要加锁的层级值
		check_for_hierarchy_value();
		//如果需要加锁的层级值无问题
		mutex_.lock();
		//更新层级值
		update_hierarchy_value();
	}

	void check_for_hierarchy_value() const{
		if (this_thread_hierarchy_value <= hierarchy_value) {
			throw std::logic_error("mutex hierarchy violated.");
		}
	}

	void update_hierarchy_value() {
		previous_hierarchy_value = this_thread_hierarchy_value;
		this_thread_hierarchy_value = hierarchy_value;
	}

	void unlock() {
		// 如果层级值不等说明加的不是同一把锁
		if (this_thread_hierarchy_value != hierarchy_value) {
			throw std::logic_error("mutex hierarchy violated.");
		}

		this_thread_hierarchy_value = previous_hierarchy_value;
		mutex_.unlock();
	}

	bool tryLock() {
		check_for_hierarchy_value();
		if (!mutex_.try_lock()) {
			return false;
		}

		update_hierarchy_value();
		return true;
	}
private:
	std::mutex mutex_;
	unsigned long const hierarchy_value;   //当前层级值
	unsigned long previous_hierarchy_value;//上一级层级值
	//本线程记录的层级值
	static thread_local unsigned long this_thread_hierarchy_value;
};
thread_local unsigned long hierarchical_mutex::this_thread_hierarchy_value(ULONG_MAX);
static void testHierarchy()
{
	hierarchical_mutex hmtx1(1000);
	hierarchical_mutex hmtx2(500);
	std::thread t1([&hmtx1, &hmtx2]() {
		hmtx1.lock();
		hmtx2.lock();
		hmtx2.unlock();
		hmtx1.unlock();
	});
	std::thread t2([&hmtx1, &hmtx2]() {
		hmtx2.lock();
		hmtx1.lock();
		hmtx1.unlock();
		hmtx2.unlock();
	});
	t1.join();
	t2.join();
}
int main()
{
#if 0
	std::thread t1(lock);
	std::thread t2([]() {
		while (true) {
			//使用lock_guard进行自动加锁
			std::lock_guard<std::mutex> lock(mtx);
			shardData--;
			std::cout << "current thread is " << std::this_thread::get_id() << "\n";
			std::cout << "Shared data is " << shardData << "\n";
			std::this_thread::sleep_for(std::chrono::seconds(1));
		}
		
	});
	t1.join();
	t2.join();

	//testSafe();
	//testSafeStack();
	//testDeadLock();

	/*testSafeLock();
	som_big_obj bigObj1(100);
	som_big_obj bigObj2(200);*/
#endif
	//testDangerSwap();
	//testSafeSwap();
	//lockTests::testSafeSwapScope();
	testHierarchy();
}