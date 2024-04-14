#ifndef STACK_H
#define STACK_H
#include <exception>
#include <mutex>
#include <stack>
#include <utility>
#include <condition_variable>
struct empty_stack :std::exception {
	const char* what() const throw();
};
template <typename T>
class threadsafe_stack {
	std::stack<T> data_;
	mutable std::mutex mutex_;
public:
	threadsafe_stack(){}
	threadsafe_stack(const threadsafe_stack& other) {
		std::lock_guard<std::mutex> lock(mutex_);
		data_ = other.data_;
	}
	threadsafe_stack& operator=(const threadsafe_stack&) = delete;

	void push(T new_value) {
		std::lock_guard<std::mutex> lock(mutex_);
		data_.push(std::move(new_value));
	}
	std::shared_ptr<T> pop() {
		std::lock_guard<std::mutex> lock(mutex_);
		if (data_.empty())throw empty_stack(); //栈空判断
		std::shared_ptr<T> const res(std::make_shared<T>(std::move(data_.top())));
		data_.pop();
		return res;
	}
	void pop(T& value) {
		std::lock_guard<std::mutex> lock(mutex_);
		if (data_.empty()) throw empty_stack();
		value = std::move(data_.top());       //栈空判断
		data_.pop();
	}

	bool empty() const {
		std::lock_guard<std::mutex> lock(mutex_);
		return data_.empty();
	}
};
//这个栈义弹出若栈为空则立马就会报异常
#endif // !STACK_H