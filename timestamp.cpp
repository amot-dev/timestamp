
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "exif_file.h"

namespace fs = std::filesystem;

// Display proposed file name changes
void display_proposed_changes(const std::vector<ExifFile>& files) {
    std::cout << "Files to rename:" << std::endl;
    for (size_t i = files.size(); i > 0; --i) {
        std::string current_name = files[i-1].get_path().filename().string();
        std::string proposed_name = files[i-1].get_proposed_name();

        std::cout << i << "\t" << current_name << " -> " << proposed_name;

        if (current_name == proposed_name) std::cout << " (no change)" << std::endl;
        else if (files[i-1].is_skipped()) std::cout << " (skipped)" << std::endl;
        else std::cout << std::endl;
    }
}

// Rename files
void rename_files(std::vector<ExifFile>& files) {
    int skip_count = 0;
    for (auto file : files) {
        if (file.is_skipped()) {
            skip_count++;
            continue;
        }

        file.rename();
    }
    int rename_count = files.size() - skip_count;
    std::cout << "Renamed " << rename_count << " files. Skipped " << skip_count << " files." << std::endl;
}

int main(int argc, char* argv[]) {
    std::string directory = (argc > 1) ? argv[1] : "."; // Default to current directory
    std::vector<ExifFile> files;

    // Collect files from the specified directory
    for (const auto& entry : fs::directory_iterator(directory)) {
        if (fs::is_regular_file(entry)) {
            ExifFile file = ExifFile(entry.path());
            
            // Ignore files without valid EXIF dates
            if (!file.is_skipped()) {
                files.push_back(entry.path());
            }
            else {
                std::cout << "Warning: Ignoring file without valid date " << file.get_path().filename().string() << std::endl;
            }
        }
    }


    if (files.empty()) {
        std::cout << "No files found in the specified directory." << std::endl;
        return 0;
    }

    while(true) {
        display_proposed_changes(files);

        std::cout << "\nOperations to edit (e.g., '1 2 3', '1-3', '^4'): ";
        std::string input;
        std::getline(std::cin, input);

        // Exit the loop if no options are selected
        if (input.empty()) {
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
        std::cout << "\n" << std::endl;
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
