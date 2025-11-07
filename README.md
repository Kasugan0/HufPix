# HufPix

HufPix 是一个围绕哈夫曼编码实现无损图像压缩/解压的教学型 C++ 项目。程序以像素字节作为符号统计频率，构建哈夫曼树生成码表，并将图像数据封装进自定义 `.hfp` 容器；同一可执行文件也可以将 `.hfp` 恢复成常见图像格式。

## 环境与依赖

- C++20 编译器（已在 GNU g++ 上验证）
- [xmake](https://xmake.io) 构建工具
- `stb_image.h`、`stb_image_write.h`（源码中已一并提供，用于图像读写）

## 构建

```bash
xmake f -m release   # 可选：选择生成模式（release/debug）
xmake                # 在 build/linux/x86_64/<mode>/ 生成可执行文件
```

若需要调试信息，可执行 `xmake f -m debug && xmake`。

## 命令行使用

编码（图像 -> .hfp）：

```bash
xmake run HufPix encode <input-image> -o <output.hfp>
```

解码（.hfp -> 图像）：

```bash
xmake run HufPix decode <input.hfp> -o <output-image>
```

- `<input-image>` 支持 stb_image 可读取的格式（PNG/JPG/BMP/TGA 等）。
- `<output-image>` 的扩展名决定写出的文件格式；写出 JPG 时若源数据为 RGBA 会自动去除 Alpha。
- 程序当前要求显式传入 `-o` 与输出路径。

## 项目结构

```
.
├── include/
│   ├── bit_io.hpp      # BitStream 与压缩/解压接口声明
│   └── huffman.hpp     # 频率表、堆、哈夫曼树与码字声明
├── src/
│   ├── bit_io.cpp      # 比特流读写、压缩与解压实现
│   ├── huffman.cpp     # 哈夫曼树构建、序列化与码表生成
│   └── main.cpp        # 命令行解析、文件封装/解析逻辑
├── test/
│   └── test_huffman.cpp # 序列化/反序列化一致性测试
├── xmake.lua
└── report.md
```

## `.hfp` 容器格式

| 偏移 | 字节数 | 描述                        |
| ---- | ------ | --------------------------- |
| 0    | 6      | 魔数 "HUFPIX"               |
| 6    | 2      | 版本号 0x0001               |
| 8    | 4      | little-endian 图像宽度      |
| 12   | 4      | little-endian 图像高度      |
| 16   | 1      | 通道数（1/3/4 等）          |
| 17   | 1      | 保留字段（目前写 0）        |
| 18   | 4      | 哈夫曼树序列化数据长度 N    |
| 22   | N      | 先序位流保存的哈夫曼树       |
| 22+N | 4      | 压缩比特流长度 M            |
| ...  | M      | 编码后的像素数据            |

- 树序列化采用先序遍历：内部节点写 0，叶节点写 1 后跟 8 位像素值。
- 位流按字节对齐存储，末尾不足一字节的位统一以 0 填充。

## 核心实现要点

- **静态频率表与节点池**：`g_freq` 负责记录 256 个像素值的频次，`g_nodes` 同时容纳叶节点与合并后的内部节点，减少动态分配。
- **手写小顶堆**：`MinHeap` 在不依赖 STL 的前提下提供最小堆操作，支撑哈夫曼树构建。
- **BitStream**：支持逐位写入/读取、写入位段与刷新对齐，是压缩与解压的基础设施。
- **命令行子命令**：单一可执行文件同时承担 `encode` 与 `decode`，并通过 `stb_image_write` 输出 PNG/JPG/BMP/TGA。

## 单元测试

运行树序列化一致性测试：

```bash
g++ -std=c++20 -Iinclude src/bit_io.cpp src/huffman.cpp test/test_huffman.cpp -o build/test_huffman
./build/test_huffman
```

测试覆盖多种手工构造的树形结构，验证 `save` 与 `load` 的互逆性。

## 后续拓展方向

- 增加 CLI 参数（如批量处理、默认输出路径）。
- 在 `.hfp` 中写入原始像素总数或校验信息，提升健壮性。
- 引入动态缓冲或流式处理以支持更大尺寸图像。
- 尝试替换哈夫曼为算术编码、ANS 等更高效的熵编码方案。

欢迎继续探索并根据需要扩展项目能力。
