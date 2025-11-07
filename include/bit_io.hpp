#pragma once

#include <cstddef>
#include <cstdint>
#include <fstream>

#include "huffman.hpp"


struct BitStream
{
    uint8_t* buf;
    size_t sz;
    size_t bytePos;
    uint8_t curByte;
    uint8_t bitPos;  // [0, 8)

    BitStream() : buf(nullptr), sz(0), bytePos(0), curByte(0), bitPos(0) {}; // Default for safe
    BitStream(uint8_t* buf, size_t size) // Write
        : buf(buf), sz(size), bytePos(0), curByte(0), bitPos(0) {}
    BitStream(const uint8_t* buf, size_t size) // Read
        : buf(const_cast<uint8_t*>(buf)), sz(size), bytePos(0), curByte(0), bitPos(0) {}
        // reader methods never write back into buf

    size_t flush();
    // Writer
    bool w(bool bit);
    bool wbits(uint64_t bits, size_t count);

    // Reader
    bool r(bool* outBit);
    bool rbits(uint64_t* outBits, size_t count);
};

bool comp(BitStream* bs, const uint8_t* data, size_t sz);
bool extr(BitStream* bs, uint8_t* data, size_t sz, const Node* root);

void put_u32(std::ofstream& out, uint32_t value);
uint32_t get_u32(const uint8_t buf[4]);