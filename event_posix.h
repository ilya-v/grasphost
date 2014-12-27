#ifndef _event_posix__h__
#define _event_posix__h__

#if defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__))
#include <unistd.h>
#if defined _POSIX_VERSION

#include "event.h"

#include <sys/types.h>
#inculde <sys/socket.h>
#include "utils.h"

struct Event::Impl
{
    Event::Impl() { ENSURE(0 == socketpair(AF_UNIX, SOCK_STREAM, 0, fd_socket), "Cannot create a POSIX socket"); }
    void Signal() { for (unsigned char n = 1; EINTR == send(fd_socket[0], &n, 1, MSG_DONTWAIT);); }
    void Wait()   { unsigned char n; recv(fd_socket[1], &n, 1, 0); }
    ~Impl()       { close(fd_socket[0]); close(fd_socket[1]); }
private:    
    int fd_socket[2] = {0, 0};
};


#endif
#endif
#endif