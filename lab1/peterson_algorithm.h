#pragma once
#include <thread>
#define MFENCE() asm volatile("mfence" ::: "memory")



class p_mutex
{
public:
    p_mutex() : turn(-1) {}
    void lock(int);
    void unlock(int);
private:
    bool interested[2];
    int turn;
};

inline void p_mutex::lock(int thread)
{
    int other = 1 - thread;
    interested[thread] = true;
    turn = thread;
    MFENCE();
    while (turn == thread && interested[other]);
}

inline void p_mutex::unlock(int thread)
{
    interested[thread] = false;
}