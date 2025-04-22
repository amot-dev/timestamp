#include "dated_file.h"

#include <iostream>
#include <sys/stat.h>

#include "color.h"
#include "utility.h"

DatedFile::DatedFile(const Settings& settings,
    fs::path path,
    std::shared_ptr<std::map<std::string, int>> proposed_name_counts_ptr)
    :   settings{settings},
        path{path},
        proposed_name_counts_ptr{proposed_name_counts_ptr} {

    if (!proposed_name_counts_ptr) throw std::runtime_error("Invalid shared pointer passed to ExifFile");

    // Get tags for extension
    std::string extension = this->path.extension();
    if (extension.empty()) {
        this->add_proposed_name("");
        return;
    }

    std::vector<std::string> tags = settings.get_tags(extension.substr(1)); // Remove dot when calling get_tags

    // If tags are empty, return with blank name (skip)
    if (tags.empty()) {
        this->add_proposed_name("");
        return;
    }

    // Try each tag in order of priority
    std::string new_name;
    for (const auto& tag : tags) {
        try {
            new_name = get_metadata_date(tag, this->settings.get_date_format());
            if (!new_name.empty()) {
                this->current_date_tag = this->default_date_tag = tag;
                break;
            }
        }
        catch (...) {
            std::cerr << YELLOW << "[WARNING] " << RESET << "Failed to read metadata from " << this->path.filename().string() << " using tag: " << tag << std::endl;
        }
    }

    // If no tags worked, return with blank name (skip)
    if (new_name.empty()) {
        this->add_proposed_name("");
        return;
    }

    // Add extension and save proposed name
    new_name += extension;
    this->add_proposed_name(new_name);
}

fs::path DatedFile::get_path() const { return this->path; }

std::string DatedFile::get_proposed_name() const { return this->proposed_name; }

bool DatedFile::is_skipped() const { return this->proposed_name.empty(); }

bool DatedFile::is_clashing() const {
    if (this->is_skipped()) return (*this->proposed_name_counts_ptr)[this->path.filename().string()] > 1;
    else return (*this->proposed_name_counts_ptr)[this->proposed_name] > 1;
}

void DatedFile::edit_proposed_name() {
    std::cout << CYAN << "\n\nPossible names for " << this->path.filename().string() << RESET << std::endl;

    std::string extension = this->path.extension();
    std::vector<std::pair<std::string, std::string>> possible_names;

    // Skip and Custom name options
    possible_names.push_back(std::make_pair("Skip", ""));
    possible_names.push_back(std::make_pair("Custom", ""));

    // Build list of possible names using all tags for the extension (in reverse order)
    std::vector<std::pair<std::string, std::string>> temp_names;
    for (const auto& tag : this->settings.get_tags(extension.substr(1))) {
        std::string date = get_metadata_date(tag, this->settings.get_date_format());
        if (!date.empty()) temp_names.emplace_back(tag, date + extension);
    }

    // Insert in reverse order
    possible_names.insert(possible_names.end(), temp_names.rbegin(), temp_names.rend());

    for (size_t i = possible_names.size(); i > 0; --i) {
        std::cout << i << "\t" << possible_names[i-1].first;
        if (possible_names[i-1].first != "Custom" && possible_names[i-1].first != "Skip") {
            std::cout << ": " << possible_names[i-1].second;
        }

        if (possible_names[i-1].first == this->current_date_tag) std::cout << GREEN << " (selected)" << RESET << std::endl;
        else if (possible_names[i-1].first == this->default_date_tag) std::cout << YELLOW << " (default)" << RESET << std::endl;
        else std::cout << std::endl;
    }

    while (true) {
        std::cout << "\nSelect an option (or blank for no change): ";

        std::string input;
        std::getline(std::cin, input); // Read the entire line as a string

        // No-op if the user just hits enter
        if (input.empty()) break;

        // Convert input to size_t
        size_t selection;
        try {
            selection = std::stoul(input); // Convert string to unsigned long
        }
        catch (...) {
            std::cerr << RED << "[ERROR] " << RESET << "Invalid choice" << std::endl;
            continue; // Invalid input, continue the loop
        }

        if (selection < 1 || selection > possible_names.size()) {
            std::cerr << RED << "[ERROR] " << RESET << "Invalid choice" << std::endl;
            continue; // Invalid range, continue the loop
        }
        
        const auto& selected = possible_names[selection - 1];
        this->current_date_tag = selected.first;

        if (selected.first == "Custom") {
            std::cout << "Enter a custom name: ";
            std::string new_name;
            std::getline(std::cin, new_name);
            this->set_proposed_name(new_name);
        }
        else if (selected.first == "Skip") this->set_skipped();
        else this->set_proposed_name(selected.second);
        
        break;
    }
}

void DatedFile::set_skipped() { this->set_proposed_name(""); }

bool DatedFile::rename() {
    if (this->is_skipped()) return false;
    if (this->proposed_name == this->path.filename().string()) return false;

    try {
        auto new_path = this->path;
        new_path.replace_filename(this->proposed_name);
        fs::rename(this->path, new_path);
    }
    catch (const fs::filesystem_error& e) {
        std::cerr << RED << "[ERROR] " << RESET << e.what() << std::endl;
        return false;
    }

    return true;
}

std::string DatedFile::get_exif_date(const Exiv2::Image::UniquePtr& media, const std::string& exif_tag, const std::string& date_format) {
    try {
        // First try grab Exif.Photo.DateTimeOriginal if available
        Exiv2::ExifData &exifData = media->exifData();
        if (!exifData.empty()) {
            Exiv2::ExifKey dateTimeOriginalKey(exif_tag);
            Exiv2::ExifData::iterator exifEntry = exifData.findKey(dateTimeOriginalKey);
            if (exifEntry != exifData.end()) {
                // Format time
                auto time = exif_date_to_time_point(exifEntry->toString());
                return time_point_to_formatted_string(time, date_format);
            }
        }
    }
    catch(std::runtime_error& e) {
        std::cerr << RED << "[ERROR] " << RESET << e.what() << std::endl;
    }
    catch(...) {
        std::cerr << RED << "[ERROR] " << RESET "Failed to read EXIF data from " << this->path.filename().string() << std::endl;
    }

    return "";
}

std::string DatedFile::get_xmp_date(const Exiv2::Image::UniquePtr& media, const std::string& xmp_tag, const std::string& date_format) {
    try {
        Exiv2::XmpData &xmpData = media->xmpData();
        if (!xmpData.empty()) {
            Exiv2::XmpKey modificationDateKey(xmp_tag);
            Exiv2::XmpData::iterator xmpEntry = xmpData.findKey(modificationDateKey);
            if (xmpEntry != xmpData.end()) {
                // Format time
                auto time = xmp_epoch_to_time_point(xmpEntry->toInt64());
                return time_point_to_formatted_string(time, date_format);
            }
        }
    }
    catch(...) {
        std::cerr << RED << "[ERROR] " << RESET "Failed to read XMP data from " << this->path.filename().string() << std::endl;
    }

    return "";
}

std::string DatedFile::get_inode_date(const std::string& inode_tag, const std::string& date_format) {
    struct stat file_stat;

    // Attempt to get file stat
    if (stat(this->path.c_str(), &file_stat) != 0) {
        std::cerr << RED << "[ERROR] " << RESET "Failed to stat file " << this->path.filename().string() << std::endl;
        return "";
    }

    time_t inode_time = 0;

    if (inode_tag == "inode.mtime") inode_time = file_stat.st_mtime;
    else if (inode_tag == "inode.atime") inode_time = file_stat.st_atime;
    else if (inode_tag == "inode.ctime") inode_time = file_stat.st_ctime;
    else {
        std::cerr << RED << "[ERROR] " << RESET "Invalid inode tag: " << inode_tag << std::endl;
        return "";
    }

    // Format time
    auto time = epoch_to_time_point(inode_time);
    return time_point_to_formatted_string(time, date_format, true);
}

std::string DatedFile::get_metadata_date(const std::string& tag, const std::string& date_format) {
    // Handle potential inode tag first
    if (tag.starts_with("inode.")) return get_inode_date(tag, date_format);

    // Load the image or video file
    Exiv2::Image::UniquePtr media = Exiv2::ImageFactory::open(this->path.string());
    if (!media.get()) {
        std::cerr << RED << "[ERROR] " << RESET "Cannot open " << this->path.filename().string() << std::endl;
        return "";
    }

    // Read the metadata from the file
    media->readMetadata();

    // Test whether this is Exif or Xmp
    if (tag.starts_with("Exif.")) return get_exif_date(media, tag, date_format);
    else if (tag.starts_with("Xmp.")) return get_xmp_date(media, tag, date_format);
    else {
        std::cerr << RED << "[ERROR] " << RESET "Invalid tag: " << tag << std::endl;
        Exiv2::XmpParser::terminate();
        return "";
    }

    Exiv2::XmpParser::terminate();
    return "";
}

void DatedFile::add_proposed_name(const std::string& proposed_name) {
    // Change name
    this->proposed_name = proposed_name;

    // Increment count based on skipped or not
    if (this->is_skipped()) (*this->proposed_name_counts_ptr)[this->path.filename().string()]++;
    if (!proposed_name.empty()) (*this->proposed_name_counts_ptr)[proposed_name]++;
}

void DatedFile::remove_proposed_name() {
    // Set old name based on skipped or not
    std::string old_name;
    if (this->is_skipped()) old_name = path.filename().string();
    else old_name = this->proposed_name;

    // Decrement count for old name (if it exists, otherwise ignore)
    if (this->proposed_name_counts_ptr->count(old_name) > 0) {
        (*this->proposed_name_counts_ptr)[old_name]--;
        if ((*this->proposed_name_counts_ptr)[old_name] == 0) {
            this->proposed_name_counts_ptr->erase(old_name);  // Clean up the map when the count reaches 0
        }
    }
}

void DatedFile::set_proposed_name(const std::string& proposed_name) {
    this->remove_proposed_name();
    this->add_proposed_name(proposed_name);
}
