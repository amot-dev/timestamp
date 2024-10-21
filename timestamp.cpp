
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "color.h"
#include "exif_file.h"

namespace fs = std::filesystem;

// Display proposed file name changes
void display_proposed_changes(const std::vector<ExifFile>& files, bool first_display) {
    if (!first_display) std::cout << "\n";

    std::cout << CYAN << "Files to rename:" << RESET << std::endl;
    for (size_t i = files.size(); i > 0; --i) {
        std::string current_name = files[i-1].get_path().filename().string();
        std::string proposed_name = files[i-1].get_proposed_name();

        std::cout << i << "\t" << current_name << " -> ";

        if (files[i-1].is_skipped()) {
            std::cout << current_name;
        }
        else {
            std::cout << proposed_name;
        }

        if (files[i-1].is_clashing()) std::cout << RED << " (clashing)" << RESET;

        if (files[i-1].is_skipped()) std::cout << YELLOW << " (skipped)" << RESET << std::endl;
        else if (current_name == proposed_name) std::cout << GRAY << " (no change)" << RESET << std::endl;
        else std::cout << std::endl;
    }
}

// Check for any clashes
bool has_clashes(const std::shared_ptr<std::map<std::string, int>>& name_count) {
    for (const auto& entry : *name_count) {
        if (entry.second > 1) {
            return true; // Found a clash
        }
    }
    return false; // No clashes found
}

// Rename files
void rename_files(std::vector<ExifFile>& files) {
    int skip_count = 0;
    for (auto file : files) {
        if (!file.rename()) skip_count++;
    }
    int rename_count = files.size() - skip_count;
    std::cout << CYAN
              << "Renamed " << rename_count << " "
              << (rename_count == 1 ? "file" : "files") << ". "
              << "Skipped " << skip_count << " "
              << (skip_count == 1 ? "file" : "files") << "."
              << RESET
              << std::endl;
}

int main(int argc, char* argv[]) {
    std::string directory = (argc > 1) ? argv[1] : "."; // Default to current directory
    std::vector<ExifFile> files;
    auto proposed_name_counts_ptr = std::make_shared<std::map<std::string, int>>();

    // Collect files from the specified directory
    for (const auto& entry : fs::directory_iterator(directory)) {
        if (fs::is_regular_file(entry)) {
            ExifFile file = ExifFile(entry.path(), proposed_name_counts_ptr);
            
            // Ignore files without valid EXIF dates
            if (!file.is_skipped()) {
                files.push_back(file);
            }
            else {
                std::cout << YELLOW << "[Warning] " << RESET << "Ignoring file without valid date " << file.get_path().filename().string() << std::endl;
            }
        }
    }

    // Check if empty
    if (files.empty()) {
        std::cout << CYAN << "No files found in the specified directory." << RESET << std::endl;
        return 0;
    }

    // Sort in reverse alphabetical order (will be shown in alphabetical order)
    std::sort(files.begin(), files.end(), [](const ExifFile& a, const ExifFile& b) {
        return a.get_path() > b.get_path(); // Reverse order based on paths
    });

    bool first_loop = true;
    bool show_clash_error = false;
    while(true) {
        display_proposed_changes(files, first_loop);
        first_loop = false;

        if (show_clash_error) {
            std::cout << "\n" << RED << "[Error]: " << RESET << "Please resolve clashes before continuing" << std::endl;
            show_clash_error = false;
        }
        else std::cout << std::endl;

        std::cout << "Operations to edit (e.g., '1 2 3', '1-3', '^4'): ";
        std::string input;
        std::getline(std::cin, input);

        // Exit the loop if no options are selected
        if (input.empty()) {
            // ...unless there are clashes
            if (has_clashes(proposed_name_counts_ptr)) {
                show_clash_error = true;
                continue;
            }

            break;
        }

        std::set<size_t> selected_files; // Use a set to store unique selections
        std::istringstream stream(input);
        std::string token;

        while (stream >> token) {
            if (token.front() == '^') { // Handle range selection
                size_t start = std::stoi(token.substr(1));
                for (size_t i = start; i < files.size(); ++i) {
                    selected_files.insert(i + 1); // Add files to selection
                }
            }
            else if (token.find('-') != std::string::npos) { // Handle ranges
                auto range = token.find('-');
                size_t start = std::stoi(token.substr(0, range));
                size_t end = std::stoi(token.substr(range + 1));
                for (size_t i = start; i <= end; ++i) {
                    selected_files.insert(i); // Add files in range
                }
            }
            else { // Handle individual selections
                selected_files.insert(std::stoi(token));
            }
        }

        // Now, apply the edits for selected files
        std::vector<ExifFile> files_to_edit;
        for (const auto& index : selected_files) {
            if (index > 0 && index <= files.size()) {
                files[index - 1].edit_proposed_name();
            }
        }
        std::cout << std::endl;
    }

    // Confirm renaming
    std::string confirm;
    std::cout << "\nRename files? (y/N): ";
    std::getline(std::cin, confirm);

    if (confirm == "y" || confirm == "Y") {
        rename_files(files);
    }

    return 0;
}
