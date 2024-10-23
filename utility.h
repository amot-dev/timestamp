#ifndef UTILITY_H
#define UTILITY_H

#include <chrono>

std::chrono::system_clock::time_point secondsSince1904ToTimePoint(long long secondsSince1904) {
    return std::chrono::time_point<std::chrono::system_clock>(std::chrono::seconds(secondsSince1904 - 2082844800)); // 2082844800 seconds is the difference from 1904 to 1970
}

#endif // UTILITY_H