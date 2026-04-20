#ifndef THREAD_WORKER_HPP
#define THREAD_WORKER_HPP

#include <functional>
#include <thread>
#include <mutex>
#include <queue>
#include <atomic>
#include <glibmm/dispatcher.h>

class ThreadWorker {
public:
    using Task = std::function<void()>;
    using ResultCallback = std::function<void(bool, const std::string&)>;

    ThreadWorker();
    ~ThreadWorker();

    void run_async(Task task, ResultCallback callback = nullptr);
    void cancel();
    bool is_running() const;

private:
    void worker_thread_func();
    void on_dispatcher_emit();

    struct QueuedTask {
        Task task;
        ResultCallback callback;
    };

    std::queue<QueuedTask> m_task_queue;
    std::mutex m_queue_mutex;
    std::thread m_thread;
    Glib::Dispatcher m_dispatcher;
    std::atomic<bool> m_running;
    std::queue<std::pair<ResultCallback, std::pair<bool, std::string>>> m_result_queue;
    std::mutex m_result_mutex;
};

#endif // THREAD_WORKER_HPP
