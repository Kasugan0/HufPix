#include "bit_io.hpp"

bool BitStream::w(bool b)
{
    if (b) curByte |= (1 << (7 - bitPos));
    if (++bitPos == 8)
    {
        if (bytePos >= sz) return false;  // Overflow!
        buf[bytePos++] = curByte;
        curByte = 0;
        bitPos = 0;
    }
    return true;
}

bool BitStream::wbits(uint64_t bits, size_t count)
{
    for (size_t i = 0; i < count; i++)
    {
        bool b = (bits >> (count - 1 - i)) & 1;
        if (!w(b)) return false;
    }
    return true;
}

size_t BitStream::flush()
{
    if (bitPos > 0)
    {
        if (bytePos >= sz) return 0;  // Overflow!
        buf[bytePos++] = curByte;
        curByte = 0;
        bitPos = 0;
    }
    return bytePos;
}

bool BitStream::r(bool* outBit)
{
    if (bitPos == 0)
    {
        if (bytePos >= sz) return false;
        curByte = buf[bytePos];
    }

    *outBit = (curByte >> (7 - bitPos)) & 1;

    if (++bitPos == 8)
    {
        bytePos++;
        bitPos = 0;
    }
    return true;
}

bool BitStream::rbits(uint64_t* outBits, size_t count)
{
    *outBits = 0;
    for (size_t i = 0; i < count; i++)
    {
        bool b;
        if (!r(&b)) return false;
        *outBits = (*outBits << 1) | (b ? 1 : 0);
    }
    return true;
}


bool comp(BitStream* bs, const uint8_t* data, size_t sz)
{
    if (!bs || !data) return false;
    for (size_t i = 0; i < sz; i++)
    {
        uint8_t p = data[i];
        const Code& code = g_codes[p];
        if (code.len == 0) return false; // Invalid
        if (!bs->wbits(code.bs, code.len)) return false;
    }
    return true;
}


bool extr(BitStream* bs, uint8_t* data, size_t sz, const Node* root)
{
    if (!bs || !data || !root) return false;
    for (size_t i = 0; i < sz; i++)
    {
        const Node* node = root;
        while (node->l || node->r)
        {
            bool bit;
            if (!bs->r(&bit)) return false;
            node = bit ? node->r : node->l;
            if (!node) return false;
        }
        data[i] = node->v;
    }
    return true;
}

void put_u32(std::ofstream& out, uint32_t value)
{
    uint8_t buf[4] = {
        static_cast<uint8_t>(value & 0xFF),
        static_cast<uint8_t>((value >> 8) & 0xFF),
        static_cast<uint8_t>((value >> 16) & 0xFF),
        static_cast<uint8_t>((value >> 24) & 0xFF)};
    out.write(reinterpret_cast<const char*>(buf), 4);
}

uint32_t get_u32(const uint8_t buf[4])
{
    return buf[0]|(buf[1] << 8)|(buf[2] << 16)|(buf[3] << 24);
}