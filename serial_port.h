#ifndef SERIAL_PORT__H__
#define SERIAL_PORT__H__


#include <functional>
#include <vector>
#include <memory>

std::vector<std::string> ScanSerialPorts();

struct SerialPort
{
    SerialPort();
    ~SerialPort();

    void  RestartInit(const char *port_name, std::function<void()> f_restart);
    void  Close();

    void  Write(const unsigned char * data, const unsigned size);
    void  Read (      unsigned char * data, const unsigned size);

private:
    struct Impl;
    std::unique_ptr<Impl> p_impl;
};

#endif
