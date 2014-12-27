#include "event.h"

#include <memory>
#include "utils.h"

#include "event_win.h"
#include "event_posix.h"

Event::Event() : p_impl(make_unique<Event::Impl>()) {}
Event::~Event() {}

void Event::Signal() { p_impl->Signal(); }
void Event::Wait() { p_impl->Wait();  }
