#include<future>
#include<thread>
#include<iostream>
#include<functional>

template<typename>
class my_package_task;


//特化版本
template<typename ReturnType, typename... Args>
class my_package_task<ReturnType(Args...)>
{

public:
    template<typename Task>
    explicit my_package_task(Task &&task) : m_task(std::forward<Task>(task))
    {}

    template<typename... Params>
    void operator()(Params &&...args)
    {
        m_promise.set_value(m_task(std::forward<Args>(args)...));
    }

    std::future<ReturnType> get_future()
    {
        return m_promise.get_future();
    }

private:
    std::function<ReturnType(Args...)> m_task;
    std::promise<ReturnType> m_promise;
};

int print_task()
{
    std::cout << "task finish" << std::endl;
    return 1;
}

std::future<int> my_async(std::function<int(int)> task, int arg)
{
    my_package_task<int(int)> _task{task};

}

int main()
{

    auto f = std::async([]()
                        {
                            std::this_thread::sleep_for(std::chrono::seconds(1));
                            std::cout << "async finish" << std::endl;
                            return 1;
                        });
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::cout << 323;
    std::cout << f.get();

}