//
// Created by Alone on 2022-8-20.
//

//
// Created by Alone on 2022-8-20.
//

#ifndef MYUTIL_THREADSAFEQUEUE_H
#define MYUTIL_THREADSAFEQUEUE_H

#include <memory>
#include <condition_variable>
#include<mutex>

namespace v3
{
    using std::shared_ptr;
    using std::unique_ptr;
    using std::mutex;
    using std::lock_guard;
    using std::unique_lock;
    using std::condition_variable;

    template<typename T>
    class ThreadSafeQueue
    {
    private:
        struct node
        {
            shared_ptr <T> data_;
            unique_ptr <node> next_;
        };
        condition_variable m_cond;
        mutex m_headMtx;
        unique_ptr <node> m_head;
        mutex m_tailMtx;
        node *m_tail;
    public:
        ThreadSafeQueue() : m_head(new node), m_tail(m_head.get())
        {}

        ThreadSafeQueue(ThreadSafeQueue const &) = delete;

        ThreadSafeQueue &operator=(ThreadSafeQueue const &) = delete;

        void Push(T new_value)
        {
            auto new_data = std::make_shared<T>(std::move(new_value));
            std::unique_ptr <node> p(new node);
            {//生产临界区
                std::lock_guard <std::mutex> tail_lock(m_tailMtx);
                m_tail->data_ = new_data;
                auto *new_tail = p.get();
                m_tail->next_ = std::move(p);
                m_tail = new_tail;
            }
            m_cond.notify_one();
        }

        std::shared_ptr <T> WaitAndPop()
        {
            std::unique_ptr <node> const old_head = wait_pop_head();
            return old_head->data;
        }

        void WaitAndPop(T &value)
        {
            auto _ = wait_pop_head(value);
        }

        std::shared_ptr <T> TryPop()
        {
            std::unique_ptr <node> old_head = try_pop_head();
            return old_head ? old_head->data : std::shared_ptr<T>();
        }

        bool TryPop(T &value)
        {
            std::unique_ptr <node> const old_head = try_pop_head(value);
            return old_head;
        }

        void Empty()
        {
            std::lock_guard <std::mutex> head_lock(m_headMtx);
            return (m_head.get() == get_tail());
        }

    private:
        node *get_tail()
        {
            lock_guard <mutex> tail_lock(m_tailMtx);
            return m_tail;
        }

        unique_ptr <node> pop_head()
        {
            auto old_head = std::move(m_head);
            m_head = std::move(old_head->next_);
            return old_head;
        }

        unique_lock <mutex> wait_for_data() //这个分成wait_pop_head就是为了多版本扩展少写点代码
        {
            unique_lock <mutex> head_lock(m_headMtx);
            m_cond.wait(head_lock, [&]
            {
                return m_head.get() != get_tail();
            });
            return std::move(head_lock);
        }

        unique_ptr <node> wait_pop_head() //版本1
        {
            unique_lock <mutex> head_lock(wait_for_data());
            return pop_head();
        }

        unique_ptr <node> wait_pop_head(T &value)
        {
            unique_lock <mutex> head_lock(wait_for_data());
            value = std::move(*(m_head->data_));
            return pop_head();
        }

        unique_ptr <node> try_pop_head()
        {
            std::lock_guard <std::mutex> head_lock(m_headMtx);
            if (m_head.get() == get_tail())
            {
                return std::unique_ptr<node>();
            }
            return pop_head();
        }

        unique_ptr <node> try_pop_head(T &value)
        {
            std::lock_guard <std::mutex> head_lock(m_headMtx);
            if (m_head.get() == get_tail())
            {
                return std::unique_ptr<node>();
            }
            value = std::move(*(m_head->data_));
            return pop_head();
        }
    };
}


#endif //MYUTIL_THREADSAFEQUEUE_H