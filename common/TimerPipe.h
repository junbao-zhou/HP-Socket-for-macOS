#ifndef TIMERPIPE_H
#define TIMERPIPE_H

#include "MessagePipe.h"
#include <stdint.h>
#include <thread>

class TimerPipe
{
public:
    static TimerPipe* Create(uint64_t tDelayTime, uint64_t tInterval, char cFlag = 0x01);

public:
    ~TimerPipe();
    int GetReadFd();
    int GetWriteFd();
private:
    TimerPipe(uint64_t tVal, MessagePipe* p);
    void Start(uint64_t tDelayTime, char cFlag);
    void Stop();
private:
    uint64_t m_tSpaceTime;
    MessagePipe* m_pPipe; //外部监听
    std::thread m_thTimer;
    MessagePipe* m_pStopPipe; //线程内部监听
};

#endif // TIMERPIPE_H
