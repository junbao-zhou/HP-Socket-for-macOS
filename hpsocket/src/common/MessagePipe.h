#ifndef MESSAGEPIPE_H
#define MESSAGEPIPE_H
#include <sys/pipe.h>
#include <unistd.h>
#include "Singleton.h"

class MessagePipe
{
public:
    static MessagePipe* Create();
    static int Write(int __fd, const char *__buf, size_t __nbyte);
    static int Read(int __fd, char *__buf, size_t __nbyte);

    static int Write(MessagePipe& p, const char* __buf, size_t __nbyte);
    static int Read(MessagePipe& p, char* __buf, size_t __nbyte);
public:
    ~MessagePipe();
    int GetReadFd();
    int GetWriteFd();
    int Read(char *__buf, size_t __nbyte);
    int Write(const char *__buf, size_t __nbyte);
private:
    MessagePipe(int rFd, int wFd);

private:
    int m_nReadFd;
    int m_nWriteFd;

    DECLARE_NO_COPY_CLASS(MessagePipe)
};

#endif // MESSAGEPIPE_H
