#ifndef _serial_port_win__h_
#define _serial_port_win__h_

#if defined(_WIN32)

#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "serial_port.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

struct SerialPort :: Impl
{
    void  RestartInit(const char *port_name, std::function<void()> f_restart);
    void  Close();

    void  Write(const unsigned char * data, const unsigned size);
    void  Read(unsigned char * data, const unsigned size);

private:
    HANDLE  myHandle;
    static HANDLE PersistentOpen(const char *port_name);
};

#endif
#endif
