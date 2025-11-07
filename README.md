# HufPix

> A simple image compress method, based on Huffman coding.

![Build with xmake](https://img.shields.io/badge/build-xmake-blue.svg)
![Language: C++20](https://img.shields.io/badge/language-C%2B%2B20-00599C.svg)
![License: MIT](https://img.shields.io/badge/license-MIT-green.svg)

## Overview

HufPix is an image compression / decompression project centered around Huffman coding. Each pixel byte is treated as a symbol; their frequencies are counted, a Huffman tree is built to generate a code table, the image is repackaged into a custom `.hfp` file, and the same executable can restore it back to common formats (PNG/JPG/BMP/TGA, etc.).

Why it exists:

- Demonstrate the end‑to‑end process of a hand‑written Huffman encoder/decoder.
- Show low‑level data layout details via bit‑stream I/O and a custom file format.
- Provide clear C++20 code that is easy to study and extend.
- It is one of my course assignments.

## Features

- Single executable exposing `encode` and `decode` subcommands.
- Custom min‑heap and node pool (no STL heap) to highlight low‑level implementation details.
- BitStream utility supporting bit‑level write/read and alignment, making it easy to swap in other entropy coders later.
- Custom `.hfp` file stores dimensions, channel count, and the serialized Huffman tree for cross‑platform readability.
- Included unit test ensures consistency of Huffman tree serialization / deserialization.

## Quick Start

### Dependencies

- C++20 compiler (verified with GNU g++)
- [xmake](https://xmake.io) build tool
- `stb_image.h` and `stb_image_write.h` (from [nothings/stb](https://github.com/nothings/stb/))

### Build & Run

```bash
xmake f -m release   # Optional: switch between release/debug
xmake                 # Builds executable under build/linux/x86_64/<mode>/
```

Debug mode:

```bash
xmake f -m debug
xmake
```

Run tests:

```bash
xmake run test
```

## Command Line Usage

Encode (image -> `.hfp`):

```bash
xmake run HufPix encode <input-image> -o <output.hfp>
```

Decode (`.hfp` -> image):

```bash
xmake run HufPix decode <input.hfp> -o <output-image>
```

Tips:

- `<input-image>` supports any stb_image-readable format (PNG/JPG/BMP/TGA, etc.).
- The extension of `<output-image>` selects the output format; JPG output automatically drops an alpha channel.
- You must explicitly specify output with `-o` to avoid overwriting the source file.

## File Format

The byte layout of an `.hfp` file:

| Offset | Size (bytes) | Description                        |
| ------ | ------------ | ---------------------------------- |
| 0      | 6            | Magic string `HUFPIX`              |
| 6      | 2            | Version `0x0001`                   |
| 8      | 4            | Little-endian image width          |
| 12     | 4            | Little-endian image height         |
| 16     | 1            | Channel count                      |
| 17     | 1            | Reserved (currently 0)             |
| 18     | 4            | Serialized Huffman tree length `N` |
| 22     | `N`          | Huffman tree bitstream (preorder)  |
| 22+N   | 4            | Compressed bitstream length `M`    |
| ...    | `M`          | Encoded pixel data                 |

Implementation details:

- Huffman tree serialization uses preorder traversal: internal node writes a `0`; leaf writes `1` followed by the 8-bit symbol value.
- Bitstream is stored byte-aligned; trailing partial byte bits are padded with `0`.
- Pipeline design allows future replacement with arithmetic coding, ANS, etc.

## Project Layout

```
include/
	bit_io.hpp        # Bitstream & container interface declarations
	huffman.hpp       # Frequency table, heap, tree & codeword declarations
src/
	bit_io.cpp        # Bit-level read/write and compression/decompression
	huffman.cpp       # Huffman tree build, serialization & code table generation
	main.cpp          # CLI parsing and file packaging logic
test/
	test.cpp          # Huffman tree serialization consistency test
report.md           # Design & implementation notes
xmake.lua           # xmake build script
```

## Development

- Code style follows modern C++20.
- Debug run: `xmake run -d HufPix encode ...`.
- Unit tests cover tree save/load; contributions adding tests for extreme images or malformed inputs are welcome.

## License

Distributed under the [MIT License](LICENSE).
