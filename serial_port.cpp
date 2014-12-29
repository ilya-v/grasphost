#include "serial_port.h"
#include "serial_port_win.h"
#include "serial_port_mac.h"
#include "utils.h"

#include <memory>

#pragma message "serial_port.cpp compiling"

SerialPort::SerialPort() : p_impl(make_unique<SerialPort::Impl>()) {}
SerialPort::~SerialPort() {}

void  SerialPort::RestartInit(const char *port_name, std::function<void()> f_restart) { p_impl->RestartInit(port_name, f_restart); }
void  SerialPort::Close() { p_impl->Close(); }
void  SerialPort::Write(const unsigned char * data, const unsigned size) { p_impl->Write(data, size); }
void  SerialPort::Read(unsigned char * data, const unsigned size) { p_impl->Read(data, size); }

