#ifndef THREADSAFEQUEUE_H
#define THREADSAFEQUEUE_H
#include <condition_variable>
#include <mutex>
#include <queue>
/*线程安全的队列*/
template<typename T>
class threadsafe_queue {
	mutable std::mutex mutex_;
	std::queue<T> data_queue_;
	std::condition_variable cv;
public:
	threadsafe_queue() {}

	void push(T new_value) {
		std::lock_guard<std::mutex> lock(mutex_);
		data_queue_.push(std::move(new_value));
		cv.notify_one();                        //(1)
	}

	void wait_and_pop(T& value) {               //(2)
		std::unique_lock<std::mutex> lock(mutex_);
		cv.wait(lock, [this]() {
			return !data_queue_.empty();        /*如果队列为空则阻塞直到push进了数据*/
		});
		value = std::move(data_queue_.front());
		data_queue_.pop();
	}
	/*如果等待期间抛出异常则不会pop出队列中的数据从而导致其他线程也不会被唤醒*/
	std::shared_ptr<T> wait_and_pop() {
		std::unique_lock<std::mutex> lock(mutex_);
		cv.wait(lock, [this]() {
			return !data_queue_.empty();
		});

		std::shared_ptr<T> res(std::make_shared<T>(std::move(data_queue_.front())));
		data_queue_.pop();
		return res;
	}
	bool try_pop(T& value)
	{
		std::unique_lock<std::mutex> lock(mutex_);
		if (data_queue_.empty())
			return false;
		value = std::move(data_queue_.front());
		data_queue_.pop();
		return true;
	}
	std::shared_ptr<T> try_pop()
	{
		std::unique_lock<std::mutex> lock(mutex_);
		if (data_queue_.empty())
			return std::shared_ptr<T>();    //⇽-- - ⑤
		std::shared_ptr<T> res(
			std::make_shared<T>(std::move(data_queue_.front())));
		data_queue_.pop();
		return res;
	}
	bool empty() const
	{
		std::unique_lock<std::mutex> lock(mutex_);
		return data_queue_.empty();
	}
};
#endif // !THREADSAFEQUEUE_H