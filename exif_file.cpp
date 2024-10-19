#include "exif_file.h"

#include <array>
#include <regex>
#include <iostream>

ExifFile::ExifFile(fs::path path) : current_exif_tag{DEFAULT_EXIF_TAG}, path{path} {
    this->skip = false;
    this->generate_new_name(DEFAULT_EXIF_TAG);
}

fs::path ExifFile::get_path() const { return this->path; }

std::string ExifFile::get_proposed_name() const { return this->proposed_name; }

bool ExifFile::is_skipped() const { return this->skip; }

void ExifFile::edit_proposed_name() {
    std::cout << "\n\nPossible names for " << this->path.filename().string() << std::endl;

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

        if (possible_names[i-1].first == this->current_exif_tag) std::cout << " (selected)" << std::endl;
        else std::cout << std::endl;
    }

    while (true) {
        std::cout << "\nSelect an option: ";

        size_t selection;
        std::cin >> selection;
        std::cin.ignore();

        if (selection < 1 || selection > possible_names.size()) {
            std::cout << "Invalid choice. Please try again." << std::endl;
            continue;
        }
        
        const auto& selected = possible_names[selection - 1];
        this->current_exif_tag = selected.first;

        if (selected.first == "Custom") {
            std::cout << "Enter a custom name: ";
            std::getline(std::cin, this->proposed_name);
            if (this->proposed_name == "") this->skip = true;
            else this->skip = false;
        }
        else if (selected.first == "Skip") {
            this->proposed_name = "";
            this->skip = true;
        }
        else {
            this->proposed_name = selected.second;
            if (selected.second == "") this->skip = true;
            else this->skip = false;
        }
        
        break;
    }
}

void ExifFile::rename() {
    if (this->skip) return;

    try {
        std::cout << "attempting rename" << std::endl;
        this->path.replace_filename(this->proposed_name);
    }
    catch (const fs::filesystem_error& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

// to be displayed in reverse order
const std::array<std::string, VALID_EXIF_TAG_COUNT> ExifFile::valid_exif_tags = {
    "MediaModifyDate",
    "MediaCreateDate",
    "FileModifyDate",
    "FileAccessDate",
    "ModifyDate",
    "CreateDate",
    "DateTimeOriginal",
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

void ExifFile::generate_new_name(const std::string& exif_tag) {
    std::string extension = this->path.extension();
    this->proposed_name = get_exif_date(exif_tag);

    if (this->proposed_name.empty()) {
        this->proposed_name = "";
        this->skip = true;
    }
    else {
        this->proposed_name += extension;
    }
}