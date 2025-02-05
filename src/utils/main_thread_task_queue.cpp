//
// Created by Gianni on 4/02/2025.
//

#include "main_thread_task_queue.hpp"

void MainThreadTaskQueue::push(Task &&task)
{
    std::lock_guard<std::mutex> lock(mTaskQueueMutex);
    mTaskQueue.push_back(std::move(task));
}

std::optional<Task> MainThreadTaskQueue::pop()
{
    std::lock_guard<std::mutex> lock(mTaskQueueMutex);

    if (!mTaskQueue.empty())
    {
        Task task = std::move(mTaskQueue.front());
        mTaskQueue.pop_front();
        return task;
    }

    return std::nullopt;
}
