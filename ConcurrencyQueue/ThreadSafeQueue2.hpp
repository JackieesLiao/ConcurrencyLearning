#ifndef THREADSAFEQUEUE_H
#define THREADSAFEQUEUE_H
#include <condition_variable>
#include <mutex>
#include <queue>
/*线程安全的队列--修复之前的可能会阻塞数据在队列中的异常*/
template<typename T>
class threadsafe_queue_ptr {
	mutable std::mutex mutex_;
	std::queue<std::shared_ptr<T>> data_queue_;
	std::condition_variable cv;
public:
	threadsafe_queue_ptr() {}

	void wait_and_pop(T& value) {               
		std::unique_lock<std::mutex> lock(mutex_);
		cv.wait(lock, [this]() {
			return !data_queue_.empty();         /*如果队列为空则阻塞直到push进了数据*/
			});
		value = std::move(*data_queue_.front());  //(1)
		data_queue_.pop();
	}

	bool try_pop(T& value)
	{
		std::lock_guard<std::mutex> lock(mutex_);
		if (data_queue_.empty())
			return false;
		value = std::move(*data_queue_.front());  //(2)
		data_queue_.pop();
		return true;
	}
	
	std::shared_ptr<T> wait_and_pop() {
		std::unique_lock<std::mutex> lock(mutex_);
		cv.wait(lock, [this]() {
			return !data_queue_.empty();
			});

		std::shared_ptr<T> res = data_queue_.front();//(3)
		data_queue_.pop();
		return res;
	}

	std::shared_ptr<T> try_pop()
	{
		std::lock_guard<std::mutex> lock(mutex_);
		if (data_queue_.empty())
			return std::shared_ptr<T>();   
		std::shared_ptr<T> res = data_queue_.front()//(4)
		data_queue_.pop();
		return res;
	}
	//push前首先构造一个智能指针，构造失败了也不会push到队列中
	void push(T new_value) {
		std::shared_ptr<T> 
			data(std::make_shared<T>(std::move(new_value)));//(5)
		std::lock_guard<std::mutex> lock(mutex_);
		data_queue_.push(data);
		cv.notify_one();                        
	}
	bool empty() const
	{
		std::unique_lock<std::mutex> lock(mutex_);
		return data_queue_.empty();
	}
};
#endif // !THREADSAFEQUEUE_H