//
//  main.cpp
//  richterpool
//
//  Created by zz on 2013.04.17.
//  Copyright (c) 2013 zz. All rights reserved.
//
#include <thread>
#include <iostream>
#include <vector>
#include <queue>
#include <mutex>
#include <future>
#include <utility>
#include <type_traits>

struct ITask {
    virtual void execute() = 0;
    virtual ~ITask(){};
};

template<typename TResult>
struct task_impl: public ITask {
	
    task_impl(std::packaged_task<TResult()> t):task_(std::move(t))
    {}
    
    std::packaged_task<TResult()> task_;
    void execute(){
        task_();
    }
};

class thread_pool
{
    std::vector<std::thread> threads_;
    std::queue<std::unique_ptr<ITask>> tasks_;//should be deque/stack
    std::mutex lock_;
public:
	
    thread_pool():threads_(std::thread::hardware_concurrency() * 2){
        for (auto& thread : threads_){
            thread = std::thread(&thread_pool::run, this);
        }
    }

    // http://en.cppreference.com/w/cpp/types/result_of

    template<typename Callable>
    std::future<typename std::result_of<Callable()>::type> execute_async(Callable func) {
        std::packaged_task<Callable> task(func);
        auto result = task.get_future();
        
        task_impl<typename std::result_of<Callable()>::type> t(std::move(task));
		
        std::lock_guard<std::mutex> _(lock_);
        tasks_.push(std::move(t));
        
        return std::move(result);
    }
    
private:
    void run() {
        while(true) {
            std::lock_guard<std::mutex> _(lock_);
            auto task = std::move(tasks_.front());
            task->execute();
            tasks_.pop();
        }
    }
    
};

int ha() { return 42; }

int main(){
    thread_pool pool;
    auto res = pool.execute_async(ha);
    auto res2 = pool.execute_async([](){return 42;});
    return res.get() + res2.get();
}
