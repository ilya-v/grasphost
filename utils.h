#ifndef _utils__h__
#define _utils__h__

#include <exception>
#include <memory>
#include <string>
#include <sstream>
#include <iostream>

#define LOG(cmd) do { std::clog << "\t" << __LINE__ << ": " << #cmd << std::endl; cmd; } while(0);

template <typename T>
std::string to_string(const T x) {
    std::stringstream() sstr;
    sstr << x;
    return  x.str();
}

#define ENSURE(cmd, err) \
    do { \
        if (!(cmd)) { std::cout << "ERROR in " <<  __FUNCTION__ << ":" <<  __LINE__ << ": "<< (err) << std::endl;  \
        throw std::exception(); } \
    } while (0);


template<typename T, typename... Args>
std::unique_ptr<T> make_unique(Args&&... args)
{
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}


#endif
