#include "utility.h"

#include <format>

std::chrono::system_clock::time_point exif_date_to_time_point(const std::string& exif_date) {
    std::chrono::system_clock::time_point time;
    std::istringstream ss(exif_date);
    ss >> std::chrono::parse("%Y:%m:%d %H:%M:%S", time);
    if (ss.fail()) throw std::runtime_error("Failed to parse EXIF date using chrono::parse");
    return time;
}

std::chrono::system_clock::time_point xmp_epoch_to_time_point(long long xmp_epoch) {
    return std::chrono::time_point<std::chrono::system_clock>(std::chrono::seconds(xmp_epoch - SECONDS_DIFFERENCE_1904_1970));
}

std::chrono::system_clock::time_point epoch_to_time_point(time_t epoch) {
    return std::chrono::system_clock::from_time_t(epoch);
}

std::string time_point_to_formatted_string(const std::chrono::system_clock::time_point& time, const std::string& date_format, bool localtime) {
    std::string final_date_format = "{:" + date_format + "}";
    auto rounded_time = std::chrono::time_point_cast<std::chrono::seconds>(time);

    if (localtime) {
        std::chrono::zoned_time zoned_time{std::chrono::current_zone(), rounded_time};
        return std::format(std::runtime_format(final_date_format), zoned_time);
    }
    return std::format(std::runtime_format(final_date_format), rounded_time);
}
