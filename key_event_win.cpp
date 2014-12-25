#ifdef _WIN32 
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace {
    void simulate_key_event_win(const unsigned key_code, const unsigned flags) {
        INPUT input = {};
        input.type = INPUT_KEYBOARD;
        input.ki.wVk = key_code;
        input.ki.dwFlags = flags;
        SendInput(1, &input, sizeof(input));
    }
}

void simulate_key_press(const unsigned key_code) { simulate_key_event_win(key_code, 0); }
void simulate_key_release(const unsigned key_code) { simulate_key_event_win(key_code, KEYEVENTF_KEYUP); }

#endif