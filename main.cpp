//
//  main.cpp
//  righterpool
//
//  Created by zz on 04.17.13.
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

///=====================
//
template <typename C>
struct has_call_operator
{
        typedef char (&yes)[2];
        typedef char (&no)[1];
        
        template <typename R>
        static yes check(decltype(&R::operator()));
        
        template <typename>
        static no check(...);
        
        static const bool value = sizeof(check<C>(0)) == sizeof(yes);
};
 
template <typename, bool Use>
struct result_of_free
// dummy type, std::conditional doesn't do lazy checking;
// should not be ever used
{
        typedef void type;
};
 
template <typename R, typename... A>
struct result_of_free<R (*)(A...), true>
{
        typedef R type;
};
 
template <typename R, typename... A>
struct result_of_free<R (*)(A......), true>
{
        typedef R type;
};
 
template <typename, bool Use>
struct result_of_memfn_impl
{ };
 
template <typename C, typename R, typename... A>
struct result_of_memfn_impl<R (C::*)(A...), true>
{
        typedef R type;
};
 
template <typename C, typename R, typename... A>
struct result_of_memfn_impl<R (C::*)(A...) const, true>
{
        typedef R type;
};
 
template <typename C, typename R, typename... A>
struct result_of_memfn_impl<R (C::*)(A...) volatile, true>
{
        typedef R type;
};
 
template <typename C, typename R, typename... A>
struct result_of_memfn_impl<R (C::*)(A...) const volatile, true>
{
        typedef R type;
};
 
template <typename C, typename R, typename... A>
struct result_of_memfn_impl<R (C::*)(A......), true>
{
        typedef R type;
};
 
template <typename C, typename R, typename... A>
struct result_of_memfn_impl<R (C::*)(A......) const, true>
{
        typedef R type;
};
 
template <typename C, typename R, typename... A>
struct result_of_memfn_impl<R (C::*)(A......) volatile, true>
{
        typedef R type;
};
 
template <typename C, typename R, typename... A>
struct result_of_memfn_impl<R (C::*)(A......) const volatile, true>
{
        typedef R type;
};
 
template <typename, bool Use>
struct result_of_memfn
// dummy type, std::conditional doesn't do lazy checking;
// should not be ever used
{
        typedef void type;
};
 
template <typename C>
struct result_of_memfn<C, true>
{
        typedef typename result_of_memfn_impl<decltype(&C::operator()), true>::type type;
};
 
template <typename T>
struct result_of
// yeah, I know, quick and dirty
{
        static const bool has_op = has_call_operator<T>::value;
        typedef typename
                std::conditional<
                        has_op,
                        typename result_of_memfn<T, has_op>::type,
                        typename result_of_free<T, !has_op>::type
                >::type type; 
};
//
//=====================


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

    // http://en.cppreference.com/w/cpp/types/result_of does not work :(

    template<typename _Callable>
    std::future<typename std::result_of<_Callable>::type> execute_async(_Callable func) {
        std::packaged_task<_Callable> task(func);
        auto result = task.get_future();
        
        task_impl<typename std::result_of<_Callable>::type> t(std::move(task));
		
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
