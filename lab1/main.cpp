#include <iostream>
#include "peterson_algorithm.h"
#include "lamport_algorithm.h"


static constexpr long long NUM_ITERATIONS = 1e4;
static constexpr long long NUMBER_OF_DELAYS = 16;

static size_t amount = 0;
static std::atomic<size_t> errors{0};
static std::atomic start{false};
static std::atomic<size_t> next_delay_index{0};

static constexpr uint32_t delays[] = {
    100, 50, 500, 100000, 3003, 4905, 32930, 94,
    500000, 1999, 3009, 12994, 3584, 1233, 8888, 1000000
};

static volatile uint64_t busy_sink = 0;

inline void busy_loop(uint32_t iterations)
{
    for (uint32_t i = 0; i < iterations; ++i)
    {
        busy_sink += i ^ (busy_sink & 0xFF);
    }
}

template <typename Mutex>
requires std::is_same_v<Mutex, p_mutex> || std::is_same_v<Mutex, l_mutex>
void thread_function(Mutex& mutex, int thread_id)
{
    while (!start.load(std::memory_order_acquire))
    {
        std::this_thread::yield();
    }

    for (size_t i = 0; i < NUM_ITERATIONS; i++)
    {
        mutex.lock(thread_id);

        if ((amount % 2) != 0)
        {
            errors.fetch_add(1, std::memory_order_relaxed);
        }

        amount += 1;

        size_t index = next_delay_index.fetch_add(1, std::memory_order_relaxed) % NUMBER_OF_DELAYS;
        uint32_t delay = delays[index];

        busy_loop(delay);

        if ((amount % 2) == 0)
        {
            errors.fetch_add(1, std::memory_order_relaxed);
        }

        amount += 1;
        mutex.unlock(thread_id);
    }

}

int main()
{
    std::cout << "\n\tPeterson Algorithm Test" << std::endl;
    //std::this_thread::sleep_for(std::chrono::seconds(1));
    {
        int thread_indexes[] = {0, 1};
        p_mutex mutex;
        auto thread1 = std::jthread(thread_function<p_mutex>, std::ref(mutex), thread_indexes[0]);
        auto thread2 = std::jthread(thread_function<p_mutex>, std::ref(mutex), thread_indexes[1]);
        start.store(true, std::memory_order_release);
    }
    std::cout << "Amount of iterations: " << amount << " Errors: " << errors << std::endl;

    std::cout << "\n\tLamport Algorithm Test" << std::endl;
    start.store(false, std::memory_order_release);
    l_mutex mutex;
    amount = 0;
    errors.store(0, std::memory_order_release);
    next_delay_index.store(0, std::memory_order_release);
    std::vector<std::thread> threads;
    for (unsigned int i = 0; i < NUM_THREADS; i++)
    {
        threads.emplace_back(thread_function<l_mutex>, std::ref(mutex), i);
    }

    start.store(true, std::memory_order_release);
    for (auto& thread : threads)
        thread.join();

    std::cout << "\nAmount of iterations: " << amount << " Errors: " << errors << std::endl;
}