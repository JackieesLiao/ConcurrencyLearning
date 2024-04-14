#ifndef _JTHREAD_H
#define _JTHREAD_H
#include <thread>
#include <utility>
class Joining_thread
{
public:
	//Ĭ�Ϲ��캯��
	Joining_thread() noexcept = default;

	template<typename Callable,typename...Args>
	explicit Joining_thread(Callable&& func, Args&&...args):
		_t(std::forward<Callable>(func),std::forward<Args>(args)...){}

	//���������̶߳�����Դ�ƶ�����Ա�����̶߳�����
	explicit Joining_thread(std::thread t)noexcept
		:_t(std::move(t)){}
	//�ٷֱ� ����һ���ƶ�������ƶ���ֵ����
	Joining_thread(Joining_thread&&other) noexcept
		:_t(std::move(other._t)){}
	Joining_thread& operator=(Joining_thread&& other) noexcept
	{
		//�����ǰ�߳̿ɻ�ϣ����ϵȴ��߳���ɺ��ٸ�ֵ
		if (joinable()) {
			join();
		}
		_t = std::move(other._t);
		return *this;
	}
	/*Joining_thread& operator=(Joining_thread other) noexcept
	{
		if (joinable()) {
			join();
		}
		_t = std::move(other._t);
		return *this;
	}*/

	~Joining_thread() {
		if (joinable()) {
			join();
		}
	}

	void swap(Joining_thread& other) noexcept
	{
		_t.swap(other._t);
	}

	//��ȡ�߳�id
	std::thread::id getID() const noexcept
	{
		return _t.get_id();
	}

	bool joinable() const noexcept
	{
		return _t.joinable();
	}

	void join()
	{
		_t.join();
	}

	void detach()
	{
		_t.detach();
	}

	std::thread& as_thread() noexcept
	{
		return _t;
	}

	const std::thread& as_thread() const noexcept
	{
		return _t;
	}
private:
	std::thread _t;
};
#endif // ! _JTHREAD_H