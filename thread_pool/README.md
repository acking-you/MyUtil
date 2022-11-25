# 线程池实现

> 整体实现架构

![绘图8.png](https://p6-juejin.byteimg.com/tos-cn-i-k3u1fbpfcp/5d21016f7b004ec58994d60234bab755~tplv-k3u1fbpfcp-watermark.image?)

> 代码仓库：[thread_pool](https://github.com/ACking-you/MyUtil/tree/master/thread_pool)

## 简易版本实现

> 整体代码实现

```cpp
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
```

> Joiner小工具

```cpp
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
```

### 易错点

1. 需要有任务发生意外的线程正常终止的处理。
2. 析构的顺序需要额外注意。
3. 线程安全需要额外注意。对应使用线程安全队列以及原子操作的bool类型变量。

这里具体的线程安全队列实现请看之前的视频：[细粒度锁实现线程安全队列](https://www.bilibili.com/video/BV1qU4y1r7si?spm_id_from=333.999.0.0)

## 带同步语义的实现（返回future）

显然想让一个函数返回一个future，在C++中有现成的packaged_task类进行封装，具体请看之前的讲解：[异步操作同步api讲解](https://www.bilibili.com/video/BV1qd4y1o749?spm_id_from=333.999.0.0)

如果想要实现它，我们可能很快想到，把之前的 function 替换为 packaged_task 存入队列不就行了？这个确实是可行的，但是我们作为一个需要通过返回 futrue
得到返回值来说的设计，我们不可能简单粗暴的把返回值就写死为void，那这除了等待任务完成，其余毫无用处。。。

我们现在需求增加到：

1. 实现同步等待任务完成。
2. 实现得到任意函数返回值。

想要完成这样的需求，那么 `packaged_task` 里的类型就不能直接写死了，比如不能直接像之前那样写 `packaged_task<void()>` 类型了，我们需要借助模板实现对任意 `packaged_task`
类型的接收，但是这样很快就会出现一个问题：作为 `ThreadPool` 类的成员函数来说，想要成员函数支持泛型，那么这个类也只能是泛型，但是如果仅仅只为了这一个函数类型而进行整体的类泛型，这得不偿失...

这个时候就不得不请出**类型擦除**方式实现的泛型了！
这种泛型在Java中得到发扬光大，虽然会有一定性能损耗，但可拓展性极强，具体原理是利用类的继承关系，以及虚函数的多态实现的泛型。

举个例子：在Java中所有的类都继承自Object类，而这个类有toString方法，所以子类只要实现了toString方法，而某个打印类就只需要存储Object类型便可完成任意类型的toString。这个把子类用基类的类型来接住然后调用的过程，我们称之为类型擦除的过程。比如String类型要想打印，那么在打印过程中会被擦除为Object类型然后利用多态调用其toString方法即可。这个重写父类方法实现的多态行为，在C++中是需要非常熟悉其中的原理的，其实就是利用了虚函数表，唯一的性能损耗就是需要多寻址一次，而且由于这种方式实现的内存一般离得比较远，这也是一定的性能损耗。

现在回到我们这里，我们可以通过实现 `FunctionWrapper` 类来对函数进行包装，里面存入一个带虚函数的基类类型 `impl_base`
，然后通过调用基类的接口来实现我们想要的功能，而具体的实现细节则由子类来实现，这个实现可以是任意类型的函数调用，这样就只有局部需要泛型了，而不需要将整个 `FunctionWrapper` 变成泛型类。

具体代码如下：

```
class FunctionWrapper
{
    //类型擦除的基础类型
    struct impl_base
    {
        virtual void call() = 0;

        virtual ~impl_base() = default;
    };

    //类型擦除的实现类型，泛型接收任意类型
    template<typename F>
    struct impl_type : public impl_base
    {
        F f;

        explicit impl_type(F &&f) : f(std::move(f))
        {}

        void call() override
        {
            f();
        }
    };

public:
    template<typename F>
    FunctionWrapper(F &&func):m_impl(new impl_type<F>(std::forward<F>(func)))
    {}

    FunctionWrapper(FunctionWrapper &&other) noexcept: m_impl(std::move(other.m_impl))
    {}

    FunctionWrapper() = default;

    FunctionWrapper &operator=(FunctionWrapper &&other)
    noexcept
    {
        m_impl = std::move(other.m_impl);
        return *this;
    }

    void operator()()
    {
        m_impl->call();
    }

    FunctionWrapper(FunctionWrapper const &) = delete;

    FunctionWrapper(FunctionWrapper &) = delete;

    FunctionWrapper &operator=(FunctionWrapper &) = delete;

private:
    std::unique_ptr<impl_base> m_impl;
};
```

> 注意由于我们这里已经很明确是要存入 packaged_task 泛型类型，而这个类型只支持移动构造和赋值，所以我们把其他版本的构造和赋值都删除，实现对应的移动即可。

### 完整代码实现

> 我们为了让整个类摆脱泛型，我们已经实现了类型擦除的存储版本，接下来是整个线程池的实现代码。

```
class ThreadPool
{
public:
    explicit ThreadPool(uint32_t threadNum = std::thread::hardware_concurrency()) : m_done(false),
                                                                                    m_joiner(m_threads)
    {
        try
        {
            for (int i = 0; i < threadNum; i++)
            {
                m_threads.emplace_back(thread(&ThreadPool::worker_thread, this));
            }
        } catch (...)
        {
            m_done = true;
            throw std::runtime_error("add thread error");
        }
    }

    ~ThreadPool()
    {
        m_done = true;
    }

    template<typename FunctionType>
    std::future<typename std::result_of<FunctionType()>::type> submit(FunctionType f)
    {
        using resultType = typename std::result_of<FunctionType()>::type;
        packaged_task<resultType()> task(std::move(f));
        future<resultType> res(task.get_future());
        m_queue.Push(std::move(task));
        return res;
    }

private:
    void worker_thread()
    {
        while (!m_done)
        {
            FunctionWrapper task;
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
    std::atomic<bool> m_done;
    util::ThreadSafeQueue<FunctionWrapper> m_queue;
    vector<thread> m_threads;
    util::Joiner m_joiner;
};
```

解释如下：

* submit 成员函数改为泛型版本，利用 `std::result_of` 可以获取对应传入函数的返回值类型，方便构造对应的 `future` 。
* 还有个小细节，我当时被这个错误耽误了至少20分钟... 注意保留`FunctionWrapper` 的隐式转化，因为在 `m_queue.Push` 的过程中，你并没有指定强转，所以需要隐式转化（主要这个地方被CLion的优化给坑了