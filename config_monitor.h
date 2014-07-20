#ifndef _config_monitor__h_
#define _config_monitor__h_

#include <string>
#include <array>
#include <tuple>

struct config_t {

    config_t & operator=(const config_t &) = default;
    bool operator !=(const config_t & x) const    { return !!memcmp(this, &x, sizeof(*this));   };

    bool  is_config_set = false;
    std::array<unsigned, 5>
        key_codes{ {} },
        level_thresholds{ {} },
        delta_thresholds{ {} };

};

config_t  get_config(const std::string & config_file_name);

#endif