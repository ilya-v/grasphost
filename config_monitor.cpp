#include "config_monitor.h"
#include <chrono>
#include <tuple>

namespace {

    using xtime_t    = std::chrono::steady_clock::time_point;
    using duration_t = std::chrono::steady_clock::duration;

    xtime_t  last_config_update_time{};
    const auto config_update_interval = std::chrono::seconds(1);

    config_t  config;
}

config_t  get_config(const std::string & config_file_name)
{
    const auto time_now = std::chrono::steady_clock::now();
    if (time_now < last_config_update_time + config_update_interval)
        return config;
    last_config_update_time = time_now;

    int nscanned = 0;
    if (FILE *fconfig = fopen(config_file_name.c_str(), "r"))
    {
        for (auto i = 0; i < 5; i++)
            nscanned += fscanf(fconfig, "%x%u%u", &config.key_codes[i], &config.level_thresholds[i], &config.delta_thresholds[i]);
        fclose(fconfig);
    }
    config.is_config_set = (nscanned == 15);
    return config;
}