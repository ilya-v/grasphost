#ifndef _key_filter__h__
#define _key_filter__h__

#include <array>
#include <cinttypes>
#include "config_monitor.h"

std::array<unsigned, 10>  process_sensor_levels(const config_t & config, const uint8_t levels[5]);

#endif