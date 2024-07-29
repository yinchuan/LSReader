# LS Archive Reader
This repository contains code to read files within an archive format called LS. This format is used by KOEI in the game [Sangokushi Sōsōden](https://en.wikipedia.org/wiki/Sangokushi_S%C5%8Ds%C5%8Dden) and potentially other games, though I don't have access to verify this.

## Dependency
`spdlog` is used for logging. `googletest` is used for test.

## Credit
This archive format and the decompression algorithm is first reverse-engineered by van from [a Chinese game forum](https://xycq.org.cn/forum/viewthread.php?tid=34612). Maxwell from the same forum [explained the algorithm](https://xycq.org.cn/forum/viewthread.php?tid=35276&extra=&authoruid=0&page=1) in detail. Please note both links are to posts in Chinese language. Functions `getBits`, `getCode`, and `decode` are originally written by van.

## Archive Format
The LS archive format supports both compressed and uncompressed data. The file structure consists of four parts:

### 1. Format Identifier:
* The first 16 bytes of the file.
* Possible values: "Ls11" or "Ls12".

### 2. Compression Dictionary:
* The next 256 bytes.
* Used for decompressing data.

### 3. File Size Information:
* A group of 12-byte entries, each providing size information for embedded files.
* Each entry consists of:
* * 4 bytes for the compressed size.
* * 4 bytes for the original size.
* * 4 bytes for the offset of the file data within the archive.
* This section ends with 4 bytes of 0.

### 4. Included Files Data Section:
* Contains the actual data of the files included in the archive.

## test_data/Logo_p.e5
This file is a valid LS archive. It has four files of which two are compressed.