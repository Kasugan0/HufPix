#pragma once

#include <cstddef>
#include <cstdint>

constexpr size_t COLOR_DEPTH = 256;

struct Node
{
    uint8_t v;
    uint64_t f; // freq
    Node* l;
    Node* r;
};

struct Code
{
    uint64_t bs;
    size_t len;
};

extern uint64_t g_freq[COLOR_DEPTH];
extern Node g_nodes[COLOR_DEPTH * 2];
extern Code g_codes[COLOR_DEPTH];

struct MinHeap
{
    Node* data[COLOR_DEPTH * 2 + 1];
    size_t sz;

    MinHeap() : sz(0) {};

    void push(Node*);
    Node* pop();
};

bool save(const Node* root, struct BitStream* bs);
Node* load(struct BitStream* bs, Node* pool, size_t* cnt, size_t poolSz);

Node* build_tree(size_t* outCount);
void get_codes(const Node* root, uint64_t code, size_t len);