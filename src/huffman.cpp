#include "huffman.hpp"
#include "bit_io.hpp"

uint64_t g_freq[COLOR_DEPTH];
Node g_nodes[COLOR_DEPTH * 2];
Code g_codes[COLOR_DEPTH];


void MinHeap::push(Node* x)
{
    size_t i = ++sz;
    data[i] = x;
    for (size_t fa; i > 1; i = fa)
    {
        fa = i / 2;
        if (data[fa]->f <= data[i]->f) break;
        std::swap(data[fa], data[i]);
    }
}

Node* MinHeap::pop()
{
    if (sz == 0) return nullptr;
    Node* top = data[1];
    data[1] = data[sz--];
    for (size_t i = 1, l, r, ans; ; i = ans)
    {
        l = i * 2;
        r = l + 1;
        ans = i;

        if (l <= sz && data[l]->f < data[ans]->f) ans = l;
        if (r <= sz && data[r]->f < data[ans]->f) ans = r;

        if (ans == i) break;
        std::swap(data[i], data[ans]);
    }
    return top;
}


bool save(const Node* root, BitStream* bs)
{
    if (!root) return false;

    if (!root->l && !root->r)
    {
        if (!bs->w(true)) return false; // is leaf
        if (!bs->wbits(root->v, 8)) return false;
    }
    else
    {
        if (!bs->w(false)) return false; // is not leaf
        if (!save(root->l, bs)) return false;
        if (!save(root->r, bs)) return false;
    }

    return true;
}


Node* load(BitStream* bs, Node* pool, size_t* cnt, size_t nodeSz)
{
    if (*cnt >= nodeSz) return nullptr; // Overflow!

    bool leaf;
    if (!bs->r(&leaf)) return nullptr;

    Node* node = &pool[(*cnt)++];
    *node = Node{0, 0, nullptr, nullptr};

    if (leaf)
    {
        uint64_t val;
        if (!bs->rbits(&val, 8)) return nullptr;
        node->v = static_cast<uint8_t>(val);
        return node;
    }

    node->l = load(bs, pool, cnt, nodeSz);
    if (!node->l) return nullptr;
    node->r = load(bs, pool, cnt, nodeSz);
    if (!node->r) return nullptr;
    return node;
}


Node* build_tree(size_t* outCnt)
{
    size_t cnt = 0;
    for (size_t i = 0; i < COLOR_DEPTH; ++i)
    {
        if (g_freq[i] == 0) continue;
        g_nodes[cnt++] = {static_cast<uint8_t>(i), g_freq[i], nullptr, nullptr};
        if (cnt >= COLOR_DEPTH * 2)
        {
            if (outCnt) *outCnt = cnt;
            return nullptr;
        }
    }

    if (cnt == 0)
    {
        if (outCnt) *outCnt = 0;
        return nullptr;
    }

    MinHeap heap;
    for (size_t i = 0; i < cnt; ++i) heap.push(&g_nodes[i]);

    if (cnt == 1)
    {
        if (outCnt) *outCnt = cnt;
        return &g_nodes[0];
    }

    size_t nxt = cnt;
    while (heap.sz > 1)
    {
        Node* a = heap.pop();
        Node* b = heap.pop();
        if (nxt >= COLOR_DEPTH * 2)
        {
            if (outCnt) *outCnt = nxt;
            return nullptr;
        }
        Node* fa = &g_nodes[nxt++];
        *fa = {0, a->f + b->f, a, b};
        heap.push(fa);
    }

    if (outCnt) *outCnt = nxt;
    return heap.pop();
}


void get_codes(const Node* root, uint64_t code, size_t len)
{
    if (!root) return;
    if (!root->l && !root->r)
    {
        size_t realLen = len;
        if (realLen == 0) realLen = 1;
        g_codes[root->v].bs = code;
        g_codes[root->v].len = realLen;
        return;
    }

    if (len >= 64) return;

    get_codes(root->l, (code << 1), len + 1);
    get_codes(root->r, (code << 1) | 1ULL, len + 1);
}
