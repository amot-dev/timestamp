
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <getopt.h>
#include <iostream>
#include <map>
#include <set>
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

// Print help menu
void print_help() {
    std::cout << "Usage: timestamp [directory] [-f|--force] [-e|--exif <tag>] [-x|--xmp <tag>] [-d|--date-format <format>] [-i|--interactive] [-h|--help]" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -f, --force                     Force execution (will delete clashing files, not recommended)" << std::endl;
    std::cout << "  -e, --exif <tag>                Specify EXIF metadata date tag" << std::endl;
    std::cout << "  -x, --xmp <tag>                 Specify XMP metadata date tag" << std::endl;
    std::cout << "  -d, --date-format <format>      Specify date format for renamed files" << std::endl;
    std::cout << "  -i, --interactive               Enable interactive mode" << std::endl;
    std::cout << "  -h, --help                      Show this help message" << std::endl;
}

int main(int argc, char* argv[]) {
    // Variables to hold the argument values
    std::string directory = ".";
    std::string exif_date_tag = "Exif.Photo.DateTimeOriginal"; // Default value
    std::string xmp_date_tag = "Xmp.video.ModificationDate"; // Default value
    std::string date_format = "%Y-%m-%d-%H%M-%S"; // Default value
    bool force = false;
    bool interactive = false;

    // Option structure for getopt_long
    static struct option long_options[] = {
        {"force",       no_argument,       0,  'f' },
        {"exif",        required_argument, 0,  'e' },
        {"xmp",         required_argument, 0,  'x' },
        {"date-format", required_argument, 0,  'd' },
        {"interactive", no_argument,       0,  'i' },
        {"help",        no_argument,       0,  'h' },
        {0,             0,                 0,   0  }
    };

    int opt;
    int option_index = 0;
    opterr = 0;

    // Loop to parse command-line arguments
    while ((opt = getopt_long(argc, argv, "fe:x:d:ih", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'f':
                force = true;
                break;
            case 'e':
                exif_date_tag = optarg;
                // TODO: validate
                break;
            case 'x':
                xmp_date_tag = optarg;
                // TODO: validate
                break;
            case 'd':
                date_format = optarg;
                // TODO: validate
                break;
            case 'i':
                interactive = true;
                break;
            case 'h':
                print_help();
                return 0;
            default:
                std::cout << RED << "[ERROR] " << RESET << "Invalid option. Use -h or --help for usage" << std::endl;
                return 1;
        }
    }

    // Handle any positional argument as directory
    for (int i = optind; i < argc; ++i) {
        if (argv[i][0] != '-') {
            directory = argv[i]; // Set the first positional argument as directory
        }
        else {
            std::cout << RED << "[ERROR] " << RESET << "Unexpected option or argument: " << argv[i] << ". Use -h or --help for usage" << std::endl;
            return 1;
        }
    }

    if (force && interactive) {
        std::cout << YELLOW << "[WARNING] " << RESET << "-f or --force has no effect in interactive mode" << std::endl;
        force = false;
    }

    std::vector<ExifFile> files;
    auto proposed_name_counts_ptr = std::make_shared<std::map<std::string, int>>();

    // Collect files from the specified directory
    for (const auto& entry : fs::directory_iterator(directory)) {
        if (fs::is_regular_file(entry)) {
            ExifFile file = ExifFile(entry.path(), proposed_name_counts_ptr, exif_date_tag, xmp_date_tag, date_format);
            
            // Ignore files without valid EXIF dates
            if (!file.is_skipped()) {
                files.push_back(file);
            }
            else {
                std::cout << YELLOW << "[Warning] " << RESET << "Ignoring file without valid date " << file.get_path().filename().string() << std::endl;
            }
        }
    }

    // Check if no files found
    if (files.empty()) {
        std::cout << CYAN << "No files found in the specified directory." << RESET << std::endl;
        return 0;
    }

    // Sort by filename in reverse alphabetical order (will be shown in alphabetical order)
    std::sort(files.begin(), files.end(), [](const ExifFile& a, const ExifFile& b) {
        return a.get_path() > b.get_path(); // Reverse order based on paths
    });

    bool first_loop = true;
    bool show_clash_error = false;
    while(true) {
        display_proposed_changes(files, first_loop);
        first_loop = false;

        // Only show clash error (or add newline) in interactive mode
        if (interactive) {
            if (show_clash_error) {
                std::cout << "\n" << RED << "[Error]: " << RESET << "Please resolve clashes before continuing" << std::endl;
                show_clash_error = false;
            }
            else std::cout << std::endl;
        }

        // Only ask to edit operations in interactive mode
        std::string input;
        if (interactive) {
            std::cout << "Operations to edit (e.g., '1 2 3', '1-3', '^4'): ";
            std::getline(std::cin, input);
        }

        // Exit the loop if no options are selected (or if not in interactive mode)
        if (input.empty()) {
            // If there are clashes
            if (has_clashes(proposed_name_counts_ptr)) {
                // In interactive mode, give the user the opportunity to fix clashes
                if (interactive) {
                    show_clash_error = true;
                    continue;
                }
                // Otherwise, unless force, abort
                else if (!force) {
                    std::cout << CYAN << "Clashes detected, aborting. Use -f or --force to ignore clashes." << RESET << std::endl;
                    return 0;
                }
                else {
                    std::cout << YELLOW << "[WARNING] " << RESET << "At least once clashing file has been deleted" << std::endl;
                }
            }

            break;
        }

        std::set<size_t> selected_files; // Use a set to store unique selections
        std::istringstream stream(input);
        std::string token;

        while (stream >> token) {
            // Handle range selection
            if (token.front() == '^') {
                size_t start = std::stoi(token.substr(1));
                for (size_t i = start; i < files.size(); ++i) {
                    selected_files.insert(i + 1);
                }
            }
            // Handle ranges
            else if (token.find('-') != std::string::npos) {
                auto range = token.find('-');
                size_t start = std::stoi(token.substr(0, range));
                size_t end = std::stoi(token.substr(range + 1));
                for (size_t i = start; i <= end; ++i) {
                    selected_files.insert(i);
                }
            }
            // Handle individual selections
            else {
                selected_files.insert(std::stoi(token));
            }
        }

        // Apply the edits for selected files
        std::vector<ExifFile> files_to_edit;
        for (const auto& index : selected_files) {
            if (index > 0 && index <= files.size()) {
                files[index - 1].edit_proposed_name();
            }
        }
        std::cout << std::endl;
    }

    // If force, just do it
    if (force) {
        std::cout << std::endl; // Formatting
        rename_files(files);
    }
    // Else confirm renaming
    else {
        std::string confirm;
        std::cout << "\nRename files? (y/N): ";
        std::getline(std::cin, confirm);

        if (confirm == "y" || confirm == "Y") {
            rename_files(files);
        }
    }

    return 0;
}
