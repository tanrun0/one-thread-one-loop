#include <iostream>
#include <functional>
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>
#include <unistd.h>

// 设计一个时间轮
// 作用：用来执行定时任务（定时指：超过一定时间该连接无其他任务产生时，自动执行的回调任务）
// 1. 每走一步代表一个时刻, 指针走到哪里, 就执行哪里的定时任务
// 2. 如果一连接在原来的定时任务前又发生了，就要重新刷新该连接的 "定时任务" 的时间位置

using TaskFunc = std::function<void()>;
using ReleaseFunc = std::function<void()>;
class TimeTask // 每个连接的超时定时任务
{
private:
    uint64_t _id;      // 连接 id
    uint32_t _outtime; // 定时时间
    TaskFunc _task_cb; // 定时任务回调函数
    ReleaseFunc _release;
    bool _cancel; // false -> 不取消 ; true -> 取消

public:
    TimeTask(uint64_t id, uint32_t delay, TaskFunc cb)
        : _id(id), _outtime(delay), _task_cb(cb), _cancel(false) {}

    void SetRelease(const ReleaseFunc &cb) // 从 _search 删，需要回调
    {
        _release = cb;
    }
    void Cancel()
    {
        _cancel = true;
    }
    uint32_t GetDelay()
    {
        return _outtime;
    }
    ~TimeTask()
    {
        if (_cancel == false)
            _task_cb(); // 执行定时任务
        _release();     // 把 weak_ptr 从 _search 里面删除
    }
};

// 时间轮
class TimeWheel
{
    // 通过 shared_ptr 计数归零 时自动调用析构  ->  （把定时任务放在析构函数里）通过对引用计数的操作，来控制定时任务的执行时机
    using TaskPtr = std::shared_ptr<TimeTask>;

    // 连接又有事件发生时，要通过事件id找到它对应的shared_ptr, 然后把这个shared_ptr插在更新后的定时任务执行的位置
    // 这样就算 tick 指到了原来的定时任务，也不会执行，因为只有引用计数到 0 才会执行
    // 但是搜索表不能直接存储share_ptr, 因为会导致：这个对象永远引用计数不为 0, 永远不会被销毁调析构
    // 我们可以用 weak_ptr 存，这样不会徒增引用计数，能确保引用计数正常 减为 0
    // 但是要注意：当连接释放的时候，要从搜索表里删除 weak_ptr，否则会资源泄漏
    using TaskWeakPtr = std::weak_ptr<TimeTask>;

private:
    int _capacity;                                     // 时间轮的时间大小
    int _tick;                                         // 时间指针
    std::vector<std::vector<TaskPtr>> _wheel;          // 二维数组, 同一个时刻上可能存在多个要执行的定时任务
    std::unordered_map<uint64_t, TaskWeakPtr> _timers; // 存放定时任务信息

private:
    void RemoveTimer(uint64_t id)
    { // 移除到时间的连接
        auto it = _timers.find(id);
        if (it != _timers.end())
            _timers.erase(it);
    }

public:
    TimeWheel()
        : _capacity(60), _tick(0), _wheel(_capacity)
    {
    }
    void AddTimer(uint64_t id, uint32_t delay, TaskFunc cb)
    {
        TaskPtr pt(new TimeTask(id, delay, cb));
        pt->SetRelease(std::bind(&TimeWheel::RemoveTimer, this, id)); // this 是 RemoverTimer 的第一个隐藏参数
        int pos = (_tick + delay) % _capacity;                        // 设置定时任务执行位置
        _wheel[pos].emplace_back(pt);
        _timers[id] = TaskWeakPtr(pt); // 用 share_ptr 构造一个 weak_ptr
    }

    // 刷新定时任务时间的接口（甭管它什么时候调用，怎么判断要刷新的，这里只提供一个刷新接口）
    void RefreshTimer(uint64_t id)
    {
        auto it = _timers.find(id);
        if (it == _timers.end())
            return;
        TaskPtr pt = _timers[id].lock(); // weak_ptr 调用 lock() 得到 shared_ptr
        int delay = pt->GetDelay();
        int pos = (_tick + delay) % _capacity;
        _wheel[pos].emplace_back(pt);
    }
    void CancelTimer(uint64_t id)
    {
        auto it = _timers.find(id);
        if (it == _timers.end())
            return; // 没找到
        TaskPtr pt = it->second.lock();
        pt->Cancel();
    }
    void RunTick()
    {
        _tick = (_tick + 1) % _capacity;
        _wheel[_tick].clear(); // 把该时刻的定时任务全部 "启动" (即：全释放，调用析构)
    }
};

class Test // 设置一个定时任务
{
public:
    Test()
    {
        std::cout << "构造" << std::endl;
    }
    ~Test()
    {
        std::cout << "析构" << std::endl;
    }
};
void DelTest(Test *t) // 定时任务
{
    delete t;
}
int main()
{
    TimeWheel wt;
    Test *pt = new Test();
    wt.AddTimer(888, 3, std::bind(DelTest, pt)); // 添加一个三秒后释放 Test 的定时任务
    for (int i = 0; i < 3; i++)
    {
        sleep(1);
        wt.RefreshTimer(888);
        wt.RunTick();
        std::cout << "刷新了定时任务" << std::endl;
    }
    wt.CancelTimer(888);
    while (1)
    {
        sleep(1);
        std::cout << "--------------" << std::endl;  // 用来体现时间流逝
        wt.RunTick();
    }
    return 0;
}
