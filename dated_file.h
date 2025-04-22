#ifndef DATED_FILE_H
#define DATED_FILE_H

#include <exiv2/exiv2.hpp>
#include <filesystem>
#include <map>
#include <vector>

#include "settings.h"

namespace fs = std::filesystem;

class DatedFile {
    Settings settings;
    fs::path path;
    std::string proposed_name;
    std::string current_date_tag;
    std::string default_date_tag;
    std::shared_ptr<std::map<std::string, int>> proposed_name_counts_ptr;

    std::string get_exif_date(const Exiv2::Image::UniquePtr& media, const std::string& exif_tag, const std::string& date_format);
    std::string get_xmp_date(const Exiv2::Image::UniquePtr& media, const std::string& xmp_tag, const std::string& date_format);
    std::string get_inode_date(const std::string& inode_tag, const std::string& date_format);
    std::string get_metadata_date(const std::string& tag, const std::string& date_format);
    void add_proposed_name(const std::string& proposed_name);
    void remove_proposed_name();
    void set_proposed_name(const std::string& proposed_name);

public:
    DatedFile(
        const Settings& settings,
        fs::path path,
        std::shared_ptr<std::map<std::string, int>> proposed_name_counts_ptr
    );
    fs::path get_path() const;
    std::string get_proposed_name() const;
    bool is_skipped() const;
    bool is_clashing() const;
    void edit_proposed_name();
    void set_skipped();
    bool rename();
};

#endif // DATED_FILE_H