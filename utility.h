#ifndef UTILITY_H
#define UTILITY_H

#include <chrono>

#define SECONDS_DIFFERENCE_1904_1970 2082844800

std::chrono::system_clock::time_point exif_date_to_time_point(const std::string& exif_date);
std::chrono::system_clock::time_point xmp_epoch_to_time_point(long long xmp_epoch);
std::chrono::system_clock::time_point epoch_to_time_point(time_t epoch);
std::string time_point_to_formatted_string(const std::chrono::system_clock::time_point& time, const std::string& date_format, bool localtime = false);

#endif // UTILITY_H