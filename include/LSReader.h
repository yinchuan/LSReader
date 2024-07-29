//
// Created by yinchuan on 26/07/24.
//

#ifndef LS11_LSREADER_H
#define LS11_LSREADER_H

#include <fstream>
#include <string>
#include <vector>
#include <filesystem>
#include <cstring>
#include <error.h>
#include <spdlog/spdlog.h>

using namespace std;

const char *IDENTIFIER_LS11 = "Ls11";
const char *IDENTIFIER_LS12 = "Ls12";

struct FileMeta {
    size_t compressedSize;
    size_t originalSize;
    size_t offset;
};

class LSReader {
    filesystem::path filepath;
    ifstream ifile;
    char dictionary[0x100];
    vector<FileMeta> fileMetas;
    bool fail;

    int lenQueue;
    char inputQueue;

    static uint32_t swapEndian(uint32_t x);

    bool decode(int originalSize, int offset, char *original);

    int getCode();

    int getBits(int len);

public:
    explicit LSReader(const string &filename);

    ~LSReader();

    size_t getNumOfFiles();

    int getFile(int index, char **buffer); // return numOfBytesRead, or -1

    bool saveFile(int index, const string &filename);

    bool isFail() const { return fail; }
};

LSReader::LSReader(const string &filename) : filepath(filename), fail(false) {
    ifile = ifstream(filepath, ios_base::in | ios_base::binary);
    if (!ifile.is_open()) {
        ifile.close();
        spdlog::error("fail to open file: {}. error: {}", filename, strerror(errno));
        fail = true;
        return;
    }

    // check if right format, read first 0x10, check if "Ls11" or "Ls12"
    char *buffer = new char[4];
    ifile.read(buffer, 4);
    if (ifile.fail()) {
        spdlog::error("fail to read first 4 bytes.");
        fail = true;
        return;
    }
    if (strncmp(buffer, IDENTIFIER_LS11, 4) != 0 && strncmp(buffer, IDENTIFIER_LS12, 4) != 0) {
        spdlog::error("{} is not LS format.", filename);
        fail = true;
        return;
    }
    delete[] buffer;

    // read dictionary
    ifile.seekg(0x10, ios_base::beg);
    if (ifile.fail()) {
        spdlog::error("fail to move read pointer to dictionary section");
        fail = true;
        return;
    }
    ifile.read(dictionary, 0x100);
    if (ifile.fail()) {
        spdlog::error("error when reading dictionary");
        fail = true;
        return;
    }

    // load file metas
    ifile.seekg(0x110, ios::beg);
    if (ifile.fail()) {
        spdlog::error("fail when move to file metas");
        fail = true;
        return;
    }
    int compressed, original, offset;
    while (true) {
        ifile.read((char *) &compressed, 4);
        ifile.read((char *) &original, 4);
        ifile.read((char *) &offset, 4);

        if (ifile.fail()) {
            spdlog::error("fail when read file metas");
            fail = true;
            return;
        }

        if (compressed == 0) {
            break;
        }
        fileMetas.push_back(FileMeta{swapEndian(compressed), swapEndian(original), swapEndian(offset)});
    }
}

LSReader::~LSReader() {
    ifile.close();
}

size_t LSReader::getNumOfFiles() {
    return fileMetas.size();
}

int LSReader::getFile(int index, char **buffer) {
    if (index > fileMetas.size()) {
        *buffer = nullptr;
        return -1;
    }
    try {
        *buffer = new char[fileMetas[index].originalSize];
    } catch (const std::bad_alloc &e) {
        *buffer = nullptr;
        spdlog::error("fail to create buffer: {}", e.what());
        return -1;
    }

    if (fileMetas[index].compressedSize == fileMetas[index].originalSize) {
        // move get pointer
        ifile.seekg(fileMetas[index].offset, ios::beg);
        if (ifile.fail()) {
            spdlog::error("fail to move to position: {}", fileMetas[index].offset);
            return false;
        }
        ifile.read(*buffer, fileMetas[index].originalSize);
        if (ifile.fail()) {
            spdlog::error("fail to copy file {}", index);
            delete[] *buffer;
            *buffer = nullptr;
            return -1;
        }
        return fileMetas[index].originalSize;
    }

    if (!decode(fileMetas[index].originalSize, fileMetas[index].offset, *buffer)) {
        spdlog::error("fail to decode file {}", index);
        delete[] *buffer;
        *buffer = nullptr;
        return -1;
    }

    return fileMetas[index].originalSize;
}

bool LSReader::saveFile(int index, const string &filename) {
    // create output file
    auto ofile = ofstream(filename, ios_base::out | ios_base::binary);
    if (!ofile.is_open()) {
        spdlog::error("fail to create output file: {}. error: {}", filename, strerror(errno));
        return false;
    }

    // decode
    char *buffer;
    int size;
    if((size=getFile(index, &buffer), size==-1)) {
        spdlog::error("Fail to decode file {}", index);
        return false;
    }

    // write to output
    ofile.write(buffer, fileMetas[index].originalSize);
    delete[] buffer;
    if (ofile.fail()) {
        spdlog::error("fail to write decompressed data to output file. error: {}", strerror(errno));
        ofile.close();
        return false;
    }

    ofile.close();
    return true;
}

uint32_t LSReader::swapEndian(uint32_t x) {
    auto *p = (unsigned char *) (&x);
    return (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
}

bool LSReader::decode(int originalSize, int offset, char *original) {
    // move get pointer
    ifile.seekg(offset, ios::beg);
    if (ifile.fail()) {
        spdlog::error("fail to move to position: {}", offset);
        return false;
    }

    int code, moveBack, copies, numOfBytes = 0;
    inputQueue = 0, lenQueue = 0;
    while (originalSize > 0) {
        code = getCode();
        if (code == -1) {
            spdlog::error("error when get code");
            return false;
        }

        if (code < 0x100) {
            *original = dictionary[code];
            original++;
            originalSize--;
            numOfBytes++;
            continue;
        }

        moveBack = code - 0x100;
        if (moveBack > numOfBytes) {
            spdlog::error("number of move back bytes is greater than already decompressed.");
            return false;
        }

        copies = getCode();
        if (copies == -1) {
            spdlog::error("error when get number of copies");
            return false;
        }

        copies += 3; // back code bytes, read copies bytes
        for (int i = 0; i < copies; i++) {
            *original = *(original - moveBack);
            original++;
        }
        originalSize -= copies;
        numOfBytes += copies;
    }
    return originalSize == 0;
}

int LSReader::getCode() {
    int code1 = 0, code2, bit, n = 0;
    do {
        bit = getBits(1);
        if (bit == -1) {
            spdlog::error("error when read code 1");
            return -1;
        }

        code1 = (code1 << 1) | bit;
        n++;
    } while (bit); // stop when bit is 0, means the end of first part

    code2 = getBits(n); // read off the same length as second part
    if (code2 == -1) {
        spdlog::error("error when read code 2");
        return -1;
    }

    return code1 + code2;
}

int LSReader::getBits(int len) {
    int ret = 0;
    while (len != 0) {
        if (lenQueue == 0) { // read one byte from file to queue
            ifile.read(&inputQueue, 1);
            if (ifile.fail()) {
                spdlog::error("error when read a byte from data section. error: {}", strerror(errno));
                return -1;
            }
            lenQueue = 8;
        }
        ret = (ret << 1) + ((inputQueue & 0x80) >> 7); // add current bit(the highest/leftmost in the queue) to ret
        lenQueue--;
        inputQueue <<= 1; // remove the highest bit
        len--;
    }
    return ret;
}

#endif //LS11_LSREADER_H
