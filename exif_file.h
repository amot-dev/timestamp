#ifndef EXIF_FILE_H
#define EXIF_FILE_H

#include <exiv2/exiv2.hpp>
#include <filesystem>
#include <map>
#include <vector>

#define EXIF_MEDIA_MODIFY_DATE "MediaModifyDate"
#define EXIF_MEDIA_CREATE_DATE "MediaCreateDate"
#define EXIF_FILE_MODIFY_DATE "FileModifyDate"
#define EXIF_FILE_ACCESS_DATE "FileAccessDate"
#define EXIF_MODIFY_DATE "ModifyDate"
#define EXIF_CREATE_DATE "CreateDate"
#define EXIF_DATE_TIME_ORIGINAL "DateTimeOriginal"
#define VALID_EXIF_TAG_COUNT 7

namespace fs = std::filesystem;

class ExifFile {
    fs::path path;
    std::string proposed_name;
    std::string current_exif_tag;
    std::shared_ptr<std::map<std::string, int>> proposed_name_counts_ptr;

    static const std::array<std::string, VALID_EXIF_TAG_COUNT> valid_exif_tags;

    std::string get_exif_date(const Exiv2::Image::UniquePtr& media, const std::string& exif_tag);
    std::string get_xmp_date(const Exiv2::Image::UniquePtr& media, const std::string& xmp_tag);
    std::string get_metadata_date(const std::string& tag);
    void add_proposed_name(const std::string& proposed_name);
    void remove_proposed_name();
    void set_proposed_name(const std::string& proposed_name);

public:
    ExifFile(fs::path path, std::shared_ptr<std::map<std::string, int>> proposed_name_counts_ptr);
    fs::path get_path() const;
    std::string get_proposed_name() const;
    bool is_skipped() const;
    bool is_clashing() const;
    void edit_proposed_name();
    bool rename();
};

#endif //EXIF_FILE_H