#include <iostream>
#include <filesystem>
#include <spdlog/spdlog.h>
#include "LSReader.h"

using namespace std;

int main(int argc, char **argv) {
    // read file name from commandline
    if (argc < 2) {
        cerr << "Usage: ./decompressor [file_name.e5]" << endl;
    }

    string filename = argv[1];
    string basename = filesystem::path(filename).stem().string();

    LSReader ls11(filename);
    if (ls11.isFail()) {
        spdlog::error("Error when reading file {}", filename);
        return 1;
    }

    // create dir
    filesystem::path storeTo(basename);
    if (filesystem::exists(storeTo)) {
        if (filesystem::is_regular_file(storeTo)) {
            spdlog::error("Can't store decompressed files to {}. It exists but is not a directory.", storeTo.string());
            return 1;
        }
    } else {
        if (!filesystem::create_directory(storeTo)) {
            spdlog::error("failed to create directory: {}", storeTo.string());
            return 1;
        }
    }
    auto n = ls11.getNumOfFiles();
    char *temp;
    spdlog::info("There are {} files.", n);
    for (int i = 0; i < n; i++) {
        if (!ls11.saveFile(i, storeTo / (basename + to_string(i)))) {
            spdlog::error("Fail to save file {} to: {}", i, (storeTo/(basename+ to_string(i))).string());
        }
    }
    spdlog::info("finished");
}
