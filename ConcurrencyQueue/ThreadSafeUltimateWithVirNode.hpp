#ifndef THREADSAFEQUEUEWITHNODE
#define THREADSAFEQUEUEWITHNODE
#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <utility>
//添加一个虚位节点表示一个空的节点没有数据
template<typename T>
class threadsafe_queue_node {
private:
	struct node
	{
		std::shared_ptr<T> data;
		std::unique_ptr<node> next;
	};
	std::mutex head_mutex;
	std::unique_ptr<node> head;
	std::mutex tail_mutex;
	node* tail;

	std::condition_variable data_cond;
	node* get_tail() {
		std::lock_guard<std::mutex> tail_lock(tail_mutex);
		return tail;
	}

	std::unique_ptr<node> pop_head() {
		std::unique_ptr<node> old_head = std::move(head);
		head = std::move(old_head->next);
		return old_head;
	}
	//等待获取头部锁
	std::unique_lock<std::mutex> wait_for_data() {
		std::unique_lock<std::mutex> head_lock(head_mutex);
		data_cond.wait(head_lock, [&]() {
			return head.get() != get_tail(); //(5)
		});

		return head_lock;
	}
	//获取头部节点数据 
	std::unique_ptr<node> wait_pop_head() {
		std::unique_lock<std::mutex> head_lock(wait_for_data());
		return pop_head();
	}

	std::unique_ptr<node> wait_pop_head(T&value) {
		std::unique_lock<std::mutex> head_lock(wait_for_data());
		value = std::move(*head->data);
		return pop_head();
	}

	std::unique_ptr<node> try_pop_head() {
		std::lock_guard<std::mutex> head_lock(head_mutex);
		if (head.get() == get_tail()) {
			return std::unique_ptr<node>();
		}

		return pop_head();
	}
	std::unique_ptr<node> try_pop_head(T&value) {
		std::lock_guard<std::mutex> head_lock(head_mutex);
		if (head.get() == get_tail()) {
			return std::unique_ptr<node>();
		}
		value = std::move(*head->data);
		return pop_head();
	}

public:
	threadsafe_queue_node():           //(1)
		head(new node),tail(head.get()){}

	threadsafe_queue_node(const threadsafe_queue_node& other) = delete;
	threadsafe_queue_node& operator=(const threadsafe_queue_node&) = delete;

	std::shared_ptr<T> wait_and_pop() { //(3)
		std::unique_ptr<node> const old_head = wait_pop_head();
		return old_head->data;
	}

	void wait_and_pop(T& value) {       //(4)
		std::unique_ptr<node> const old_head = wait_pop_head(value);
	}

	std::shared_ptr<T> try_pop() {
		std::unique_ptr<node> old_head = try_pop_head();
		return old_head ? old_head->data : std::shared_ptr<T>();
	}
	bool try_pop(T& value)
	{
		std::unique_ptr<node> const old_head = try_pop_head(value);
		return old_head;
	}

	bool empty() {
		std::lock_guard<std::mutex> head_lock(head_mutex);
		return (head.get() == get_tail());
	}

	void push(T new_value) { //(2)
		std::shared_ptr<T> new_data(std::make_shared<T>(std::move(new_value)));
		std::unique_ptr<node> p(new node);
		node* const new_tail = p.get(); //p就是新的虚尾节点new_tail
		std::lock_guard<std::mutex> tail_lock(tail_mutex);
		tail->data = new_data;
		tail->next = std::move(p);
		tail = new_tail;
		data_cond.notify_one();
	}
};
#endif // !THREADSAFEQUEUEWITHNODE