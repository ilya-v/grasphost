#include "config_monitor.h"
#include <chrono>
#include <tuple>

namespace {

    using xtime_t    = std::chrono::steady_clock::time_point;
    using duration_t = std::chrono::steady_clock::duration;

    xtime_t  last_config_update_time{};
    const auto config_update_interval = std::chrono::seconds(1);

    std::array<unsigned, 5> keycodes = {};
    std::array<unsigned, 5> thresholds = {};
}

std::tuple<bool, std::array<unsigned, 5>, std::array<unsigned, 5>, bool>  get_config(const std::string & config_file_name)
{
    const auto time_now = std::chrono::steady_clock::now();
    if (time_now < last_config_update_time + config_update_interval)
    {
        return std::make_tuple(true, keycodes, thresholds, false);
    }
    last_config_update_time = time_now;

    std::array<unsigned, 5> old_keycodes = keycodes;
    std::array<unsigned, 5> old_thresholds = thresholds;

    int nscanned = 0;
    if (FILE *fconfig = fopen(config_file_name.c_str(), "r"))
    {
        for (auto i = 0; i < 5; i++)
            nscanned += fscanf(fconfig, "%x%d", &keycodes[i], &thresholds[i]);
        fclose(fconfig);
    }
    return std::make_tuple( nscanned == keycodes.size() * 2, keycodes, thresholds, (keycodes != old_keycodes)||(thresholds != old_thresholds) );
}