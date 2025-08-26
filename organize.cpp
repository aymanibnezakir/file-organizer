/*
  This program organizes files in a specified directory by moving them into subfolders
  based on their file extensions. It can be run with a command-line argument.
  Designed to be cross-platform and will work on Windows, macOS, and Linux.

  WARNING:
  - Do not use this program on secure OS folders. It may lead to a system failure.
  - Only use on folders created by you, in the Downloads, or in the Desktop folder.
  - The author will not be responsible for any consequences.

  Copyright (C) 2025
  "organizer" is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

  Author: https://www.github.com/aymanibnezakir
*/

#include <iostream>
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <filesystem>
#include <algorithm>
#include <cctype>

// Use the filesystem namespace for convenience
namespace fs = std::filesystem;

// --- Configuration: Mapping of folder names to their file extensions ---
const std::unordered_map<std::string, std::vector<std::string>> FOLDER_MAP = {
    {"Programs", {".exe", ".msi", ".bat", ".sh", ".apk", ".app", ".jar", ".cmd", ".gadget", ".wsf", ".deb", ".rpm", ".bin", ".com", ".vbs", ".ps1"}},
    {"Documents", {".pdf", ".doc", ".docx", ".txt", ".ppt", ".pptx", ".xls", ".xlsx", ".odt", ".csv", ".rtf", ".tex", ".epub", ".md", ".log", ".json", ".xml", ".yaml", ".yml", ".ini"}},
    {"Compressed", {".zip", ".rar", ".7z", ".tar", ".gz", ".bz2", ".xz", ".iso", ".cab", ".arj", ".lzh", ".ace", ".uue", ".tar.gz", ".tar.bz2", ".tar.xz"}},
    {"Music", {".mp3", ".wav", ".aac", ".flac", ".ogg", ".m4a", ".wma", ".alac", ".amr", ".aiff", ".opus", ".mid", ".midi"}},
    {"Video", {".mp4", ".mkv", ".avi", ".mov", ".wmv", ".flv", ".webm", ".mpeg", ".mpg", ".m4v", ".3gp", ".3g2", ".vob", ".ogv", ".rm", ".rmvb", ".ts", ".m2ts"}},
    {"Images", {".jpg", ".jpeg", ".png", ".gif", ".bmp", ".tiff", ".tif", ".webp", ".svg", ".ico", ".heic", ".raw", ".psd", ".ai", ".indd", ".eps", ".jfif", ".apng", ".avif", ".cr2", ".nef", ".orf", ".sr2"}},
    {"Others", {}} // For unknown or uncategorized extensions
};

// --- Pre-computed map for faster lookups ---
std::unordered_map<std::string, std::string> g_extension_to_folder_map;

/**
 * @brief Populates the global extension-to-folder map for efficient lookups.
 * This creates a reverse map from the FOLDER_MAP constant.
 */
void build_extension_map() {
    for (const auto& [folder, extensions] : FOLDER_MAP) {
        for (const auto& ext : extensions) {
            g_extension_to_folder_map[ext] = folder;
        }
    }
}

/**
 * @brief Ensures that all required destination folders exist in the base path.
 * If they don't exist, they are created. This function handles potential errors
 * during directory creation.
 * @param base_path The root directory where folders should be created.
 */
void ensure_folders(const fs::path& base_path) {
    for (const auto& [folder_name, _] : FOLDER_MAP) {
        fs::path folder_path = base_path / folder_name;
        try {
            if (!fs::exists(folder_path)) {
                fs::create_directory(folder_path);
            }
        } catch (const fs::filesystem_error& e) {
            // Throw a runtime_error to be caught by the calling function
            throw std::runtime_error("Error creating directory " + folder_path.string() + ": " + e.what());
        }
    }
}

/**
 * @brief Determines the target folder for a given file extension using the pre-computed map.
 * @param file_ext The file extension (e.g., ".txt", ".pdf").
 * @return The name of the folder where the file should be moved. Returns "Others" if not found.
 */
std::string get_target_folder(std::string_view file_ext) {
    std::string lower_ext;
    lower_ext.reserve(file_ext.length());
    std::transform(file_ext.begin(), file_ext.end(), std::back_inserter(lower_ext),
                   [](unsigned char c){ return std::tolower(c); });

    auto it = g_extension_to_folder_map.find(lower_ext);
    if (it != g_extension_to_folder_map.end()) {
        return it->second;
    }
    return "Others";
}

/**
 * @brief Organizes all files in the given base path.
 * It iterates through each item, and if it's a file, moves it to the appropriate subfolder.
 * @param base_path The directory whose files need to be organized.
 * @param self_path The full path to the executable to avoid moving itself.
 */
void organize_files(const fs::path& base_path, const fs::path& self_path) {
    ensure_folders(base_path);
    for (const auto& entry : fs::directory_iterator(base_path)) {
        // We only want to move files, not directories or the program itself
        // Use weakly_canonical to resolve paths for comparison
        if (entry.is_regular_file() && fs::weakly_canonical(entry.path()) != self_path) {
            fs::path item_path = entry.path();
            std::string ext = item_path.extension().string();

            // Skip files with no extension
            if (ext.empty()) {
                continue;
            }

            std::string folder = get_target_folder(ext);
            fs::path target_dir = base_path / folder;
            fs::path target_path = target_dir / item_path.filename();

            try {
                // Avoid overwriting files with the same name.
                if (!fs::exists(target_path)) {
                    fs::rename(item_path, target_path);
                } else {
                    std::cout << "Skipping '" << item_path.filename().string() << "': file already exists in '" << folder << "' folder." << std::endl;
                }
            } catch (const fs::filesystem_error& e) {
                // Report error for the specific file and continue with others
                std::cerr << "Error moving file '" << item_path.filename().string() << "': " << e.what() << std::endl;
            }
        }
    }
}

/**
 * @brief Trims leading/trailing whitespace and specified quote characters from a string.
 * @param str The string to trim.
 * @return The trimmed string as a string_view.
 */
std::string_view trim_path(std::string_view str) {
    const std::string_view whitespace = " \t\n\r\f\v";
    const std::string_view quotes = "\"'";

    size_t first = str.find_first_not_of(whitespace);
    if (std::string::npos == first) return "";
    size_t last = str.find_last_not_of(whitespace);
    str = str.substr(first, (last - first + 1));

    first = str.find_first_not_of(quotes);
    if (std::string::npos == first) return "";
    last = str.find_last_not_of(quotes);
    return str.substr(first, (last - first + 1));
}


/**
 * @brief Displays a help message for the program's usage.
 */

void show_help() {
    std::cout << "Usage: organize <folder_path> OR organize -c" << std::endl;
    std::cout << "Organizes files in the specified folder into subdirectories based on file type." << std::endl;
    std::cout << "\nOptions/Flags:\n";
    std::cout << "  --help, -h, -H   :   Show this help message." << std::endl;
    std::cout << "  --current, -c, -C:   Organize files in the current working directory." << std::endl;
}

int main(int argc, char* argv[]) {
    build_extension_map();

    if (argc < 2) {
        show_help();
        return 0;
    }

    std::string_view arg1 = argv[1];
    if (arg1 == "--help" || arg1 == "-h" || arg1 == "-H") {
        show_help();
        return 0;
    }

    fs::path folder_path;
    if (arg1 == "-c" || arg1 == "--current" || arg1 == "-C") {
        folder_path = fs::current_path();
    } else {
        folder_path = fs::path(trim_path(arg1));
    }

    if (!fs::exists(folder_path)) {
        std::cerr << "Error: The specified path does not exist: '" << folder_path.string() << "'" << std::endl;
        return 1;
    }
    if (!fs::is_directory(folder_path)) {
        std::cerr << "Error: The specified path is not a directory: '" << folder_path.string() << "'" << std::endl;
        return 1;
    }

    try {
        folder_path = fs::canonical(folder_path);
        fs::path self_path = fs::weakly_canonical(fs::path(argv[0]));

        std::cout << "Organizing files in '" << folder_path.string() << "'..." << std::endl;
        organize_files(folder_path, self_path);
        std::cout << "File organization complete." << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "An unexpected error occurred: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
