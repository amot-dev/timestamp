#include "exif_file.h"

#include <array>
#include <iostream>
#include <regex>

#include "color.h"
#include "config.h"
#include "utility.h"

ExifFile::ExifFile(fs::path path, std::shared_ptr<std::map<std::string, int>> proposed_name_counts_ptr) : path{path}, proposed_name_counts_ptr{proposed_name_counts_ptr} {
    if (!proposed_name_counts_ptr) throw std::runtime_error("Invalid shared pointer passed to ExifFile.");

    std::string extension = this->path.extension();
    std::string new_name = get_metadata_date(DEFAULT_DATE_TAG);

    // If no date could be set with default tag, try fallback tag
    if (new_name.empty()) {
        new_name = get_metadata_date(FALLBACK_DATE_TAG);

        // Give up after failing fallback (should never happen in theory)
        if (new_name.empty()) {
            this->add_proposed_name("");
            return;
        }
        else this->current_date_tag = this->default_date_tag = FALLBACK_DATE_TAG;
    }
    else this->current_date_tag = this->default_date_tag = DEFAULT_DATE_TAG;

    // Add extension and save proposed name
    new_name += extension;
    this->add_proposed_name(new_name);
}

fs::path ExifFile::get_path() const { return this->path; }

std::string ExifFile::get_proposed_name() const { return this->proposed_name; }

bool ExifFile::is_skipped() const { return this->proposed_name.empty(); }

bool ExifFile::is_clashing() const {
    if (this->is_skipped()) return (*this->proposed_name_counts_ptr)[this->path.filename().string()] > 1;
    else return (*this->proposed_name_counts_ptr)[this->proposed_name] > 1;
}

void ExifFile::edit_proposed_name() {
    std::cout << CYAN << "\n\nPossible names for " << this->path.filename().string() << RESET << std::endl;

    std::string extension = this->path.extension();
    std::vector<std::pair<std::string, std::string>> possible_names;

    // Skip and Custom name options
    possible_names.push_back(std::make_pair("Skip", ""));
    possible_names.push_back(std::make_pair("Custom", ""));

    // Default tag option
    possible_names.push_back(std::make_pair(this->default_date_tag, get_metadata_date(this->default_date_tag) + extension));
       
    for (size_t i = possible_names.size(); i > 0; --i) {
        std::cout << i << "\t" << possible_names[i-1].first;
        if (possible_names[i-1].first != "Custom" && possible_names[i-1].first != "Skip") {
            std::cout << ": " << possible_names[i-1].second;
        }

        if (possible_names[i-1].first == this->current_date_tag) std::cout << GREEN << " (selected)" << RESET << std::endl;
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
            std::cout << RED << "[ERROR] " << RESET << "Invalid choice" << std::endl;
            continue; // Invalid input, continue the loop
        }

        if (selection < 1 || selection > possible_names.size()) {
            std::cout << RED << "[ERROR] " << RESET << "Invalid choice" << std::endl;
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
        else if (selected.first == "Skip") this->set_proposed_name("");
        else this->set_proposed_name(selected.second);
        
        break;
    }
}

bool ExifFile::rename() {
    if (this->is_skipped()) return false;
    if (this->proposed_name == this->path.filename().string()) return false;

    try {
        auto new_path = this->path;
        new_path.replace_filename(this->proposed_name);
        fs::rename(this->path, new_path);
    }
    catch (const fs::filesystem_error& e) {
        std::cerr << RED << "[Error] " << RESET << e.what() << std::endl;
        return false;
    }

    return true;
}

std::string ExifFile::get_exif_date(const Exiv2::Image::UniquePtr& media, const std::string& exif_tag) {
    try {
        // First try grab Exif.Photo.DateTimeOriginal if available
        Exiv2::ExifData &exifData = media->exifData();
        if (!exifData.empty()) {
            Exiv2::ExifKey dateTimeOriginalKey(exif_tag);
            Exiv2::ExifData::iterator exifEntry = exifData.findKey(dateTimeOriginalKey);
            if (exifEntry != exifData.end()) {
                // Format time
                auto time = exif_date_to_time_point(exifEntry->toString());
                return time_point_to_formatted_string(time, FINAL_DATE_FORMAT);
            }
        }
    }
    catch(std::runtime_error& e) {
        std::cout << RED << "[ERROR] " << RESET << e.what() << std::endl;
    }
    catch(...) {
        std::cout << RED << "[ERROR] " << RESET "Failed to read EXIF data from " << this->path.filename().string() << std::endl;
    }

    return "";
}

std::string ExifFile::get_xmp_date(const Exiv2::Image::UniquePtr& media, const std::string& xmp_tag) {
    try {
        Exiv2::XmpData &xmpData = media->xmpData();
        if (!xmpData.empty()) {
            Exiv2::XmpKey modificationDateKey(xmp_tag);
            Exiv2::XmpData::iterator xmpEntry = xmpData.findKey(modificationDateKey);
            if (xmpEntry != xmpData.end()) {
                // Format time
                auto time = xmp_epoch_to_time_point(xmpEntry->toInt64());
                return time_point_to_formatted_string(time, FINAL_DATE_FORMAT);
            }
        }
    }
    catch(...) {
        std::cout << RED << "[ERROR] " << RESET "Failed to read XMP data from " << this->path.filename().string() << std::endl;
    }

    return "";
}

std::string ExifFile::get_metadata_date(const std::string& tag) {
    try {
        // Load the image or video file
        Exiv2::Image::UniquePtr media = Exiv2::ImageFactory::open(this->path.string());
        if (!media.get()) {
            std::cout << RED << "[ERROR] " << RESET "Cannot open " << this->path.filename().string() << std::endl;
            return "";
        }

        // Read the metadata from the file
        media->readMetadata();

        if (tag.starts_with("Exif.")) return get_exif_date(media, tag);
        else if (tag.starts_with("Xmp.")) return get_xmp_date(media, tag);
        else {
            std::cout << RED << "[ERROR] " << RESET "Invalid tag: " << tag << std::endl;
            return "";
        }
    }
    catch (...) {
        std::cout << RED << "[ERROR] " << RESET "Failed to read from " << this->path.filename().string() << std::endl;
    }

    return "";
}

void ExifFile::add_proposed_name(const std::string& proposed_name) {
    // Change name
    this->proposed_name = proposed_name;

    // Increment count based on skipped or not
    if (this->is_skipped()) (*this->proposed_name_counts_ptr)[this->path.filename().string()]++;
    if (!proposed_name.empty()) (*this->proposed_name_counts_ptr)[proposed_name]++;
}

void ExifFile::remove_proposed_name() {
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

void ExifFile::set_proposed_name(const std::string& proposed_name) {
    this->remove_proposed_name();
    this->add_proposed_name(proposed_name);
}