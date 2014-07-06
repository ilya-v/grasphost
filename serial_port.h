#ifndef SERIAL_PORT__H__
#define SERIAL_PORT__H__

#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <windows.h>
#include <functional>
#include <vector>

std::vector<std::string> ScanSerialPorts();

struct SerialPort
{
    void  RestartInit(const char *port_name, std::function<void()> f_restart);
    void  Close();

    void  Write(const unsigned char * data, const unsigned size);
    void  Read (      unsigned char * data, const unsigned size);

    struct Exception {};

private:
    HANDLE  myHandle;
    static HANDLE PersistentOpen(const char *port_name);    
};


#endif