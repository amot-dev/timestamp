#include "settings.h"

#include <fstream>
#include <iostream>
#include <yaml-cpp/yaml.h>

#include "color.h"

Settings::Settings(const std::string& filepath) {
    try {
        YAML::Node config = YAML::LoadFile(filepath);

        // Load date format
        // TODO: verify this date format
        if (config["date_format"]) date_format = config["date_format"].as<std::string>();
        else date_format = "%Y-%m-%d-%H%M-%S";  // Default format

        // Load tags_for section
        if (config["tags_for"]) {
            for (const auto& group : config["tags_for"]) {
                std::string group_name = group.first.as<std::string>();
                for (const auto& tag : group.second) {
                    extension_to_tags[group_name].push_back(tag.as<std::string>());
                }
            }
        }

        // Load extension_groups and map extensions to tags
        if (config["extension_groups"]) {
            for (const auto& group : config["extension_groups"]) {
                std::string group_name = group.first.as<std::string>();

                if (config["tags_for"][group_name]) {
                    for (const auto& extension : group.second) {
                        std::string ext = extension.as<std::string>();
                        extension_to_tags[ext] = config["tags_for"][group_name].as<std::vector<std::string>>();
                    }
                }
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << RED << "[ERROR] " << RESET << "Failed to parse YAML file: " << e.what() << std::endl;
    }
}

std::string Settings::get_primary_tag(const std::string& extension) const {
    auto it = extension_to_tags.find(extension);
    if (it != extension_to_tags.end() && !it->second.empty()) {
        return it->second.front();
    }

    return "";  // Return empty string if no tag is found
}

std::vector<std::string> Settings::get_tags(const std::string& extension) const {
    auto it = extension_to_tags.find(extension);
    if (it != extension_to_tags.end()) return it->second;

    return {};  // Return empty vector if no tags are found
}