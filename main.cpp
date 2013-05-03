#include <iostream>
#include <thread>
#include <future>
#include <mutex>
#include <queue>
#include <vector>
#include <utility>
#include <type_traits>
#include <memory>
#include <atomic>
#include <condition_variable>

struct abstract_task
{
    virtual void execute() = 0;
    virtual ~abstract_task() {};
};

template<typename TResult>
struct task_impl : public abstract_task
{
    typedef std::packaged_task<TResult()> task_type;
    task_type task_;

    task_impl(task_type&& t) {
        task_ = std::move(t);
    }

    void execute() {
        task_();
    }
};

// TODO: 
// 1. add min and max hints
// 2. add queue per thread
// 3. add arbitrary function support (dispatch any function with any number of arguments)
// 4. add lock free containers for tasks and controlled threads
// 5. add priorities for tasks
// 6. add limit on amount of executed tasks
// 7. add thread pool execution control (let tasks use sleep and mutex pool in order to say to pool that this is a long running task and instead of waiting for it to finish - create a new tread) 
// 8. add blows and whistles

class thread_pool
{ 
    std::queue<std::unique_ptr<abstract_task>> tasks_;
    std::vector<std::thread> threads_;
    std::mutex lock_tasks_;
    
    // for suspending threads which do not have anything to do
    std::atomic<bool> terminate_{false};
    std::atomic<size_t> execute_task_{size_t(0)};

    std::mutex condition_mutex_;
    std::condition_variable condition_;

public:
    thread_pool()
        : threads_(std::thread::hardware_concurrency()*2)
    {
        std::clog << "HW concurrency: " << std::thread::hardware_concurrency() << std::endl;
        for (auto& thread : threads_) {
            thread = std::thread(&thread_pool::run, this);
        }
    }

    ~thread_pool()
    {
        std::clog << "Thread pool dead, joining runners" << std::endl;
        // notify about death
        terminate_ = true;
        condition_.notify_all();
        for (auto& thread : threads_) {
            thread.join();
        }
    }

    template<typename TFunc>
    std::future<typename std::result_of<TFunc()>::type> execute_async(TFunc func)
    {
        typedef typename std::result_of<TFunc()>::type return_type;
        typedef std::packaged_task<return_type()> task_type;
        typedef std::future<return_type> future_type;
        task_type task(func);
        future_type result = task.get_future();

        {
            std::unique_ptr<abstract_task> ptask(new task_impl<return_type>(std::move(task)));
            std::lock_guard<std::mutex> _(lock_tasks_);
            tasks_.push(std::move(ptask));
            ++execute_task_;
            condition_.notify_one();
            std::clog << "Thread " << std::this_thread::get_id() << " added task " << ptask.get() << " tasks to exec " << execute_task_ << std::endl;
        }

        return result;
    }

private:
    void run()
    {
        std::cout << "Started thread runner id " << std::this_thread::get_id();
        while (true)
        {
            std::unique_lock<std::mutex> lock(condition_mutex_);
            condition_.wait(lock, [this]()
            {
                return terminate_ || execute_task_ != 0;
            });

            if (terminate_) {
                std::clog << "Runner " << std::this_thread::get_id() << " finished, bye-bye!" << std::endl;
                return;
            }

            while(true) {
                std::unique_ptr<abstract_task> task;
                {
                    std::lock_guard<std::mutex> _(lock_tasks_);
                    if (execute_task_ == 0) break;
 
                    --execute_task_;
                    task = std::move(tasks_.front());
                    tasks_.pop();
                }
                std::clog << "Runner " << std::this_thread::get_id() << " grabbed task " << task.get() << std::endl;
                task->execute();
            }
        }
    }
};

int main()
{
    thread_pool pool;
    for(size_t index = 0; index < 100; ++index)
    {   
        pool.execute_async([index](){
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            std::cout << std::this_thread::get_id() << index << std::endl; });
        pool.execute_async([](){std::cout << std::this_thread::get_id() << "Hello world" << std::endl; });
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(10000));
    //    std::cout << res.get();
    //res2.get();

    return 0;
}
