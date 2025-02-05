//
// Created by Gianni on 4/02/2025.
//

#ifndef VULKANRENDERINGENGINE_MAIN_THREAD_TASK_QUEUE_HPP
#define VULKANRENDERINGENGINE_MAIN_THREAD_TASK_QUEUE_HPP

using Task = std::function<void()>;

class MainThreadTaskQueue
{
public:
    void push(Task&& task);
    std::optional<Task> pop();

private:
    std::deque<Task> mTaskQueue;
    std::mutex mTaskQueueMutex;
};

#endif //VULKANRENDERINGENGINE_MAIN_THREAD_TASK_QUEUE_HPP
