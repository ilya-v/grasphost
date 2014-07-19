#ifndef _config_monitor__h_
#define _config_monitor__h_

#include <tuple>
#include <array>
#include <string>

std::tuple<bool, std::array<unsigned, 5>, std::array<unsigned, 5>, bool>  get_config(const std::string & config_file_name);

#endif