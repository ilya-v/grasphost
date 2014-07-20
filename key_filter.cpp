#include "key_filter.h"
#include <windows.h>

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
    
    std::array<unsigned, 5>  filtered_levels{};
    for (unsigned i = 0; i < 5; i++)
    {
        if (original_levels[i] > config.level_thresholds[i] && delta_levels[i] > config.delta_thresholds[i])
            filtered_levels[i] = config.level_thresholds[i];
    }
    

    for (auto i = 0u; i < 5; ++i)
    {
        INPUT input = {};
        input.type = INPUT_KEYBOARD;
        input.ki.wVk = config.key_codes[i];
        if (!key_states[i] && filtered_levels[i] < config.level_thresholds[i])
        {
            key_states[i] = true;
            SendInput(1, &input, sizeof(input));
        }
        else  if (key_states[i] && filtered_levels[i] >= config.level_thresholds[i])
        {
            key_states[i] = false;
            input.ki.dwFlags = KEYEVENTF_KEYUP;
            SendInput(1, &input, sizeof(input));
        }
    }

    std::array<unsigned, 10>  result{};
    std::copy_n(original_levels.begin(), 5, result.begin() + 0);
    std::copy_n(filtered_levels.begin(), 5, result.begin() + 5);
    return  result;
}
