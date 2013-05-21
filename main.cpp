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
    std::atomic<bool> terminate_;

public:
    thread_pool()
        : threads_(std::thread::hardware_concurrency()*2)
        , terminate_(false)
    {
        for (auto& thread : threads_) {
            thread = std::thread(&thread_pool::run, this);
        }
    }

    ~thread_pool()
    {
        // notify about death
        terminate_ = true;
        for (auto& thread : threads_) {
            thread.join();
        }
    }

    template<typename TFunc, typename ...TArgs>
    std::future<typename std::result_of<TFunc(TArgs...)>::type> execute_async(TFunc func, TArgs&&... args)
    {
        typedef typename std::result_of<TFunc(TArgs...)>::type return_type;
        typedef std::packaged_task<return_type()> task_type;
        typedef std::future<return_type> future_type;
        task_type task([=](){ return func(std::forward<TArgs>(args)...); });
        future_type result = task.get_future();

        {
            std::unique_ptr<abstract_task> ptask(new task_impl<return_type>(std::move(task)));
            std::lock_guard<std::mutex> _(lock_tasks_);
            tasks_.push(std::move(ptask));
        }

        return result;
    }

private:
    void run()
    {
        // TODO: add exit condition
        while (!terminate_)
        {
            std::unique_ptr<abstract_task> task;
            {
                std::lock_guard<std::mutex> _(lock_tasks_);
                if (tasks_.empty()) {
                    continue;
                }
                std::cout << "Task taken by thread: " << std::this_thread::get_id() << std::endl; 
                task = std::move(tasks_.front());
                tasks_.pop();
            }
            task->execute();
        }
    }
};

int sum(int x, int y) {
    return x + y;
}

void fuck(int x, int y) {
    std::cout << x << " fuck " << y << std::endl;
}

class X {
public:
    void suck(){
        std::cout << "suck" << std::endl;
    }
};

void stupid_sort(std::vector<int>& array){
    for (size_t index = 0; index < array.size(); ++index)
        for (size_t index2 = 1; index2 < array.size(); ++index2) {
            if (array[index2] < array[index2-1]) {
                int temp = array[index2];
                array[index2] = array[index2-1];
                array[index2-1] = temp;
            }
        }
}

int main()
{
    thread_pool pool;
    // auto res = pool.execute_async([](){return 42;});
    // auto res2 = pool.execute_async([](){std::cout << "Hello world" << std::endl; });
    // auto res3 = pool.execute_async(sum, 2, 3);
    // auto res4 = pool.execute_async(fuck, 2, 3);
    // TODO: implement this
    // X x;
    // auto res5 = pool.execute_async(&X::suck, &x);
    // res4.get();
    // std::cout << res3.get();
    // std::cout << res.get();
    // res2.get();
    // 
    std::vector<int> super_array(100000000);
    std::vector<int>& arr = super_array;
    std::vector<std::future<void>> futures;
    futures.push_back(pool.execute_async(stupid_sort, super_array));//,
    futures.push_back(pool.execute_async(stupid_sort, super_array));
    futures.push_back(pool.execute_async(stupid_sort, super_array));
    futures.push_back(pool.execute_async(stupid_sort, super_array));

    for (auto& fut: futures) fut.get();
    // };

    return 0;
}
