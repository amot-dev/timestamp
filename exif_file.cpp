#include "exif_file.h"

#include <array>
#include <regex>
#include <iostream>

#include "color.h"

ExifFile::ExifFile(fs::path path, std::shared_ptr<std::map<std::string, int>> proposed_name_counts_ptr) : path{path}, proposed_name_counts_ptr{proposed_name_counts_ptr} {
    std::string extension = this->path.extension();
    std::string new_name = get_exif_date(DEFAULT_EXIF_TAG);

    // If no date could be set with default tag, try fallback tag
    if (new_name.empty()) {
        new_name = get_exif_date(FALLBACK_EXIF_TAG);

        // Give up after failing fallback (should never happen in theory)
        if (new_name.empty()) {
            this->add_proposed_name("");
            return;
        }
        else current_exif_tag = FALLBACK_EXIF_TAG;
    }
    else current_exif_tag = DEFAULT_EXIF_TAG;

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
    possible_names.push_back(std::make_pair("Skip", ""));
    possible_names.push_back(std::make_pair("Custom", ""));
    for (const auto& tag : this->valid_exif_tags) {
        std::string date = get_exif_date(tag);
        if (date.empty()) continue;

        date += extension;
        possible_names.push_back(std::make_pair(tag, date));
    }
       
    for (size_t i = possible_names.size(); i > 0; --i) {
        std::cout << i << "\t" << possible_names[i-1].first;
        if (possible_names[i-1].first != "Custom" && possible_names[i-1].first != "Skip") {
            std::cout << ": " << possible_names[i-1].second;
        }

        if (possible_names[i-1].first == this->current_exif_tag) std::cout << GREEN << " (selected)" << RESET << std::endl;
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
        this->current_exif_tag = selected.first;

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

// to be displayed in reverse order
const std::array<std::string, VALID_EXIF_TAG_COUNT> ExifFile::valid_exif_tags = {
    EXIF_MEDIA_MODIFY_DATE,
    EXIF_MEDIA_CREATE_DATE,
    EXIF_FILE_MODIFY_DATE,
    EXIF_FILE_ACCESS_DATE,
    EXIF_MODIFY_DATE,
    EXIF_CREATE_DATE,
    EXIF_DATE_TIME_ORIGINAL
};

std::string ExifFile::run_command(const std::string& command) {
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"), pclose);
    if (!pipe) throw std::runtime_error("popen() failed!");

    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

std::string ExifFile::get_exif_date(const std::string& exif_tag) {
    std::string command = "exiftool -" + exif_tag + " \"" + this->path.string() + "\" -d %Y-%m-%d-%H%M-%S -S";
    std::string output = run_command(command);
    std::regex date_regex(R"(\d{4}-\d{2}-\d{2}-\d{2}\d{2}-\d{2})");
    std::smatch match;
    if (std::regex_search(output, match, date_regex)) return match.str();
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