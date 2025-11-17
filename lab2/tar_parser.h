#pragma once
#include <string>
#include <map>
#include <filesystem>
#include <iostream>
#include <vector>
#include <fstream>
#include <algorithm>

class TarManager {
private:
    struct TarEntry {
        std::string name;
        std::string full_path;
        size_t size;
        size_t offset;
        bool is_dir;
        bool is_file;

        TarEntry() : size(0), offset(0), is_dir(false), is_file(false) {}
    };

    std::string archive_path;
    std::map<std::string, TarEntry> entries;
    std::map<std::string, std::vector<std::string>> directory_tree;

public:
    TarManager(const std::string& path) : archive_path(path) {
        parse_tar_structure();
        build_directory_tree();
    }

    bool parse_tar_structure() {
        std::ifstream file(archive_path, std::ios::binary);
        if (!file) {
            std::cerr << "Error: Cannot open archive " << archive_path << std::endl;
            return false;
        }

        char header[512];
        size_t global_offset = 0;

        while (file.read(header, 512)) {
            if (header[0] == '\0') break;

            std::string name(header, 100);
            name.erase(name.find_last_not_of('\0') + 1);

            if (name.empty()) continue;


            if (header[156] == 'L') {
                size_t long_name_size = strtoul(header + 124, nullptr, 8);
                size_t data_blocks = (long_name_size + 511) / 512;
                global_offset += 512 * (1 + data_blocks);
                file.seekg(global_offset);
                continue;
            }

            TarEntry entry;
            entry.name = get_filename(name);
            entry.full_path = name;
            entry.size = strtoul(header + 124, nullptr, 8);
            entry.offset = global_offset + 512;
            entry.is_dir = (header[156] == '5');
            entry.is_file = !entry.is_dir;

            entries[entry.full_path] = entry;

            size_t data_blocks = (entry.size + 511) / 512;
            global_offset += 512 * (1 + data_blocks);
            file.seekg(global_offset);
        }

        std::cout << "Parsed " << entries.size() << " entries from archive" << std::endl;
        return true;
    }

    void build_directory_tree() {
        directory_tree[""] = std::vector<std::string>();

        for (const auto& pair : entries) {
            const std::string& full_path = pair.first;
            const TarEntry& entry = pair.second;

            create_parent_directories(full_path);

            std::string parent_dir = get_parent_directory(full_path);
            std::string item_name = get_filename(full_path);

            if (directory_tree[parent_dir].end() ==
                std::find(directory_tree[parent_dir].begin(), directory_tree[parent_dir].end(), item_name)) {
                directory_tree[parent_dir].push_back(item_name);
                }
        }

        for (auto& pair : directory_tree) {
            std::sort(pair.second.begin(), pair.second.end());
        }
    }

    void create_parent_directories(const std::string& path) {
        std::vector<std::string> parts = split_path(path);
        std::string current_path = "";

        for (size_t i = 0; i < parts.size(); ++i) {
            std::string old_path = current_path;
            if (!current_path.empty()) current_path += "/";
            current_path += parts[i];
            if (i < parts.size() - 1) {
                if (entries.find(current_path) == entries.end()) {
                    TarEntry dir_entry;
                    dir_entry.name = parts[i];
                    dir_entry.full_path = current_path;
                    dir_entry.is_dir = true;
                    dir_entry.is_file = false;
                    entries[current_path] = dir_entry;

                    std::string parent = get_parent_directory(current_path);
                    std::string dir_name = get_filename(current_path);

                    if (std::find(directory_tree[parent].begin(), directory_tree[parent].end(), dir_name) ==
                        directory_tree[parent].end()) {
                        directory_tree[parent].push_back(dir_name);
                        }
                }
            }
        }
    }

    void print_tree(const std::string& path = "/", int depth = 0, const std::string& prefix = "") {
    std::string search_path = path;
    if (search_path == "/") search_path = "";

    if (directory_tree.find(search_path) == directory_tree.end()) {
        if (search_path.empty() && directory_tree.find("/") != directory_tree.end()) {
            search_path = "/";
        } else {
            return;
        }
    }

    const auto& items = directory_tree[search_path];

    for (size_t i = 0; i < items.size(); ++i) {
        bool is_last = (i == items.size() - 1);
        std::string current_prefix = prefix + (is_last ? "└── " : "├── ");

        std::string item_path;
        if (search_path.empty() || search_path == "/") {
            item_path = items[i];
        } else {
            item_path = search_path + "/" + items[i];
        }

        auto entry_it = entries.find(item_path);
        bool is_directory = false;

        if (entry_it != entries.end()) {
            is_directory = entry_it->second.is_dir;
        } else {
            is_directory = (directory_tree.find(item_path) != directory_tree.end());
        }

        if (is_directory) {
            std::cout << current_prefix << "\033[1;34m" << items[i] << "\033[0m/" << std::endl;

            std::string new_prefix = prefix + (is_last ? "    " : "│   ");
            print_tree(item_path, depth + 1, new_prefix);
        } else {
            std::cout << current_prefix << "\033[1;32m" << items[i] << "\033[0m";
            if (entry_it != entries.end() && entry_it->second.size > 0) {
                std::cout << " (" << format_size(entry_it->second.size) << ")";
            }
            std::cout << std::endl;
        }
    }
}

    void list_directory(const std::string& path = "/") {
        std::string search_path = path;
        if (search_path.empty() || search_path == ".") search_path = "/";

        if (directory_tree.find(search_path) == directory_tree.end()) {
            std::cerr << "Error: Directory not found: " << path << std::endl;
            return;
        }

        const auto& items = directory_tree[search_path];

        std::cout << "Contents of " << (search_path == "/" ? "root" : search_path) << ":" << std::endl;
        std::cout << std::string(50, '-') << std::endl;

        for (const auto& item : items) {
            std::string item_path = search_path;
            if (search_path != "/") item_path += "/";
            item_path += item;

            const TarEntry& entry = entries[item_path];

            if (entry.is_dir) {
                std::cout << "\033[1;34m" << std::setw(30) << std::left << (item + "/") << "\033[0m";
                std::cout << "<DIR>" << std::endl;
            } else {
                std::cout << "\033[1;32m" << std::setw(30) << std::left << item << "\033[0m";
                std::cout << std::setw(12) << std::right << format_size(entry.size);
                std::cout << std::endl;
            }
        }
    }

    void extract_file(const std::string& file_path, const std::string& output_path = "") {
        if (entries.find(file_path) == entries.end()) {
            std::cerr << "Error: File not found: " << file_path << std::endl;
            return;
        }

        const TarEntry& entry = entries[file_path];
        if (entry.is_dir) {
            std::cerr << "Error: " << file_path << " is a directory" << std::endl;
            return;
        }

        std::string output_file = output_path.empty() ? get_filename(file_path) : output_path;

        std::ifstream archive(archive_path, std::ios::binary);
        if (!archive) {
            std::cerr << "Error: Cannot open archive for reading" << std::endl;
            return;
        }

        archive.seekg(entry.offset);
        std::vector<char> buffer(entry.size);
        archive.read(buffer.data(), entry.size);

        std::ofstream outfile(output_file, std::ios::binary);
        if (!outfile) {
            std::cerr << "Error: Cannot create output file: " << output_file << std::endl;
            return;
        }

        outfile.write(buffer.data(), entry.size);
        std::cout << "Extracted: " << file_path << " -> " << output_file
                  << " (" << format_size(entry.size) << ")" << std::endl;
    }

    void read_file_content(const std::string& file_path) {
        if (entries.find(file_path) == entries.end()) {
            std::cerr << "Error: File not found: " << file_path << std::endl;
            return;
        }

        const TarEntry& entry = entries[file_path];
        if (entry.is_dir) {
            std::cerr << "Error: " << file_path << " is a directory" << std::endl;
            return;
        }

        if (entry.size == 0) {
            std::cout << "File is empty: " << file_path << std::endl;
            return;
        }

        std::ifstream archive(archive_path, std::ios::binary);
        if (!archive) {
            std::cerr << "Error: Cannot open archive for reading" << std::endl;
            return;
        }

        archive.seekg(entry.offset);
        std::vector<char> buffer(entry.size);
        archive.read(buffer.data(), entry.size);

        std::cout << "Content of " << file_path << " (" << format_size(entry.size) << "):" << std::endl;
        std::cout << std::string(50, '-') << std::endl;
        std::cout << std::string(buffer.begin(), buffer.end()) << std::endl;
    }

    void print_info() {
        size_t total_files = 0;
        size_t total_dirs = 0;
        size_t total_size = 0;

        for (const auto& pair : entries) {
            if (pair.second.is_dir) {
                total_dirs++;
            } else {
                total_files++;
                total_size += pair.second.size;
            }
        }

        std::cout << "Archive: " << archive_path << std::endl;
        std::cout << "Total entries: " << entries.size() << std::endl;
        std::cout << "Directories: " << total_dirs << std::endl;
        std::cout << "Files: " << total_files << std::endl;
        std::cout << "Total size: " << format_size(total_size) << std::endl;
    }

    void print_completion_files(const std::string& prefix = "") {
        for (const auto& pair : entries) {
            const std::string& path = pair.first;
            const TarEntry& entry = pair.second;

            if (entry.is_dir) {
                std::string dir_path = path + "/";
                if (prefix.empty() || dir_path.find(prefix) == 0) {
                    std::cout << dir_path << std::endl;
                }
            } else {
                if (prefix.empty() || path.find(prefix) == 0) {
                    std::cout << path << std::endl;
                }
            }
        }
    }

    void print_completion_dirs(const std::string& prefix = "") {
        for (const auto& pair : entries) {
            const std::string& path = pair.first;
            const TarEntry& entry = pair.second;

            if (entry.is_dir) {
                std::string dir_path = path + "/";
                if (prefix.empty() || dir_path.find(prefix) == 0) {
                    std::cout << dir_path << std::endl;
                }
            }
        }

        if (prefix.empty() || "/" == prefix) {
            std::cout << "/" << std::endl;
        }
    }

    bool is_directory(const std::string& path) {
        auto it = entries.find(path);
        if (it != entries.end()) {
            return it->second.is_dir;
        }
        return false;
    }

    std::vector<std::string> find_matching_paths(const std::string& prefix, bool directories_only = false) {
        std::vector<std::string> result;

        for (const auto& pair : entries) {
            const std::string& path = pair.first;
            const TarEntry& entry = pair.second;

            if (directories_only && !entry.is_dir) continue;

            if (path.find(prefix) == 0) {
                if (entry.is_dir) {
                    result.push_back(path + "/");
                } else {
                    result.push_back(path);
                }
            }
        }


        if (prefix.empty() || prefix == "/") {
            result.push_back("/");
        }

        std::sort(result.begin(), result.end());
        return result;
    }

private:
    std::string get_filename(const std::string& path) {
        size_t pos = path.find_last_of('/');
        if (pos == std::string::npos) return path;
        return path.substr(pos + 1);
    }

    std::string get_parent_directory(const std::string& path) {
        if (path.empty()) return "";

        size_t pos = path.find_last_of('/');
        if (pos == std::string::npos) return "";
        return path.substr(0, pos);
    }

    std::vector<std::string> split_path(const std::string& path) {
        std::vector<std::string> result;
        std::stringstream ss(path);
        std::string item;

        while (std::getline(ss, item, '/')) {
            if (!item.empty()) {
                result.push_back(item);
            }
        }

        return result;
    }

    std::string format_size(size_t bytes) {
        const char* sizes[] = {"B", "KB", "MB", "GB"};
        int order = 0;
        double size = bytes;

        while (size >= 1024 && order < 3) {
            order++;
            size /= 1024;
        }

        std::stringstream ss;
        ss << std::fixed << std::setprecision(2) << size << " " << sizes[order];
        return ss.str();
    }

};
