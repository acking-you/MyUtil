//
// Created by Alone on 2022-8-20.
//

#ifndef MYUTIL_THEADSAFE_QUEUE_H
#define MYUTIL_THEADSAFE_QUEUE_H

#include<memory>
#include<mutex>
#include<condition_variable>

namespace util
{
    using std::shared_ptr;
    using std::mutex;
    using std::lock_guard;
    using std::condition_variable;
    using std::unique_lock;

    template<typename T>
    class ThreadSafeQueue
    {
    private:
        struct node
        {
            shared_ptr<T> data_;
            node *next_{};
        };
    public:
        ThreadSafeQueue() : m_head(new node), m_tail(m_head)
        {}

        ~ThreadSafeQueue()
        {
            destroy();
        }

        ThreadSafeQueue(ThreadSafeQueue const &) = delete;

        ThreadSafeQueue &operator=(ThreadSafeQueue const &) = delete;
        
        void Push(T value)
        {
            auto p = std::make_shared<T>(std::move(value));
            node *new_tail = new node;
            {
                lock_guard<mutex> tail_lock(m_tailMtx);
                m_tail->data_ = std::move(p);
                m_tail->next_ = new_tail;
                m_tail = new_tail;
            }
            m_cond.notify_one();
        }

        shared_ptr<T> WaitAndPop()
        {
            node *old_head = wait_pop_head();
            auto ret = old_head->data_;
            delete old_head;
            return ret;
        }

        void WaitAndPop(T &value)
        {
            node *old_head = wait_pop_head(value);
            delete old_head;
        }

        shared_ptr<T> TryPop()
        {
            node *old_head = try_pop_head();

            if (!old_head) return nullptr;

            auto ret = old_head->data_;
            delete old_head;
            return ret;
        }

        bool TryPop(T &value)
        {
            node *old_head = try_pop_head(value);

            if (!old_head)return false;

            delete old_head;

            return true;
        }

    private:
        node *get_tail()
        {
            lock_guard<mutex> tail_lock(m_tailMtx);
            return m_tail;
        }

        node *pop_head()
        {
            auto *old_head = m_head;
            m_head = m_head->next_;
            return old_head;
        }

        unique_lock<mutex> wait_for_data()
        {
            unique_lock<mutex> head_lock(m_headMtx);
            m_cond.wait(head_lock, [this]
            {
                return m_head != get_tail();
            });
            return std::move(head_lock);
        }

        node *wait_pop_head()
        {
            unique_lock<mutex> head_lock(wait_for_data());
            return pop_head();
        }

        node *wait_pop_head(T &value)
        {
            unique_lock<mutex> head_lock(wait_for_data());
            value = std::move(*(m_head->data_));
            return pop_head();
        }

        node *try_pop_head()
        {
            lock_guard head_lock(m_headMtx);
            if (m_head == get_tail())
            {
                return nullptr;
            }
            return pop_head();
        }

        node *try_pop_head(T &value)
        {
            lock_guard head_lock(m_headMtx);
            if (m_head == get_tail())
            {
                return nullptr;
            }
            value = std::move(*(m_head->data_));
            return pop_head();
        }

        void destroy()
        {
            std::lock(m_headMtx, m_tailMtx); //获得锁确保操作的内存不被共享
            while (m_head)
            {
                auto *next = m_head->next_;
                delete m_head;
                m_head = next;
            }
        }

    private:
        condition_variable m_cond;
        mutex m_headMtx;
        node *m_head;
        mutex m_tailMtx;
        node *m_tail;
    };
}


#endif //MYUTIL_THEADSAFE_QUEUE_H
