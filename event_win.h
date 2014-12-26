#ifndef __event_win__h__
#define __event_win__h__

#ifdef _WIN32 

#include "event.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

struct Event::Impl 
{
    void Signal() { SetEvent(h_event); }
    void Wait()   { WaitForSingleObject(h_event, INFINITE); }
    ~Impl()       { CloseHandle(h_event); }
private:
    HANDLE h_event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
};

#endif
#endif
