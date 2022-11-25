## 包含同步语义的简单实现

```cpp
template <typename T>
class ThreadSafeQueue
{
public:
    void Push(T new_value)
    {
        std::lock_guard<std::mutex> lk(m_mtx);
        m_queue.push(std::move(new_value));
        m_cond.notify_one(); // 1
    }

    void WaitAndPop(T &value) // 2
    {
        std::unique_lock<std::mutex> lk(m_mtx);
        m_cond.wait(lk, [this]
                    { return !m_queue.empty(); });
        value = std::move(m_queue.front());
        m_queue.pop();
    }

    std::shared_ptr<T> WaitAndPop() // 3
    {
        std::unique_lock<std::mutex> lk(m_mtx);
        m_cond.wait(lk, [this]
                    { return !m_queue.empty(); }); // 4
        std::shared_ptr<T> res(
            std::make_shared<T>(std::move(m_queue.front())));
        m_queue.pop();
        return res;
    }

    bool TryPop(T &value)
    {
        std::lock_guard<std::mutex> lk(m_mtx);
        if (m_queue.empty())
            return false;
        value = std::move(m_queue.front());
        m_queue.pop();
        return true;
    }

    std::shared_ptr<T> TryPop()
    {
        std::lock_guard<std::mutex> lk(m_mtx);
        if (m_queue.empty())
            return std::shared_ptr<T>(); // 5
        std::shared_ptr<T> res(
            std::make_shared<T>(std::move(m_queue.front())));
        m_queue.pop();
        return res;
    }

    bool Empty() const
    {
        std::lock_guard<std::mutex> lk(m_mtx);
        return m_queue.empty();
    }

private:
    mutable mutex m_mtx;
    queue<T> m_queue;
    condition_variable m_cond;
};
```

这个版本是最为简单的实现版本，直接用的stl库中的队列来实现，所有成员函数公用一把锁来实现线程安全，需要注意的点有以下几点：

1. 条件变量产生的虚假唤醒，你可以通过手动while循环来避免，也可以通过在wait后面加上谓词条件（lamda表达式）
2. 锁需要设置为mutable，保证const版本的成员函数可用

但这个实现有非常大的隐患和不足！

### 隐患

如果在调用WaitAndPop函数时发生了异常，由于可能有其他的线程也在调用WaitAndPop发生等待，而由于每次notify一个线程，一旦构造 std::shared_ptr的过程中发生异常，那么其他的线程将会陷入永久的等待！

**解决方法**：
由于异常发生在内存的申请过程中，我们如果把 `std::queue` 中直接存入 `shared_ptr` 那么就不会有这个问题。

改写后的代码如下：

```cpp
template <typename T>
class ThreadSafeQueue
{
public:
    void Push(T new_value)
    {
        auto data = std::make_shared(std::move(new_value));
        std::lock_guard<std::mutex> lk(m_mtx);
        m_queue.push(data);
        m_cond.notify_one(); // 1
    }

    void WaitAndPop(T &value) // 2
    {
        std::unique_lock<std::mutex> lk(m_mtx);
        m_cond.wait(lk, [this]
                    { return !m_queue.empty(); });
        value = std::move(*m_queue.front());
        m_queue.pop();
    }

    std::shared_ptr<T> WaitAndPop() // 3
    {
        std::unique_lock<std::mutex> lk(m_mtx);
        m_cond.wait(lk, [this]
                    { return !m_queue.empty(); }); // 4
        std::shared_ptr<T> res = m_queue.front();
        m_queue.pop();
        return res;
    }

    bool TryPop(T &value)
    {
        std::lock_guard<std::mutex> lk(m_mtx);
        if (m_queue.empty())
            return false;
        value = std::move(*m_queue.front());
        m_queue.pop();
        return true;
    }

    std::shared_ptr<T> TryPop()
    {
        std::lock_guard<std::mutex> lk(m_mtx);
        if (m_queue.empty())
            return std::shared_ptr<T>(); // 5
        std::shared_ptr<T> res = m_queue.front();
        m_queue.pop();
        return res;
    }

    bool Empty() const
    {
        std::lock_guard<std::mutex> lk(m_mtx);
        return m_queue.empty();
    }

private:
    mutable mutex m_mtx;
    queue<std::shared_ptr<T>> m_queue;
    condition_variable m_cond;
};
```

这个版本的代码不仅是预防了异常安全，同样性能也得到了很好的优化，Push 过程的内存申请过程可以放到临界区以外，提高了并发度。

## 设计细粒度锁队列提高并发

前面的简单版本，有个非常明显的不足，几乎没有任何并发的性能，因为所有的成员函数都必须加锁，临界区非常的大，这哪里是并发，这都强行变成了同步执行，那这样肯定不行啊，我们找找原因。

这个原因很简单，由于我们是通过stl内部的queue封装所实现的，我们的任何的成员函数操作实现都必须访问到这个共享变量，一旦变量被共享，要实现线程安全那就必须加锁同步，这便是原因所在了。

这就是我们现在要做的事情，把锁的粒度减少，实际就是把变量的共享和操作细分。

### 细粒度锁队列实现

#### 实现简单队列

在这之前我们先自己实现一个简单的队列，如下：

```cpp
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
        m_head = std::move(oldHead->next_); //这里把next资源进行转移，防止oldHead析构后导致整个链表析构
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
```

* next指针为啥不用原始指针？嗯，其实应该要用原始指针的，这里偷个懒，为了不写delete语句，用的unique_ptr，在使用这个独占指针的时候记得要转移所有权，否则会出现连环析构的现象！
*
由于使用了unique_ptr管理next_指针，那么析构的时候会自动完成，但是会有个问题，如果队列中的数据量大的话，整个函数栈会爆掉，我亲自测试了下，大概存入的数据量达到1e4级别就会爆栈。。。但是没关系，我们将他用作并发编程中的队列时，用于生产消费的队列里的空闲任务一般也不会到达这个量级，当然有空的话也可以改进然后优化。

**分析并发设计**

我们再来简单分析下这个内存共享的情况pop操作需要用到head，push操作需要用到head和tail。但是有个严重的问题：除了这两个内存被共享外，由于未采用空头节点，两个成员函数内用 `next_`
指针访问到的内存都可能发生共享（对应 `m_tail->next_` 与 `oldHead->next_`）。这样的话很难在保证细粒度的情况下实现线程安全了。。。这样下去的实现还不如之前的。

#### 通过分离数据实现并发

前面的隐患已经分析清楚了，如何解决它？你可以使用预分配一个虚拟节点(无数据)，确保这个节点永远在队列的最后，用来分离头尾指针能访问的节点”的办法，走出这个困境。这样通过 pop 和 push 操作通过 next_
指针访问到的数据就永远不可能是同一个数据了。
代码如下：

```cpp
template<typename T>
class Queue
{
private:
    struct node
    {
        std::shared_ptr<T> data_;
        std::unique_ptr <node> next_;
    };

    std::unique_ptr <node> m_head;
    node *m_tail;

public:
    Queue():m_head(new node),m_tail(m_head.get()){}; //初始化空节点

    Queue(const Queue &other) = delete;

    Queue &operator=(const Queue &other) = delete;

    std::shared_ptr <T> TryPop()
    {
        if (m_head.get() == m_tail)
        {
            return nullptr;
        }
        auto ret = m_head->data_;
        auto oldHead = std::move(m_head); 
        m_head = std::move(oldHead->next_);
        return ret;
    }

    void Push(T new_value)
    {
        auto data = std::make_shared<T>(std::move(new_value));
        auto p = std::make_unique<node>(new_value); //新的空节点
        m_tail->data_ = data;
        //开始移动补充最后的空节点
        auto* new_tail = p.get();
        m_tail->next_ = std::move(p);
        m_tail = new_tail;
    }
};
```

现在两个操作共享的内存就只有 m_head 和 m_tail 了，而且在 Push 操作中只使用到了共享内存
m_tail，那么接下来的并发安全实现可以开始细粒度化了，我们用两个互斥锁来实现它。一个互斥锁用于锁住访问m_head的行为，一个用于锁住访问m_tail的行为，具体到代码可以因使用时间的长短对临界区进行进一步缩小。

具体代码如下：

```cpp
template<typename T>
class ThreadSafeQueue
{
    struct node
    {
        std::shared_ptr <T> data;
        std::unique_ptr <node> next;
    };
    std::mutex m_headMtx;
    std::unique_ptr <node> m_head;
    std::mutex m_tailMtx;
    node *m_tail;

public:
    ThreadSafeQueue() :
            m_head(new node), m_tail(m_head.get())
    {}

    ThreadSafeQueue(const ThreadSafeQueue &other) = delete;

    ThreadSafeQueue &operator=(const ThreadSafeQueue &other) = delete;

    std::shared_ptr <T> TryPop()
    {
        std::unique_ptr <node> old_head = pop_head();
        return old_head ? old_head->data : std::shared_ptr<T>();
    }

    void Push(T new_value)
    {
        std::shared_ptr <T> new_data(
                std::make_shared<T>(std::move(new_value)));
        std::unique_ptr <node> p(new node);
        node *const new_tail = p.get();
        
        //开始锁临界区
        std::lock_guard <std::mutex> tail_lock(m_tailMtx);
        m_tail->data = new_data;
        m_tail->next = std::move(p);
        m_tail = new_tail;
    }

private:
    
    node *get_tail()
    {
        std::lock_guard <std::mutex> tail_lock(m_tailMtx);
        return m_tail;
    }

    std::unique_ptr <node> pop_head()
    {
        //这里head一定要先被锁
        std::lock_guard <std::mutex> head_lock(m_headMtx);
        if (m_head.get() == get_tail())
        {
            return nullptr;
        }
        std::unique_ptr <node> old_head = std::move(m_head);
        m_head = std::move(old_head->next);
        return old_head;
    }
};
```

注意：
当get_tail()调用前，请确保 m_headMtx 已经上锁，这一步也是很重要的哦。如果不这样，调用pop_head()时，就无法确保 get_tail
得到的数据在使用的时候为最新，如下代码，如果进入head_lock临界区后，old_tail被其他线程改了，那么整个操作就不对了。

```cpp
std::unique_ptr<node> pop_head() // 这是个有缺陷的实现
{
  node* const old_tail=get_tail();  // ① 在m_headMtx范围外获取旧尾节点的值
  std::lock_guard<std::mutex> head_lock(head_mutex);

  if(head.get()==old_tail)  // ②
  {
    return nullptr;
  }
  std::unique_ptr<node> old_head=std::move(head);
  head=std::move(old_head->next);  // ③
  return old_head;
}
```

再来看看异常安全是否有保证，如果TryPop() 中的对锁的操作会产生异常，由于直到锁获取后才能对数据进行修改。因此，TryPop()是异常安全的。另一方面，Push()
可以在堆上新分配出一个T的实例，以及一个node的新实例，这里可能会抛出异常。但是，所有分配的对象都赋给了智能指针，那么当异常发生时，他们就会被释放掉。

**并发度分析**

TryPop()持有m_tailMtx也只有很短的时间，只为保护对tail的读取。因此，当有数据push进队列后，TryPop()
几乎及可以完全并发调用了。同样在执行中，对m_headMtx的持有时间也是极短的。当并发访问时，会增加对TryPop()的访问次数；由于只有一个线程，在同一时间内可以访问pop_head()
，且多线程情况下可以删除队列中的旧节点，并且安全的返回数据。

### 添加条件变量实现同步等待

现在已经实现了细粒度锁的线程安全队列，不过只有TryPop()可以并发访问(且只有一个重载存在)。那么方便的同步的WaitAndPop()呢？

#### Push实现

向队列中添加新节点是相当简单的——下面的实现与上面的代码差不多。

```cpp
template<typename T>
void ThreadSafe<T>::Push(T new_value)
{
  auto new_data = std::make_shared<T>(std::move(new_value));
  std::unique_ptr<node> p(new node);
  {//生产临界区
    std::lock_guard<std::mutex> tail_lock(tail_mutex);
    tail->data=new_data;
    auto* new_tail=p.get();
    tail->next=std::move(p);
    tail=new_tail;
  }
  data_cond.notify_one();
}
```

#### WaitAndPop实现

```cpp
template<typename T>
class ThreadSafeQueue
{
private:
  node* get_tail()
  {
    std::lock_guard<std::mutex> tail_lock(tail_mutex);
    return tail;
  }

  std::unique_ptr<node> pop_head()  // 1
  {
    std::unique_ptr<node> old_head=std::move(head);
    head=std::move(old_head->next);
    return old_head;
  }

  std::unique_lock<std::mutex> wait_for_data()  // 2
  {
    std::unique_lock<std::mutex> head_lock(head_mutex);
    data_cond.wait(head_lock,[&]{return head.get()!=get_tail();});
    return std::move(head_lock);  // 3
  }

  std::unique_ptr<node> wait_pop_head()
  {
    std::unique_lock<std::mutex> head_lock(wait_for_data());  // 4
    return pop_head();
  }

  std::unique_ptr<node> wait_pop_head(T& value)
  {
    std::unique_lock<std::mutex> head_lock(wait_for_data());  // 5
    value=std::move(*head->data);
    return pop_head();
  }
public:
  std::shared_ptr<T> WaitAndPop()
  {
    std::unique_ptr<node> const old_head=wait_pop_head();
    return old_head->data;
  }

  void WaitAndPop(T& value)
  {
    auto _ = wait_pop_head(value);
  }
};
```

可能大家看到代码好像有点多，实际上都只是为了代码的重用，例如pop_head()①和wait_for_data()②，这些函数分别是删除头结点和等待队列中有数据弹出的。wait_for_data()
特别值得关注，因为其不仅等待使用lambda函数对条件变量进行等待，而且它还会将锁的实例返回给调用者③。这就确保了wait_pop_head的线程安全。pop_head()是对TryPop()代码的复用。

#### TryPop和Empty实现

```cpp
template<typename T>
class ThreadSafeQueue
{
private:
  std::unique_ptr<node> try_pop_head()
  {
    std::lock_guard<std::mutex> head_lock(head_mutex);
    if(head.get()==get_tail())
    {
      return std::unique_ptr<node>();
    }
    return pop_head();
  }

  std::unique_ptr<node> try_pop_head(T& value)
  {
    std::lock_guard<std::mutex> head_lock(head_mutex);
    if(head.get()==get_tail())
    {
      return std::unique_ptr<node>();
    }
    value=std::move(*head->data);
    return pop_head();
  }
public:
  std::shared_ptr<T> TryPop()
  {
    std::unique_ptr<node> old_head=try_pop_head();
    return old_head?old_head->data:std::shared_ptr<T>();
  }

  bool TryPop(T& value)
  {
    std::unique_ptr<node> const old_head=try_pop_head(value);
    return old_head;
  }

  void Empty()
  {
    std::lock_guard<std::mutex> head_lock(head_mutex);
    return (head.get()==get_tail());
  }
};
```

## 简单测试

* 一个生产者，三个消费者，数据量15000

| 次数   | v1版本 | v3版本 | v3原始指针版 |
| ------ | ------ | ------ | ------------ |
| 第一次 | 7.31ms | 8.51   | 8.53         |
| 第二次 | 6.61ms | 9.26   | 7.25         |
| 第三次 | 7.60ms | 8.90   | 7.95         |

* 一个生产者，三个同时消费生产，一个消费，数据量1500000

| 次数   | v1版本 | v3版本 | v3原始指针版 |
| ------ | ------ | ------ | ------------ |
| 第一次 | 397.90 | 爆栈   | 399.68       |
| 第二次 | 398.0  | 爆栈   | 362.06       |
| 第三次 | 319.28 | 爆栈   | 355.62       |

>
我的测试仅限于少量线程，而且任务负担也不重，故测出来的结果竟然是直接封装标准库的队列性能最好（都是在release模式下），我猜大概率是标准库的内存分配器优于我这个简单的内存管理，再加上我测试的线程数量非常少，细粒度的锁并未体现出它的优势。。。

## 完整代码

代码仓库：[thread_safe_queue](https://github.com/ACking-you/MyUtil/tree/master/threadsafe_queue)

