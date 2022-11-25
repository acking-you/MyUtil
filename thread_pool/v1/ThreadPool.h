//
// Created by Alone on 2022-8-23.
//

#ifndef MYUTIL_THREADPOOL_H
#define MYUTIL_THREADPOOL_H

#include"../../threadsafe_queue/theadsafe_queue.h"
#include"../joiner.h"
#include<vector>
#include<functional>
#include<thread>

namespace v1
{
    using std::thread;
    using std::vector;
    using std::function;
    using std::atomic;

    class ThreadPool
    {
    public:
        explicit ThreadPool(uint32_t threadNum = thread::hardware_concurrency()) : m_done(false),
                                                                                   m_joiner(m_threads)
        {
            try
            {
                for (int i = 0; i < threadNum; i++)
                {
                    m_threads.emplace_back(&ThreadPool::worker_thread, this);
                }
            }
            catch (...)
            {
                m_done = true; //任务发生意外
                throw std::runtime_error("thread create error");
            }
        }

        ~ThreadPool()
        {
            m_done = true;
        }

        template<typename FunctionType>
        void Submit(FunctionType f)
        {
            m_queue.Push(function<void()>(f));
        }

    private:
        void worker_thread()
        {
            while (!m_done)
            {
                function<void()> task;
                if (m_queue.TryPop(task))
                {
                    task();
                } else
                {
                    std::this_thread::yield(); //当前线程挂起，避免争抢资源
                }
            }
        }

    private:
        atomic<bool> m_done;
        util::ThreadSafeQueue<function<void()>> m_queue;
        vector<thread> m_threads;
        util::Joiner m_joiner;
    };
}


#endif //MYUTIL_THREADPOOL_H
