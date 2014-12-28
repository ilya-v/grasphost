#ifndef _config_monitor__h_
#define _config_monitor__h_

#include <string>
#include <array>
#include <tuple>

struct config_t {

    config_t & operator=(const config_t &) = default;
    bool operator !=(const config_t & x) const    { 
        return std::tie(is_config_set, key_codes, level_thresholds, delta_thresholds) 
            != std::tie(x.is_config_set, x.key_codes, x.level_thresholds, x.delta_thresholds);
    };

    bool  is_config_set = false;
    std::array<unsigned, 5>
        key_codes = {},
        level_thresholds ={},
        delta_thresholds ={};
};

config_t  get_config(const std::string & config_file_name);

#endif