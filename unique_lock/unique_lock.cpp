// unique_lock.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <memory>
#include <mutex>
#include <map>
#include <shared_mutex>
#include <thread>
#include <vector>
std::mutex mtx;
int shared_data = 0;
static void use_unique()
{
    std::unique_lock<std::mutex> lock(mtx);
    shared_data++;
    lock.unlock();
}
static void owns_lock()
{
    std::unique_lock<std::mutex> lock(mtx);
    shared_data++;
    if (lock.owns_lock()) {
        std::cout << "owns lock\n";
    }
    else {
        std::cout << "doesn't own lock\n";
    }
    lock.unlock();

    if (lock.owns_lock()) {
        std::cout << "owns lock\n";
    }
    else {
        std::cout << "doesn't own lock\n";
    }
}

//unique_lock可以延迟加锁
static void def_lock()
{
    std::unique_lock<std::mutex> lock(mtx, std::defer_lock);
    lock.lock();
    //可以自动析构解锁也可以手动解锁
    lock.unlock();
}

static void use_own_defer()
{
    std::unique_lock<std::mutex> lock(mtx);
    if (lock.owns_lock()) {
        std::cout << "Main thread owns lock\n";
    }
    else {
        std::cout << "Main thread does not have the lock\n";
    }

    std::thread t1([]() {
        std::unique_lock<std::mutex> lock(mtx, std::defer_lock);
        if (lock.owns_lock()) {
            std::cout << "child thread owns lock\n";
        }
        else {
            std::cout << "child thread does not have the lock\n";
        }

        lock.lock();
        //判断是否拥有锁
        if (lock.owns_lock()) {
            std::cout << "child thread has lock\n";
        }
        else {
            std::cout << "child thread does not have the lock\n";
        }

        lock.unlock();
    });
    t1.join();
}
 
//unique_lock支持领养操作
static void use_own_adopt()
{
    mtx.lock();
    std::unique_lock<std::mutex> lock(mtx, std::adopt_lock);
    if (lock.owns_lock()) {
        std::cout << "owns lock\n";
    }
    else {
        std::cout << "does not have the lock\n";
    }

    lock.unlock();
}

namespace swap_test {
    int a{ 1 };
    int b{ 2 };
    std::mutex mtx1;
    std::mutex mtx2;

    static void safe_swap() {
        std::lock(mtx1, mtx2);
        //领养加锁 
        std::unique_lock<std::mutex> lock1(mtx1, std::adopt_lock);
        std::unique_lock<std::mutex> lock2(mtx2, std::adopt_lock);
        std::swap(a, b);
    }

    static void safe_swap1() {
        //延迟加锁
        std::unique_lock<std::mutex> lock1(mtx1, std::defer_lock);
        std::unique_lock<std::mutex> lock2(mtx2, std::defer_lock);
        std::lock(lock1, lock2);
        std::swap(a, b);
    } 

    //转移互斥量所有权
    //互斥量本身不支持move操作但是unique_lock支持 

    static std::unique_lock<std::mutex> get_lock() {
        std::unique_lock<std::mutex> lock(mtx);
        shared_data++;
        return lock;
    }

    static void use_return() {
        std::unique_lock<std::mutex> lock(get_lock());
        shared_data++;
    }
}//namespace swap_test

//锁粒度表示加锁的精细程度
//一个锁的粒度要足够大以保证可以锁住要访问的共享数据
//一个锁的粒度要足够小以保证可以非共享数据不被锁住以影响性能
static void precision_lock() {
    std::unique_lock<std::mutex> lock(mtx);
    shared_data++;
    lock.unlock();
    std::this_thread::sleep_for(std::chrono::seconds(1));
    lock.lock();
    shared_data++;
}

class DNSService {
public:
    //读操作采用共享锁
    std::string QueryDNS(std::string& dns_name) {
        std::shared_lock<std::shared_mutex> shared_locks(shared_mtx);
        auto iter = dns_info.find(dns_name);
        if (iter != dns_info.end()) {
            return iter->second;
        }
        return " ";
    }

    //写操作采用独占锁
    void AddDNSInfo(std::string& dns_name, std::string& dns_entry) {
        std::lock_guard<std::shared_mutex> guard_locks(shared_mtx);
        dns_info.insert({ dns_name,dns_entry });
    }
private:
    std::shared_mutex shared_mtx;
    std::map<std::string, std::string> dns_info;
};
//递归锁
namespace recursive_lock {
    class RecursiveLock {
    public:
        RecursiveLock(){}
        bool QueryStudent(const std::string& name) {
            std::lock_guard<std::recursive_mutex> recursive_lock(recursive_mtx);
            auto iter_find = students_info.find(name);
            if (iter_find == students_info.end()) {
                return false;
            }
            return true;
        }

        void AddScore(const std::string& name,double score) {
            std::lock_guard<std::recursive_mutex> recursive_lock(recursive_mtx);
            if (!QueryStudent(name)) {
                //内部调用QueryStudent，所以需要加递归锁，否则会嵌套死锁
                students_info.insert(std::make_pair(name, score));
                return;
            }

            students_info[name] = students_info[name] + score;
        }

        //不推荐使用递归锁
        //推荐拆分共用逻辑，将共用逻辑拆分为统一接口
        void AddScoreAtomic(const std::string& name, double score) {
            std::lock_guard<std::recursive_mutex> recursive_lock(recursive_mtx);
            auto iter_find = students_info.find(name);
            if (iter_find == students_info.end()) {
                students_info.insert(std::make_pair(name, score));
                return;
            }

            students_info[name] = students_info[name] + score;
            return;
        }
    private:
        std::map<std::string, double > students_info;
        std::recursive_mutex recursive_mtx;
    };
}//namespace recursive_lock
int main()
{
    //use_own_defer();
    use_own_adopt();
}