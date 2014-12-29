#include "key_filter.h"
#include "key_event.h"
#include <algorithm>

namespace
{
    std::array<bool, 5> key_states{};
    std::array<unsigned, 5>  prev_levels{};
    bool  are_prev_levels_set  =  false;
}

enum {
    key_0_pressed = 1 << 0,
    key_1_pressed = 1 << 1,
    key_2_pressed = 1 << 2,
    key_3_pressed = 1 << 3,
    key_4_pressed = 1 << 4
};

/*
int filter_keys(const int adc[5])
{
    static int 
        PF_set = 0,                 // are forces from the previous iteration set
        PF[5] = { 0, 0, 0, 0, 0 };  // forces from previous iteration; stored between calls to this function

    const int // F is array of pressure forces on each sensor, in range [0, 127]
        F[5] = { 0x7F - adc[0], 0x7F - adc[1], 0x7F - adc[2], 0x7F - adc[3], 0x7F - adc[4] };

    int D[5] = { 0, 0, 0, 0, 0 }; // Delta between current forces (F) and previous step forces (PF)
    if (PF_set) {
        D[0] = F[0] - PF[0]; D[1] = F[1] - PF[1]; D[2] = F[2] - PF[2]; D[3] = F[3] - PF[3]; D[4] = F[4] - PF[4];
    }

    int S[5] = { 0, 0, 0, 0, 0 }; // Suppress flag: which detected key presses to suppress 

    const int d2 = 


    // store current forces for the next iteration
    PF[0] = F[0]; PF[1] = F[1]; PF[2] = F[2]; PF[3] = F[3]; PF[4] = F[4];
    // "previous forces" were set for the next iteration
    PF_set = 1;
    
}
*/

std::array<unsigned, 10>  process_sensor_levels(const config_t & config, const uint8_t levels[5])
{
    std::array<unsigned, 5>  original_levels{};
    std::transform(levels, levels + 5, original_levels.begin(), [](uint8_t l) { return 0x7F - l; });

    if (!are_prev_levels_set) {
        prev_levels = original_levels;
        are_prev_levels_set = true;
    }

    std::array<int, 5>   delta_levels{};
    for  (unsigned i = 0; i < 5; i++)
    {
        delta_levels[i] = original_levels[i] - prev_levels[i];
    }
    prev_levels = original_levels;

    std::array<bool, 5>  suppress{};
    {
        const int
            delta_2 = std::max(delta_levels[2] - (int)config.delta_thresholds[2], 0),
            delta_3 = std::max(delta_levels[3] - (int)config.delta_thresholds[3], 0),
            delta_4 = std::max(delta_levels[4] - (int)config.delta_thresholds[4], 0),
            max_delta = std::max({ delta_2, delta_3, delta_4 });

        if (max_delta >= 0)
        {
            suppress[2] = suppress[3] = suppress[4] = true;
            const int  idx_max = ((delta_4 == max_delta) ? 4 : (delta_3 == max_delta) ? 3 : 2);
            suppress[idx_max] = false;
        }
    }
    
    std::array<unsigned, 5>  filtered_levels{};
    std::array<bool, 5> active_sensors = key_states;
    for (unsigned i = 0; i < 5; i++)
    {
        if (original_levels[i] > config.level_thresholds[i] && delta_levels[i] >(int)config.delta_thresholds[i]  && ! suppress[i])
        {
            active_sensors[i] = true;
        }   

        if (active_sensors[i] && original_levels[i] <= config.level_thresholds[i])
        {
            active_sensors[i] = false;
        }

        filtered_levels[i] = active_sensors[i]?original_levels[i] : 0;
    }    

    for (unsigned i = 0; i < 5; ++i)
        if (key_states[i] != active_sensors[i])
            ((key_states[i] = active_sensors[i]) ? simulate_key_press : simulate_key_release)(config.key_codes[i]);

    std::array<unsigned, 10>  result{};
    std::copy_n(original_levels.begin(), 5, result.begin() + 0);
    std::copy_n(filtered_levels.begin(), 5, result.begin() + 5);
    return  result;
}
