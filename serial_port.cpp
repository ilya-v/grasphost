#include "serial_port.h"
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <functional>
#include <string>
#include <sstream>
#include <vector>


#define ENSURE(cmd, err) \
    do { \
        if (!(cmd)) { std::cout << "ERROR in " <<  __FUNCTION__ << ":" <<  __LINE__ << ": "<< (err) << std::endl;  \
        throw SerialPort::Exception(); } \
    } while (0);

HANDLE CreateSerialPort(const std::string &serial_port_id)
{
    const auto serial_port_name = std::string("\\\\.\\") + serial_port_id;
    return ::CreateFileA(serial_port_name.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
}

std::vector<std::string> ScanSerialPorts()
{
    std::vector<std::string> port_names;
    for (auto i = 1; i < 100; i++) {
        const auto  port_name = std::string("COM") + std::to_string(i);
        auto h = CreateSerialPort(port_name);
        if (h != INVALID_HANDLE_VALUE)
            port_names.push_back(port_name);
        ::CloseHandle(h);
    }
    return port_names;   
       
}

HANDLE SerialPort :: PersistentOpen(const char *port_name_arg)
{
    std::string serial_port_name = std::string("\\\\.\\") + port_name_arg;
    HANDLE serial_handle = INVALID_HANDLE_VALUE;
    for (int attempt_count = 20; attempt_count > 0 && serial_handle == INVALID_HANDLE_VALUE; attempt_count--)
    {
        ::Sleep(500);
        serial_handle = CreateSerialPort(port_name_arg);
    }
    return  serial_handle;
}


void  SerialPort::RestartInit(const char *port_name, std::function<void()> f_restart)
{
    myHandle = PersistentOpen(port_name);
    f_restart();
    this->Close();
    myHandle = PersistentOpen(port_name);
    ENSURE(myHandle != INVALID_HANDLE_VALUE, "Error opening serial port");
}


void SerialPort :: Close()    {   CloseHandle(myHandle);  }


void SerialPort::Write(const unsigned char * data, const unsigned size)
{
    if (!size)      return;
    DWORD written;
    ENSURE(WriteFile(myHandle, data, size, &written, NULL), std::string("Cannot wrtie to the serial port "));
}


void SerialPort::Read(unsigned char *dest_buf,  const unsigned length)
{
    DWORD nread;
    ENSURE( TRUE == ReadFile(myHandle, dest_buf, length, &nread, NULL), GetLastError());
    ENSURE(nread == length, GetLastError());
}



#undef ENSURE