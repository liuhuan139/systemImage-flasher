#include "thread_worker.hpp"

ThreadWorker::ThreadWorker()
    : m_running(true)
{
    m_dispatcher.connect(sigc::mem_fun(*this, &ThreadWorker::on_dispatcher_emit));
    m_thread = std::thread(&ThreadWorker::worker_thread_func, this);
}

ThreadWorker::~ThreadWorker() {
    cancel();
    if (m_thread.joinable()) {
        m_thread.join();
    }
}

void ThreadWorker::run_async(Task task, ResultCallback callback) {
    std::lock_guard<std::mutex> lock(m_queue_mutex);
    m_task_queue.push({std::move(task), std::move(callback)});
}

void ThreadWorker::cancel() {
    m_running = false;
}

bool ThreadWorker::is_running() const {
    return m_running;
}

void ThreadWorker::worker_thread_func() {
    while (m_running) {
        QueuedTask queued_task;
        {
            std::lock_guard<std::mutex> lock(m_queue_mutex);
            if (m_task_queue.empty()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                continue;
            }
            queued_task = std::move(m_task_queue.front());
            m_task_queue.pop();
        }

        bool success = true;
        std::string error_message;
        try {
            if (queued_task.task) {
                queued_task.task();
            }
        } catch (const std::exception& e) {
            success = false;
            error_message = e.what();
        } catch (...) {
            success = false;
            error_message = "Unknown error";
        }

        if (queued_task.callback) {
            std::lock_guard<std::mutex> lock(m_result_mutex);
            m_result_queue.push({std::move(queued_task.callback), {success, error_message}});
        }
        m_dispatcher.emit();
    }
}

void ThreadWorker::on_dispatcher_emit() {
    std::queue<std::pair<ResultCallback, std::pair<bool, std::string>>> result_queue;
    {
        std::lock_guard<std::mutex> lock(m_result_mutex);
        result_queue.swap(m_result_queue);
    }

    while (!result_queue.empty()) {
        auto& item = result_queue.front();
        if (item.first) {
            item.first(item.second.first, item.second.second);
        }
        result_queue.pop();
    }
}
