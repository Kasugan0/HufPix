#include <iostream>
#include <cstring>
#include "huffman.hpp"
#include "bit_io.hpp"


bool compareTrees(const Node* a, const Node* b)
{
    if (!a && !b) return true;
    if (!a || !b) return false;

    bool aLeaf = (!a->l && !a->r);
    bool bLeaf = (!b->l && !b->r);

    if (aLeaf != bLeaf) return false;
    if (aLeaf && a->v != b->v) return false;

    return compareTrees(a->l, b->l) && compareTrees(a->r, b->r);
}


bool testCase(const char* name, Node* pool, size_t cnt, const Node* root)
{
    (void)pool;
    std::cout << "\n=== Test: " << name << " ===" << std::endl;
    std::cout << "Input nodes: " << cnt << std::endl;

    uint8_t buffer[4096];
    memset(buffer, 0, sizeof(buffer));

    BitStream bsW(buffer, sizeof(buffer));
    std::cout << "BitStream initialized: buffer size = " << sizeof(buffer) << std::endl;

    if (!save(root, &bsW))
    {
        std::cerr << "Save failed! bytePos=" << bsW.bytePos << " bitPos=" << (int)bsW.bitPos << std::endl;
        return false;
    }

    std::cout << "Save succeeded. bytePos=" << bsW.bytePos << " bitPos=" << (int)bsW.bitPos << std::endl;

    size_t bytes = bsW.flush();
    std::cout << "Serialized: " << bytes << " bytes" << std::endl;

    BitStream bsR(buffer, bytes);
    Node newPool[50] = {};
    size_t newCnt = 0;

    std::cout << "Starting load..." << std::endl;
    Node* newRoot = load(&bsR, newPool, &newCnt, 50);

    if (!newRoot)
    {
        std::cerr << "Load failed! bytePos=" << bsR.bytePos << " bitPos=" << (int)bsR.bitPos << std::endl;
        return false;
    }

    std::cout << "Load succeeded." << std::endl;
    std::cout << "Original nodes: " << cnt << ", Loaded nodes: " << newCnt << std::endl;

    if (cnt != newCnt)
    {
        std::cerr << "Node count mismatch!" << std::endl;
        return false;
    }

    bool eq = compareTrees(root, newRoot);
    std::cout << "Trees equal: " << (eq ? "YES" : "NO") << std::endl;

    return eq;
}


int main()
{
    int passed = 0;
    int total = 0;

    {
        total++;
        Node pool[10] = {};
        pool[0] = Node{'A', 10, nullptr, nullptr};
        if (testCase("Single leaf node", pool, 1, &pool[0])) passed++;
    }

    {
        total++;
        Node pool[10] = {};
        pool[0] = Node{'A', 5, nullptr, nullptr};
        pool[1] = Node{'B', 5, nullptr, nullptr};
        pool[2] = Node{0, 10, &pool[0], &pool[1]};
        if (testCase("Two equal frequency leaves", pool, 3, &pool[2])) passed++;
    }

    {
        total++;
        Node pool[10] = {};
        pool[0] = Node{'A', 1, nullptr, nullptr};
        pool[1] = Node{'B', 2, nullptr, nullptr};
        pool[2] = Node{0, 3, &pool[0], &pool[1]};
        pool[3] = Node{'C', 3, nullptr, nullptr};
        pool[4] = Node{0, 6, &pool[2], &pool[3]};
        if (testCase("Unbalanced frequency tree", pool, 5, &pool[4])) passed++;
    }

    {
        total++;
        Node pool[20] = {};
        pool[0] = Node{'A', 5, nullptr, nullptr};
        pool[1] = Node{'B', 7, nullptr, nullptr};
        pool[2] = Node{0, 12, &pool[0], &pool[1]};
        pool[3] = Node{'C', 10, nullptr, nullptr};
        pool[4] = Node{0, 22, &pool[2], &pool[3]};
        pool[5] = Node{'D', 15, nullptr, nullptr};
        pool[6] = Node{0, 37, &pool[4], &pool[5]};
        pool[7] = Node{'E', 20, nullptr, nullptr};
        pool[8] = Node{0, 57, &pool[6], &pool[7]};
        if (testCase("Larger tree with multiple levels", pool, 9, &pool[8])) passed++;
    }

    {
        total++;
        Node pool[30] = {};
        size_t cnt = 0;

        for (size_t i = 0; i < 8; i++)
        {
            pool[cnt++] = Node{static_cast<uint8_t>('A' + i), 1, nullptr, nullptr};
        }

        MinHeap heap;
        for (size_t i = 0; i < 8; i++) heap.push(&pool[i]);

        while (heap.sz > 1)
        {
            Node* a = heap.pop();
            Node* b = heap.pop();
            pool[cnt] = Node{0, a->f + b->f, a, b};
            heap.push(&pool[cnt]);
            cnt++;
        }

        Node* root = heap.pop();
        if (testCase("Equal frequency balanced tree", pool, cnt, root)) passed++;
    }

    {
        total++;
        std::cout << "\n=== Test: Multi-round serialization ===" << std::endl;

        Node pool1[10] = {};
        pool1[0] = Node{'X', 3, nullptr, nullptr};
        pool1[1] = Node{'Y', 5, nullptr, nullptr};
        pool1[2] = Node{0, 8, &pool1[0], &pool1[1]};

        Node* root = &pool1[2];
        bool success = true;

        for (int round = 1; round <= 3; round++)
        {
            std::cout << "Round " << round << ": ";

            uint8_t buffer[128];
            memset(buffer, 0, sizeof(buffer));

            BitStream bsW(buffer, sizeof(buffer));
            if (!save(root, &bsW))
            {
                std::cerr << "Save failed in round " << round << std::endl;
                success = false;
                break;
            }

            size_t bytes = bsW.flush();

            BitStream bsR(buffer, bytes);
            Node newPool[10] = {};
            size_t newCnt = 0;
            Node* newRoot = load(&bsR, newPool, &newCnt, 10);

            if (!newRoot)
            {
                std::cerr << "Load failed in round " << round << std::endl;
                success = false;
                break;
            }

            if (!compareTrees(root, newRoot))
            {
                std::cerr << "Tree mismatch in round " << round << std::endl;
                success = false;
                break;
            }

            std::cout << "OK" << std::endl;

            memcpy(pool1, newPool, sizeof(Node) * newCnt);

            for (size_t i = 0; i < newCnt; i++)
            {
                if (pool1[i].l)
                {
                    size_t offset = pool1[i].l - newPool;
                    pool1[i].l = &pool1[offset];
                }
                if (pool1[i].r)
                {
                    size_t offset = pool1[i].r - newPool;
                    pool1[i].r = &pool1[offset];
                }
            }

            root = &pool1[newCnt - 1];
        }

        if (success) passed++;
    }

    std::cout << "\n=== Results ===" << std::endl;
    std::cout << "Passed: " << passed << "/" << total << std::endl;

    return (passed == total) ? 0 : 1;
}
