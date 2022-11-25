//
// Created by Alone on 2022-8-19.
//

#ifndef MYUTIL_QUEUE_H
#define MYUTIL_QUEUE_H

#include<memory>

namespace v2
{
    template<typename T>
    class Queue
    {
    private:
        struct node
        {
            T data_;
            std::unique_ptr <node> next_;

            node(T data) : data_(std::move(data))
            {}
        };

        std::unique_ptr <node> m_head;
        node *m_tail{};

    public:
        Queue() = default;

        Queue(const Queue &other) = delete;

        Queue &operator=(const Queue &other) = delete;

        std::shared_ptr <T> TryPop()
        {
            if (!m_head)
            {
                return nullptr;
            }
            auto ret = std::make_shared<T>(std::move(m_head->data_));
            auto oldHead = std::move(m_head);
            m_head = std::move(oldHead->next_);
            return ret;
        }

        void Push(T new_value)
        {
            auto p = std::make_unique<node>(new_value);
            auto *new_tail = p.get();
            if (m_tail)
            {//如果队列不为空
                m_tail->next_ = std::move(p);
            } else
            {//队列为空则需要特殊处理
                m_head = std::move(p);
            }
            m_tail = new_tail;
        }
    };
}
#endif //MYUTIL_QUEUE_H
