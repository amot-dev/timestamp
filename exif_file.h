#ifndef EXIF_FILE_H
#define EXIF_FILE_H

#include <filesystem>
#include <vector>

#define DEFAULT_EXIF_TAG "FileModifyDate"
#define VALID_EXIF_TAG_COUNT 7

namespace fs = std::filesystem;

class ExifFile {
    std::string proposed_name;
    std::string current_exif_tag;
    bool skip;

    static const std::array<std::string, VALID_EXIF_TAG_COUNT> valid_exif_tags;

    std::string run_command(const std::string& command);
    std::string get_exif_date(const std::string& exif_tag);
    std::vector<std::pair<std::string, std::string>> get_available_exif_tags();
    void generate_new_name(const std::string& exif_tag);

public: 
    fs::path path;

    ExifFile(fs::path path);
    fs::path get_path() const;
    std::string get_proposed_name() const;
    bool is_skipped() const;
    void edit_proposed_name();
    void rename();
};

#endif //EXIF_FILE_H