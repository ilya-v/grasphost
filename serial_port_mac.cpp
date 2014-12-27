#include "serial_port_mac.h"

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
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/serial/IOSerialKeys.h>
#include <IOKit/serial/ioss.h>
#include <IOKit/IOBSD.h>

std::vector<std::string> ScanSerialPorts()
{
    CFMutableDictionaryRef classesToMatch = IOServiceMatching(kIOSerialBSDServiceValue);
    ENSURE(classesToMatch, "IOServiceMatching returned a NULL dictionary");

    CFDictionarySetValue(classesToMatch, CFSTR(kIOSerialBSDTypeKey), CFSTR(kIOSerialBSDAllTypes));


    io_iterator_t matchingServices;
    const kern_return_t res = IOServiceGetMatchingServices(kIOMasterPortDefault, classesToMatch, matchingServices);
    ENSURE(KERN_SUCCESS == kernResult, "IOServiceGetMatchingServices returned " + to_string(res));

    std::vector<std::string> ports_found;
    io_object_t		modemService;
    for (; (modemService = IOIteratorNext(serialPortIterator)); ) {
        CFTypeRef bsdPathAsCFString = IORegistryEntryCreateCFProperty(modemService, CFSTR(kIOCalloutDeviceKey), kCFAllocatorDefault, 0);
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


#endif
#endif
