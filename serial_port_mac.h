#ifndef _serial_port_mac__h__
#define _serial_port_mac__h__

#if defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__))
#include <unistd.h>
#if defined _POSIX_VERSION

#include <functional>
#include "serial_port.h"

#pragma message "serial_port_mac.h included"

struct SerialPort::Impl
{
    void  RestartInit(const char *port_name, std::function<void()> f_restart);
    void  Close();

    void  Write(const unsigned char * data, const unsigned size);
    void  Read(unsigned char * data, const unsigned size);

private:
    int fd_port = 0;
    static int PersistentOpen(const char *port_name);
};

#endif
#endif
#endif
