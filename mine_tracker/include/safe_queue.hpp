#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>
#include <utility>

namespace mine_tracker {

template <typename T>
class SafeQueue {
    public:
        SafeQueue() = default;
        ~SafeQueue() {
            stop();
        }

        SafeQueue(const SafeQueue&) = delete;
        SafeQueue& operator=(const SafeQueue&) = delete;

        void push(T value) {
            {
                std::lock_guard<std::mutex> lock(_mtx);
                _queue.push(std::move(value));
            }
            _cv.notify_one();
        }

        bool pop(T& value) {
            std::unique_lock<std::mutex> lock(_mtx);
            _cv.wait(lock, [this] { return !_queue.empty() || _stop; });
            
            if (_stop && _queue.empty()) {
                return false;
            }

            value = std::move(_queue.front());
            _queue.pop();
            return true;
        }

        void stop() {
            {
                std::lock_guard<std::mutex> lock(_mtx);
                _stop = true;
            }
            _cv.notify_all();
        }

        bool empty() const {
            std::lock_guard<std::mutex> lock(_mtx);
            return _queue.empty();
        }

        size_t size() const {
            std::lock_guard<std::mutex> lock(_mtx);
            return _queue.size();
        }

    private:
        std::queue<T> _queue;
        mutable std::mutex _mtx;
        std::condition_variable _cv;
        bool _stop{false};
    };
} // namespace mine_tracker