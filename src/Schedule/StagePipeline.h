#pragma once
#include "Pass/Pass.h"
#include "Resource/DisplayResource.h"
#include "Resource/BackendResource.h"
#include <future>
#include <queue>
#include <thread>

class StagePipeline {
public:
    struct TaskInfo {
        uint backendIndex;
        uint frameIndex;
        uint64 copyFenceValue;
        GBufferPass gBufferPass;
        LightPass lightPass;
        SobelPass sobelPass;
        QuadPass quadPass;
    };
    StagePipeline(uint threadCount, size_t stageCount);
    ~StagePipeline();

    void Stop();

    std::vector<std::function<void(TaskInfo)>> stageTasks;
    std::vector<std::queue<TaskInfo>> stageQueues;
    std::vector<std::thread> threads;
    std::atomic_uint currentWorkNum = 0;

    std::vector<std::mutex> stageMutexes;
    std::vector<std::condition_variable> stageConditionVariables;
    std::vector<std::atomic_bool> stopFlags;

    void SubmitResource(TaskInfo info);
    uint GetWorkNum() const { return currentWorkNum; }

    template <class F>
    void SetStageTask(uint stageIndex, F &&func)
    {
        stageTasks.at(stageIndex) = std::bind(std::forward<F>(func), std::placeholders::_1);
    }

};

inline StagePipeline::StagePipeline(uint threadCount, size_t stageCount) :
    stageMutexes(stageCount), stageConditionVariables(stageCount),
    stageTasks(stageCount), stopFlags(stageCount),stageQueues(3)
{
    for (uint i = 0; i < threadCount; i++) {
        uint stageIndex = i % stageCount;
        threads.emplace_back([this, stageIndex,stageCount]() {
            while (true) {
                std::unique_lock<std::mutex> lock(stageMutexes[stageIndex]);
                stageConditionVariables[stageIndex].wait(lock, [this, stageIndex]() { return !stageQueues[stageIndex].empty() || stopFlags[stageIndex]; });
                if (stopFlags[stageIndex]) {
                    return;
                }
                auto stage = std::move(stageQueues[stageIndex].front());
                stageQueues[stageIndex].pop();
                lock.unlock();

                // TODO : Execute Stage Here
                stageTasks[stageIndex](stage);
                
                if (stageIndex < stageCount-1) {
                    lock.lock();
                    stageQueues[stageIndex + 1].push(stage);
                    lock.unlock();
                    stageConditionVariables[stageIndex + 1].notify_one();
                } else {
                    currentWorkNum--;
                }
            }
        });
    };
}

inline void StagePipeline::SubmitResource(TaskInfo info)
{
    for (auto &func : stageTasks) {
        if (func == nullptr) {
            throw std::exception("StagePipeline::SubmitResource: StageTask is not set");
        }
    }
    std::unique_lock<std::mutex> lock(stageMutexes[0]);
    currentWorkNum++;
    stageQueues[0].push(info);
    stageConditionVariables[0].notify_one();
    lock.unlock();
}


inline StagePipeline::~StagePipeline()
{
    Stop();
}

inline void StagePipeline::Stop()
{
    for (auto &flag : stopFlags) {
        flag = true;
    }
    for (auto &cv : stageConditionVariables) {
        cv.notify_all();
    }
    for (auto &thread : threads) {
        if (thread.joinable()) { // 可能还没到joinable的时候就已经结束了,j
            thread.join();
        }
    }
}
