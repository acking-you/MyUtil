//
// Created by Alone on 2022-8-23.
//

#ifndef MYUTIL_JOINER_H
#define MYUTIL_JOINER_H

#include <thread>
#include<vector>

namespace util
{
    using std::vector;
    using std::thread;

    class Joiner
    {
    public:
        explicit Joiner(vector<thread> &threads) : m_threads(threads)
        {}

        ~Joiner()
        {
            for (auto &&th: m_threads)
            {
                if (th.joinable())
                {
                    th.join();
                }
            }
        }

    private:
        vector<thread> &m_threads;
    };
}

#endif //MYUTIL_JOINER_H
