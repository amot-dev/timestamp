#ifndef SETTINGS_H
#define SETTINGS_H

#include <string>
#include <unordered_map>
#include <vector>
#include <yaml-cpp/yaml.h>

class Settings {
private:
    static const YAML::Node DEFAULT_CONFIG;
    std::string date_format;
    std::unordered_map<std::string, std::vector<std::string>> extension_to_tags;

public:
    Settings(const std::string& filepath);
    static bool generate_default_config(const std::string& filepath);
    std::string get_primary_tag(const std::string& extension) const;
    std::vector<std::string> get_tags(const std::string& extension) const;
    std::string get_date_format() const { return date_format; }
};

#endif // SETTINGS_H