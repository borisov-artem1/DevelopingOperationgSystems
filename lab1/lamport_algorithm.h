#pragma once
#include <thread>
#include <vector>
#define MFENCE() asm volatile("mfence" ::: "memory")
//#define MFENCE() { __asm__("mfence" ::: "memory"); }
//#define MEMORY_BARRIER() __sync_synchronize()

static constexpr long long NUM_THREADS = 10;

class l_mutex
{
public:
    l_mutex()
    {
        for (unsigned int i = 0; i < m_amount_of_threads; ++i)
        {
            m_choosing[i] = false;
            m_number[i] = 0;
        }
    }
    void lock(int);
    void unlock(int);
private:
    bool m_choosing[NUM_THREADS];
    int m_number[NUM_THREADS];
    unsigned int m_amount_of_threads = NUM_THREADS;
};

inline void l_mutex::lock(int thread_id)
{
    m_choosing[thread_id] = true;
    MFENCE();
    //MEMORY_BARRIER();
    int max_num = 0;

    for (unsigned int i = 0; i < m_amount_of_threads; ++i)
    {
        max_num = std::max(max_num, m_number[i]);
    }


    m_number[thread_id] = max_num + 1;
    MFENCE();
    //MEMORY_BARRIER();
    m_choosing[thread_id] = false;
    //MEMORY_BARRIER();
    MFENCE();
    for (int j = 0; j < m_amount_of_threads; ++j)
    {
        if (j == thread_id) continue;

        while (m_choosing[j]);

        while (m_number[j] != 0 &&
            (m_number[j] < m_number[thread_id] ||
            (m_number[j] == m_number[thread_id] && j < thread_id)));
    }
}

inline void l_mutex::unlock(int thread_id)
{
    m_number[thread_id] = 0;
    MFENCE();
    //MEMORY_BARRIER();
}