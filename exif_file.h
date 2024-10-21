#ifndef EXIF_FILE_H
#define EXIF_FILE_H

#include <filesystem>
#include <map>
#include <vector>

#define DEFAULT_EXIF_TAG "FileModifyDate"
#define VALID_EXIF_TAG_COUNT 7

namespace fs = std::filesystem;

class ExifFile {
    fs::path path;
    std::string proposed_name;
    std::string current_exif_tag;
    std::shared_ptr<std::map<std::string, int>> proposed_name_counts_ptr;

    static const std::array<std::string, VALID_EXIF_TAG_COUNT> valid_exif_tags;

    std::string run_command(const std::string& command);
    std::string get_exif_date(const std::string& exif_tag);
    std::vector<std::pair<std::string, std::string>> get_available_exif_tags();
    void set_proposed_name(const std::string& proposed_name);
    void generate_new_name(const std::string& exif_tag);

public:
    ExifFile(fs::path path, std::shared_ptr<std::map<std::string, int>> proposed_name_counts_ptr);
    fs::path get_path() const;
    std::string get_proposed_name() const;
    bool is_skipped() const;
    bool is_clashing() const;
    void edit_proposed_name();
    void rename();
};

#endif //EXIF_FILE_H