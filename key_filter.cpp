#include "key_filter.h"
#include "key_event.h"
#include <algorithm>

namespace
{
    std::array<bool, 5> key_states{};
    std::array<unsigned, 5>  prev_levels{};
    bool  are_prev_levels_set  =  false;
}


namespace {
    int max(int a, int b) { return a > b ? a : b; }

    const int
        LT[5] = { 10, 10, 15, 15, 15 },
        DT[5] = { 3, 4, 4, 5, 5 };

#define FORALL for(unsigned i = 0; i < 5; i++)

    void filter_keys(const int adc[5], int pressed[5])
    {
        static int
            PF_set = 0,                 // are forces from the previous iteration set
            PF[5] = { 0, 0, 0, 0, 0 };  // forces from previous iteration; stored between calls to this function

        int F[5];// F is array of pressure forces on each sensor, with elements in range of [0, 127]
        FORALL F[i] = 0x7F - adc[i];

        int D[5] = { 0, 0, 0, 0, 0 };   // Delta between the current forces (F) and previous step forces (PF)
        if (PF_set)                     //only calculate Delta if the previous forces are set
            FORALL D[i] = F[i] - PF[i];

        int S[5] = { 0, 0, 0, 0, 0 };   // Suppress flag defines which detected key presses to suppress 
        {
            const int d2 = max(D[2] - DT[2], 0), d3 = max(D[3] - DT[3], 0), d4 = max(D[4] - DT[4], 0);
            const int dmax = max(d2, max(d3, d4));

            if (dmax >= 0) {  // from the sensors 2, 3 and 4 (starting from 0), only one counts as pressed: the one with the maximum Delta
                S[2] = S[3] = S[4] = 1;
                if (dmax == d4)  S[4] = 0; else if (dmax == d3) S[3] = 0; else S[2] = 0;
            }
        }

        FORALL pressed[i] = pressed[i] || (D[i] > DT[i] && !S[i]);
        FORALL pressed[i] = pressed[i] && F[i] > LT[i];

        FORALL PF[i] = F[i];            // store current forces for the next iteration
        PF_set = 1;                     // "previous forces" were set for the next iteration
    }
}

std::array<unsigned, 10>  process_sensor_levels(const config_t & config, const uint8_t levels[5])
{
    const int adc[5] = { levels[0], levels[1], levels[2], levels[3], levels[4] };
    int pressed[5] = { 0, };
    filter_keys(adc, pressed);    
    
    std::array < unsigned, 5 >
        filtered_levels = {},
        original_levels = {};
    for (unsigned i = 0; i < 5; i++) original_levels[i] = 0x7F - adc[i];
    for (unsigned i = 0; i < 5; i++) filtered_levels[i] = (pressed[i] ? original_levels[i] : 0);
    

    for (unsigned i = 0; i < 5; ++i)
        if (key_states[i] != !! pressed[i])
            ((key_states[i] = !! pressed[i]) ? simulate_key_press : simulate_key_release)(config.key_codes[i]);

    std::array<unsigned, 10>  result{};
    std::copy_n(original_levels.begin(), 5, result.begin() + 0);
    std::copy_n(filtered_levels.begin(), 5, result.begin() + 5);
    return  result;
}
