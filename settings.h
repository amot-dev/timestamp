#ifndef SETTINGS_H
#define SETTINGS_H

#include <string>
#include <unordered_map>
#include <vector>

class Settings {
private:
    std::string date_format;
    std::unordered_map<std::string, std::vector<std::string>> extension_to_tags;

public:
    Settings(const std::string& filepath);
    std::string get_primary_tag(const std::string& extension) const;
    std::vector<std::string> get_tags(const std::string& extension) const;
    std::string get_date_format() const { return date_format; }
};

#endif // SETTINGS_H