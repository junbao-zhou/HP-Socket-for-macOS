#include "timerpipe.h"
#include "./common/GeneralHelper.h"

TimerPipe* TimerPipe::Create(uint64_t tDelayTime, uint64_t tInterval, char cFlag){
    MessagePipe* p = MessagePipe::Create();
    if(p){
        TimerPipe* timer =  new TimerPipe(tInterval, p);
        timer->Start(tDelayTime, cFlag);
        return timer;
    }
    return nullptr;
}

TimerPipe::TimerPipe(uint64_t tVal, MessagePipe* p)
    : m_tSpaceTime(tVal), m_pPipe(p), m_pStopPipe(nullptr){

}

TimerPipe::~TimerPipe(){
    Stop();

    if(m_thTimer.joinable()){
        m_thTimer.join();
    }

    if(m_pPipe)
        delete m_pPipe;

    if(m_pStopPipe)
        delete m_pStopPipe;
}

int TimerPipe::GetReadFd(){
    ASSERT(m_pPipe);
    return m_pPipe->GetReadFd();
}

int TimerPipe::GetWriteFd(){
    ASSERT(m_pPipe);
    return m_pPipe->GetWriteFd();
}

void TimerPipe::Stop(){
    ASSERT(m_pStopPipe);
    if(!m_pStopPipe->Write("1", 1))
        ASSERT(FALSE);
}

void TimerPipe::Start(uint64_t tDelayTime, char cFlag){
    ASSERT(m_pPipe);
    ASSERT(m_pStopPipe == nullptr);

    m_pStopPipe = MessagePipe::Create();
    m_thTimer = std::thread([this, cFlag, tDelayTime](){
        char cVal = cFlag;

        pollfd pfd = {m_pStopPipe->GetReadFd(), POLLIN};

        timeval spaceTime;
        spaceTime.tv_sec = m_tSpaceTime / std::milli::den;
        spaceTime.tv_usec = (m_tSpaceTime % std::milli::den) * 1000;

        int nLen = 0;
        uint64_t tRunTime = ::TimeGetTime64() + tDelayTime;
        while(true){
            if(tRunTime > ::TimeGetTime64()){
                std::this_thread::sleep_for(std::chrono::milliseconds(tDelayTime + 10));
                continue;
            }

            if(TIMEOUT == (nLen = (int)::PollForSingleObject(pfd, m_tSpaceTime))){
                if(!m_pPipe->Write(&cVal, 1)){
                    ASSERT(FALSE);
                }
            }else if(nLen > 0){
                break;
            }else{
                ASSERT(FALSE);
            }
            if(m_tSpaceTime == 0)
                break;
        }
    });
}
