#include "key_filter.h"
#include "key_event.h"
#include <algorithm>

namespace
{
    std::array<bool, 5> key_states{};
    std::array<unsigned, 5>  prev_levels{};
    bool  are_prev_levels_set  =  false;
}

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
