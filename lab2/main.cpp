#include "tar_parser.h"

int main(int argc, char* argv[]) {
    if (argc < 3) {
        return 1;
    }

    std::string archive_path = argv[1];
    std::string command = argv[2];

    TarManager manager(archive_path);

    if (command == "_completion_files") {
        std::string prefix = (argc > 3) ? argv[3] : "";
        manager.print_completion_files(prefix);
        return 0;
    }

    if (command == "_completion_dirs") {
        std::string prefix = (argc > 3) ? argv[3] : "";
        manager.print_completion_dirs(prefix);
        return 0;
    }

    if (command == "_is_dir") {
        if (argc < 4) {
            return 1;
        }
        return manager.is_directory(argv[3]) ? 0 : 1;
    }

    if (command == "_completion_suggest") {
        std::string prefix = (argc > 3) ? argv[3] : "";
        bool dirs_only = (argc > 4) && std::string(argv[4]) == "dirs";

        auto paths = manager.find_matching_paths(prefix, dirs_only);
        for (const auto& path : paths) {
            std::cout << path << std::endl;
        }
        return 0;
    }

    if (command == "tree") {
        manager.print_tree();
    }
    else if (command == "ls") {
        std::string path = (argc > 3) ? argv[3] : "/";
        manager.list_directory(path);
    }
    else if (command == "cat") {
        if (argc < 4) {
            std::cerr << "Error: File path required for 'cat' command\n";
            return 1;
        }
        manager.read_file_content(argv[3]);
    }
    else if (command == "extract") {
        if (argc < 4) {
            std::cerr << "Error: File path required for 'extract' command\n";
            return 1;
        }
        std::string output_path = (argc > 4) ? argv[4] : "";
        manager.extract_file(argv[3], output_path);
    }
    else if (command == "info") {
        manager.print_info();
    }
    else {
        std::cerr << "Error: Unknown command: " << command << std::endl;
        return 1;
    }

    return 0;
}