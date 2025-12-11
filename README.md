# gomoku
国科大C++大作业-五子棋程序

## 项目简介

C++ 大作业，一个五子棋程序，支持双人、人机对战，使用 C++26 + module 特性编写。

AI 部分是 minimax + alpha-beta 剪枝，有 zobrist 缓存，能够并行搜索。

## 依赖

Clang >= 20.0, MSVC 没测过

[stdexec](https://github.com/nvidia/stdexec)

[spdlog](https://github.com/gabime/spdlog)

使用 xmake 构建，cmake 支持是坏的

## 编译 / 运行

### Linux

一键构建：

```bash
$ bash ./build.sh
```

运行：

```bash
$ xmake run
```

或者

```bash
$ ./bin/gomoku
```

### Windows

TODO

## 问题

难度选择，选 difficult 肯定会超时，medium 可能会超时
