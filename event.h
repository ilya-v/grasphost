#ifndef _event__h__
#define _event__h__

#include <memory>

struct Event {
    Event();
    ~Event();

    void Signal();
    void Wait();
private:
    struct Impl;
    std::unique_ptr<Impl> p_impl;
};

#endif