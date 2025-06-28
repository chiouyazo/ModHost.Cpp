#include <condition_variable>
#include <mutex>

#include "headers/ModHostBridge.h"

int main() {
    ModHostBridge bridge(63982, "examplecpp");

    std::mutex mtx;
    std::unique_lock lock(mtx);
    std::condition_variable cv;
    cv.wait(lock);
    return 0;
}
