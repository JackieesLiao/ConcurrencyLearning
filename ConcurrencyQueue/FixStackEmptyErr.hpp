#ifndef STACKPOPNOERROR_H
#define STACKPOPNOERROR_H
#include <condition_variable>
#include <mutex>
#include <stack>
#include <utility>
template <typename T>
class threadsafe_stack_waitable {
	std::stack<T> data_;
	mutable std::mutex mutex_;
	std::condition_variable cv;
public:
	threadsafe_stack_waitable(){}
	threadsafe_stack_waitable(const threadsafe_stack_waitable& stack) {
		std::lock_guard<std::mutex> lock(mutex_);
		data_ = stack.data_;
	}
	threadsafe_stack_waitable& operator=(const threadsafe_stack_waitable&) = delete;
	void push(T new_value) {
		std::lock_guard<std::mutex> lock(mutex_);
		data_.push(std::move(new_value));
		cv.notify_one();
	}

	std::shared_ptr<T> wait_and_pop() {
		std::lock_guard<std::mutex> lock(mutex_);
		cv.wait(lock, [this]() {/*只要栈为空则一直阻塞等待*/
			if (data_.empty()) {
				return false;
			}
			return true;
		});

		std::shared_ptr<T> const res(std::make_shared<T>(std::move(data_.top())));
		data_.pop();
		return res;
	}

	void wait_and_pop(T&value) {
		std::unique_lock<std::mutex> lock(mutex_);
		cv.wait(lock, [this]() {
			if (data_.empty()) {
				return false;
			}
			return true;
		});

		value = std::move(data_.top());
		data_.pop();
	}
	/*try_pop版本与前面的wait阻塞等待不同，这里要是栈为空则直接返回*/
	bool try_pop(T&value) {
		std::lock_guard<std::mutex> lock(mutex_);
		if (data_.empty()) {
			return false;
		}

		value = std::move(data_.top());
		data_.pop();
		return true;
	}

	std::shared_ptr<T> try_pop() {
		std::lock_guard<std::mutex> lock(mutex_);
		if (data_.empty()) {
			return std::shared_ptr<T>();
		}
		std::shared_ptr<T> res(std::make_shared<T>(std::move(data_.top())));
		data_.pop();
		return res;
	}
	bool empty() const {
		std::lock_guard<std::mutex> lock(mutex_);
		return data_.empty();
	}
};
#endif // !STACKPOPNOERROR_H