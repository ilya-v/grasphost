#if defined(__APPLE__) && defined(__MACH__)

#include <ApplicationServices/ApplicationServices.h>

//http://web.archive.org/web/20100501161453/http://www.classicteck.com/rbarticles/mackeyboard.php

namespace {
    void simulate_key_event_mac(const unsigned key_code, const bool is_push) {

        CGEventSourceRef source = CGEventSourceCreate(kCGEventSourceStateCombinedSessionState);
        CGEventRef cmd = CGEventCreateKeyboardEvent(source, (CGKeyCode)key_code, is_push);
        CGEventPost(kCGAnnotatedSessionEventTap, cmd);
        CFRelease(cmd);
        CFRelease(source);
    }
}

void simulate_key_press(const unsigned key_code) { simulate_key_event_mac(key_code, true); }
void simulate_key_release(const unsigned key_code) { simulate_key_event_mac(key_code, false); }

#endif