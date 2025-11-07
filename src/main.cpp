#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>

#include "bit_io.hpp"
#include "huffman.hpp"


constexpr std::string_view USAGE =
    "Usage:\n"
    "  hufpix encode [input] [-o output]\n"
    "  hufpix decode [input] [-o output]\n";
constexpr std::string_view MAGIC = "HUFPIX";
constexpr size_t TREE_SZ = 1024;

int done_enc(int status, uint8_t* tree, uint8_t* payload, uint8_t* image)
{
    if (payload) delete[] payload;
    if (tree) delete[] tree;
    if (image) stbi_image_free(image);
    return status;
}

int done_dec(int status, uint8_t* res, uint8_t* payload, uint8_t* trData)
{
    if (res) delete[] res;
    if (payload) delete[] payload;
    if (trData) delete[] trData;
    return status;
}

bool w_img(const std::string& path, int w, int h, int c, const uint8_t* data)
{
    if (path.empty() || !data || w <= 0 || h <= 0 || c <= 0) return false;
    std::string ext;
    const auto dot = path.find_last_of('.');
    if (dot != std::string::npos && dot + 1 < path.size()) ext = path.substr(dot + 1);
    for (char& ch : ext) ch = static_cast<char>(std::tolower(static_cast<uint8_t>(ch)));
    const char* outPath = path.c_str();
    if (ext == "png") return stbi_write_png(outPath, w, h, c, data, w * c) != 0;
    if (ext == "bmp") return stbi_write_bmp(outPath, w, h, c, data) != 0;
    if (ext == "tga") return stbi_write_tga(outPath, w, h, c, data) != 0;
    if (ext == "jpg" || ext == "jpeg")
    {
        if (c == 1 || c == 3) return stbi_write_jpg(outPath, w, h, c, data, 90) != 0;
        if (c == 4)
        {
            const size_t pxCnt = static_cast<size_t>(w) * static_cast<size_t>(h);
            if (pxCnt == 0) return false;
            uint8_t* rgb = new (std::nothrow) uint8_t[pxCnt * 3];
            if (!rgb) return false;
            for (size_t i = 0; i < pxCnt; ++i)
            {
                rgb[i * 3 + 0] = data[i * 4 + 0];
                rgb[i * 3 + 1] = data[i * 4 + 1];
                rgb[i * 3 + 2] = data[i * 4 + 2];
            }
            const bool ok = stbi_write_jpg(outPath, w, h, 3, rgb, 90) != 0;
            delete[] rgb;
            return ok;
        }
    }
    return false;
}


int run_encode(const std::string& inPath, const std::string& outPath)
{
    int w = 0, h = 0, c = 0;
    uint8_t* image = stbi_load(inPath.c_str(), &w, &h, &c, 0);
    uint8_t* tree = nullptr;
    uint8_t* payload = nullptr;
    if (!image) return done_enc(2, tree, payload, image);

    const size_t tot = static_cast<size_t>(w) * static_cast<size_t>(h) * static_cast<size_t>(c);
    if (!tot || tot > (SIZE_MAX - 16) / 2) return done_enc(3, tree, payload, image);

    std::fill_n(g_freq, COLOR_DEPTH, 0ULL);
    for (size_t i = 0; i < tot; ++i) g_freq[image[i]]++;

    Node* root = build_tree(nullptr);
    if (!root) return done_enc(3, tree, payload, image);
    get_codes(root, 0, 0);

    tree = new (std::nothrow) uint8_t[TREE_SZ];
    if (!tree) return done_enc(4, tree, payload, image);
    BitStream trWrt(tree, TREE_SZ);
    if (!save(root, &trWrt)) return done_enc(4, tree, payload, image);
    const size_t trBytes = trWrt.flush();
    if (trBytes > 0xFFFFFFFFu) return done_enc(4, tree, payload, image);

    const size_t payloadSz = tot * 2 + 16;
    payload = new (std::nothrow) uint8_t[payloadSz];
    if (!payload) return done_enc(4, tree, payload, image);
    BitStream payloadWrt(payload, payloadSz);
    if (!comp(&payloadWrt, image, tot)) return done_enc(4, tree, payload, image);
    const size_t payloadBytes = payloadWrt.flush();
    if (payloadBytes > 0xFFFFFFFFu) return done_enc(4, tree, payload, image);

    std::ofstream out(outPath, std::ios::binary);
    if (!out) return done_enc(2, tree, payload, image);
    auto put = [&](const void* ptr, size_t bytes)
    {
        out.write(reinterpret_cast<const char*>(ptr), static_cast<std::streamsize>(bytes));
        return static_cast<bool>(out);
    };

    uint8_t header[18] = {};
    // Layout: magic(6) | version(2) | width(4) | height(4) | channels(1) | padding
    std::copy(MAGIC.begin(), MAGIC.end(), header);
    header[6] = 0x01;
    const auto w_dim = [&](int value, int offset)
    {
        const uint32_t v = static_cast<uint32_t>(value);
        header[offset + 0] = static_cast<uint8_t>(v & 0xFF);
        header[offset + 1] = static_cast<uint8_t>((v >> 8) & 0xFF);
        header[offset + 2] = static_cast<uint8_t>((v >> 16) & 0xFF);
        header[offset + 3] = static_cast<uint8_t>((v >> 24) & 0xFF);
    };
    w_dim(w, 8);
    w_dim(h, 12);
    header[16] = static_cast<uint8_t>(c);

    if (!put(header, sizeof(header))) return done_enc(4, tree, payload, image);
    put_u32(out, static_cast<uint32_t>(trBytes));
    if (!out) return done_enc(4, tree, payload, image);
    if (trBytes && !put(tree, trBytes)) return done_enc(4, tree, payload, image);
    put_u32(out, static_cast<uint32_t>(payloadBytes));
    if (!out) return done_enc(4, tree, payload, image);
    if (payloadBytes && !put(payload, payloadBytes)) return done_enc(4, tree, payload, image);

    return done_enc(0, tree, payload, image);
}


int run_decode(const std::string& inPath, const std::string& outPath)
{
    std::ifstream in(inPath, std::ios::binary);
    if (!in) return 2;

    uint8_t* trData = nullptr, *payload = nullptr, *res = nullptr;
    uint32_t treeSz = 0, payloadSz = 0;

    uint8_t header[18];
    if (!in.read(reinterpret_cast<char*>(header), sizeof(header))) return 5;
    if (std::string_view(reinterpret_cast<char*>(header), MAGIC.size()) != MAGIC) return 3;
    if (header[6] != 0x01 || header[7] != 0x00) return 3;

    const auto r_dim = [&](int offset)
    {
        return static_cast<uint32_t>(header[offset]) |
               (static_cast<uint32_t>(header[offset + 1]) << 8) |
               (static_cast<uint32_t>(header[offset + 2]) << 16) |
               (static_cast<uint32_t>(header[offset + 3]) << 24);
    };
    const uint32_t w = r_dim(8);
    const uint32_t h = r_dim(12);
    const uint8_t c = header[16];
    if (!w || !h || !c) return 3;

    uint8_t sizeBuf[4];
    if (!in.read(reinterpret_cast<char*>(sizeBuf), 4)) return 5;
    treeSz = get_u32(sizeBuf);
    if (!treeSz) return 3;

    trData = new (std::nothrow) uint8_t[treeSz];
    if (!trData) return done_dec(5, res, payload, trData);
    if (!in.read(reinterpret_cast<char*>(trData), static_cast<std::streamsize>(treeSz))) return done_dec(5, res, payload, trData);

    if (!in.read(reinterpret_cast<char*>(sizeBuf), 4)) return done_dec(5, res, payload, trData);
    payloadSz = get_u32(sizeBuf);
    if (!payloadSz) return done_dec(3, res, payload, trData);
    payload = new (std::nothrow) uint8_t[payloadSz];
    if (!payload) return done_dec(5, res, payload, trData);
    if (!in.read(reinterpret_cast<char*>(payload), static_cast<std::streamsize>(payloadSz))) return done_dec(5, res, payload, trData);

    BitStream trRder(trData, treeSz);
    size_t used = 0;
    Node* root = load(&trRder, g_nodes, &used, COLOR_DEPTH * 2); // Reuse the shared node arena
    if (!root) return done_dec(3, res, payload, trData);

    const size_t tot = static_cast<size_t>(w) * static_cast<size_t>(h) * static_cast<size_t>(c);
    if (!tot) return done_dec(3, res, payload, trData);
    res = new (std::nothrow) uint8_t[tot];
    if (!res) return done_dec(4, res, payload, trData);
    BitStream payloadRder(payload, payloadSz);
    if (!extr(&payloadRder, res, tot, root)) return done_dec(3, res, payload, trData);
    if (!w_img(outPath, static_cast<int>(w), static_cast<int>(h), c, res)) return done_dec(4, res, payload, trData);

    return done_dec(0, res, payload, trData);
}


int main(int argc, char** argv)
{
    uint8_t err = 0;
    if (argc == 5)
    {
        const std::string mode = argv[1], input = argv[2], flag = argv[3], output = argv[4];

        if (flag != "-o") err = 1;
        else if (mode == "encode") err = run_encode(input, output);
        else if (mode == "decode") err = run_decode(input, output);
        else err = 1;
    }
    else err = 1;

    switch (err)
    {
        case 0: return 0; break;
        case 1: 
            std::cerr << "Bad Command" << std::endl;
            std::cerr << USAGE << std::endl;
            return 1; break;
        case 2: 
            std::cerr << "Failed to open" << std::endl;
            return 1; break;
        case 3: 
            std::cerr << "Invalid data" << std::endl;
            return 1; break;
        case 4: 
            std::cerr << "Failed to write" << std::endl;
            return 1; break;
        case 5: 
            std::cerr << "Failed to read" << std::endl;
            return 1; break;
        default: return 1; break;
    }
    return 0;
}
