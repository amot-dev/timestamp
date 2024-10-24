#ifndef UTILITY_H
#define UTILITY_H

#include <chrono>
#include <format>

#define SECONDS_DIFFERENCE_1904_1970 2082844800 // 2082844800 seconds is the difference from 1904 to 1970

std::chrono::system_clock::time_point exif_date_to_time_point(const std::string& exif_date) {
    std::chrono::system_clock::time_point time;
    std::istringstream ss(exif_date);

    ss >> std::chrono::parse("%Y:%m:%d %H:%M:%S", time);
    if (ss.fail()) throw std::runtime_error("Failed to parse EXIF date using chrono::parse");
    
    return time;
}

std::chrono::system_clock::time_point xmp_epoch_to_time_point(const long long xmp_epoch) {
    return std::chrono::time_point<std::chrono::system_clock>(std::chrono::seconds(xmp_epoch - SECONDS_DIFFERENCE_1904_1970));
}

std::string time_point_to_formatted_string(const std::chrono::system_clock::time_point& time, const std::string& format) {
    auto rounded_time = std::chrono::time_point_cast<std::chrono::seconds>(time);
    return std::format(std::runtime_format(format), rounded_time);
}

#endif // UTILITY_H