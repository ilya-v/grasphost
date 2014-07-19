#ifndef _key_filter__h__
#define _key_filter__h__

#include <array>
#include <cinttypes>

std::array<unsigned, 10>  process_sensor_levels(std::array<unsigned, 5> keycodes, std::array<unsigned, 5> thresholds, const uint8_t levels[5]);

#endif