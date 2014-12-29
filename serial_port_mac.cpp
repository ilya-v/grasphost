#include "serial_port_mac.h"
#include "utils.h"
#include <thread>

#if defined(__APPLE__) && defined(__MACH__)

#include <vector>
#include <string>

#include <errno.h>

#include <paths.h>
#include <termios.h>
#include <sysexits.h>
#include <sys/param.h>
#include <sys/select.h>
#include <sys/time.h>
#include <time.h>
#include <mach/mach.h>
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/IOTypes.h>
#include <IOKit/serial/IOSerialKeys.h>
#include <IOKit/serial/ioss.h>
#include <IOKit/IOBSD.h>


#include <thread>
#include <chrono>

std::vector<std::string> ScanSerialPorts()
{
    CFMutableDictionaryRef classesToMatch = IOServiceMatching(kIOSerialBSDServiceValue);
    ENSURE(classesToMatch, "IOServiceMatching returned a NULL dictionary");

    CFDictionarySetValue(classesToMatch, CFSTR(kIOSerialBSDTypeKey), CFSTR(kIOSerialBSDAllTypes));


    io_iterator_t matchingServices;
    const kern_return_t res = IOServiceGetMatchingServices(kIOMasterPortDefault, classesToMatch, &matchingServices);
    ENSURE(KERN_SUCCESS == res, "IOServiceGetMatchingServices returned " + to_string(res));

    std::vector<std::string> ports_found;
    io_object_t		modemService;
    for (; (modemService = IOIteratorNext(matchingServices)); ) {
        CFStringRef bsdPathAsCFString = (CFStringRef)IORegistryEntryCreateCFProperty(modemService, CFSTR(kIOCalloutDeviceKey), kCFAllocatorDefault, 0);
        if (!bsdPathAsCFString) continue;

        char bsdPath[MAXPATHLEN];
        const Boolean path_res = CFStringGetCString(bsdPathAsCFString, bsdPath, sizeof(bsdPath), kCFStringEncodingUTF8);
        CFRelease(bsdPathAsCFString);
        (void)IOObjectRelease(modemService);

        if (path_res)
            ports_found.push_back(bsdPath);
    }
    return ports_found;
}

#endif



#if defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__))
#include <unistd.h>
#if defined _POSIX_VERSION

#include <errno.h>
#include <fcntl.h>


void  SerialPort::Impl::RestartInit(const char *port_name, std::function<void()> f_restart)
{
    fd_port = PersistentOpen(port_name);
    f_restart();
    this->Close();
    fd_port = PersistentOpen(port_name);
    ENSURE(fd_port > 0, "Error opening serial port");
}

void  SerialPort::Impl::Close() { if(fd_port > 0) close(fd_port); }

void  SerialPort::Impl::Write(const unsigned char * data, const unsigned size)
{
    unsigned n = 0;
    for (int r =  write(fd_port, data, size); errno == EINTR|| (r >= 0 && n < size); n += std::max(r,0) )
        r = write(fd_port, data + n, size - n);

    ENSURE(n == size, "Error writing to the serial port")
}
void  SerialPort::Impl::Read(unsigned char * data, const unsigned size)
{
    unsigned n = 0;
    for (int r = read(fd_port, data, size); errno == EINTR || (r >= 0 && n < size); n += std::max(r,0) )
        r = read(fd_port, data + n, size - n);
}

int SerialPort::Impl::PersistentOpen(const char *port_name) {
    int fd_port = -1;
    for (int attempt_count = 20; attempt_count > 0 && fd_port < 0; attempt_count--)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        fd_port = open(port_name, O_RDWR | O_NOCTTY | O_NDELAY);
    }
    fcntl(fd_port, F_SETFL, 0);
    return  fd_port;
}


#endif
#endif
