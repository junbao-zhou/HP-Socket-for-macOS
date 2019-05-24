#include "messagepipe.h"
#include <cassert>
#include <unistd.h>

MessagePipe* MessagePipe::Create(){
    int nPipe[2];
    if(pipe(nPipe) != 0)
        return nullptr;

    return new MessagePipe(nPipe[0], nPipe[1]);
}

int MessagePipe::Read(int __fd, char *__buf, size_t __nbyte){
    assert(__buf);
    assert(__nbyte > 0);
    int nByte = __nbyte;
    int nOffset = 0;
    int nLen = 0;
    while(nByte > 0
          && nOffset < __nbyte
          && 0 < (nLen = read(__fd, &__buf[nOffset], nByte))){
        nOffset+= nLen;
        nByte -= nLen;
    }

    return nOffset == 0 ? -1 : nOffset;
}

int MessagePipe::Write(int __fd, const char *__buf, size_t __nbyte){
    assert(__buf);
    assert(__nbyte > 0);

    int nByte = __nbyte;
    int nOffset = 0;
    int nLen = 0;
    while(nByte > 0
          && nOffset < __nbyte
          && 0 < (nLen = write(__fd, &__buf[nOffset], nByte))){
        nOffset += nLen;
        nByte -= nLen;
    }

    return nOffset == 0 ? -1 : nOffset;
}

int MessagePipe::Write(MessagePipe& p, const char* __buf, size_t __nbyte){
    return MessagePipe::Write(p.GetWriteFd(), __buf, __nbyte);
}

int MessagePipe::Read(MessagePipe& p, char* __buf, size_t __nbyte){
    return MessagePipe::Read(p.GetReadFd(), __buf, __nbyte);
}

///////////////////////////////////////////////////////////////////////////

int MessagePipe::Read(char *__buf, size_t __nbyte){
    return MessagePipe::Read(*this, __buf, __nbyte);
}

int MessagePipe::Write(const char *__buf, size_t __nbyte){
    return MessagePipe::Write(*this, __buf, __nbyte);
}

MessagePipe::MessagePipe(int rFd, int wFd): m_nReadFd(rFd), m_nWriteFd(wFd)
{
}

MessagePipe::~MessagePipe(){
    if(m_nReadFd != -1)
        close(m_nReadFd);

    if(m_nWriteFd != -1)
        close(m_nWriteFd);
}

int MessagePipe::GetReadFd(){
    return m_nReadFd;
}

int MessagePipe::GetWriteFd(){
    return m_nWriteFd;
}
