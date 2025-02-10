#include "settings.h"

#include <fstream>
#include <iostream>

#include "color.h"
#include "utility.h"

Settings::Settings(const std::string& filepath) {
    try {
        YAML::Node config = YAML::LoadFile(filepath);

        // Load date format
        if (config["date_format"]) {
            date_format = config["date_format"].as<std::string>();

            // Attempt to use given date format (will throw exception if invalid format)
            time_point_to_formatted_string(std::chrono::system_clock::now(), date_format);
        }
        else {
            date_format = "%Y-%m-%d-%H%M-%S";  // Default format
            std::cerr << YELLOW << "[WARNING] " << RESET << "Config contains no date format. Using default of " << date_format << std::endl;
        }

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
        std::cerr << RED << "[ERROR] " << RESET << "Failed to parse config file: " << e.what() << std::endl;
        throw std::runtime_error("Failed to parse config file");
    }
}

const YAML::Node Settings::DEFAULT_CONFIG = [] {
    YAML::Node node;

    // Date format
    node["date_format"] = "%Y-%m-%d-%H%M-%S";

    // Extension groups
    node["extension_groups"]["image"] = YAML::Node(YAML::NodeType::Sequence);
    node["extension_groups"]["image"].push_back("avif");
    node["extension_groups"]["image"].push_back("gif");
    node["extension_groups"]["image"].push_back("jpg");
    node["extension_groups"]["image"].push_back("jpeg");
    node["extension_groups"]["image"].push_back("png");
    node["extension_groups"]["image"].push_back("svg");
    node["extension_groups"]["image"].push_back("webp");

    node["extension_groups"]["video"] = YAML::Node(YAML::NodeType::Sequence);
    node["extension_groups"]["video"].push_back("avi");
    node["extension_groups"]["video"].push_back("mkv");
    node["extension_groups"]["video"].push_back("mov");
    node["extension_groups"]["video"].push_back("mp4");
    node["extension_groups"]["video"].push_back("mpeg");
    node["extension_groups"]["video"].push_back("webm");

    // Tags for extension groups
    node["tags_for"]["image"] = YAML::Node(YAML::NodeType::Sequence);
    node["tags_for"]["image"].push_back("Exif.Photo.DateTimeOriginal");
    node["tags_for"]["image"].push_back("inode.mtime");

    node["tags_for"]["video"] = YAML::Node(YAML::NodeType::Sequence);
    node["tags_for"]["video"].push_back("Xmp.video.ModifyDate");
    node["tags_for"]["video"].push_back("inode.mtime");

    return node;
}();

bool Settings::generate_default_config(const std::string& filepath) {
    try {
        std::ofstream file(filepath);
        if (!file) {
            std::cerr << RED << "[ERROR] " << RESET << "Failed to create config file: " << filepath << std::endl;
            return false;
        }

        file << DEFAULT_CONFIG;  // Dump YAML to file
        file.close();
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << RED << "[ERROR] " << RESET << "Failed to write config file: " << e.what() << std::endl;
        return false;
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