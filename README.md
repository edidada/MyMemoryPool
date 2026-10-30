# MyMemoryPool

一个用 C 写的小玩具内存池，灵感来自 `readmd.txt` 里的内存分配器教程
（从最简单的 free-list 一直讲到真正的 slab 分配器）。

> 完整教程原文见仓库里的 `readmd.txt`。

## 这个池子做了什么

实现的是文章第 3 步的「定长 FreeList 池 + 多尺寸档位」思路：

- 一组定长 free-list 分配器，每个对应一个尺寸档位：
  `8, 16, 32, 64, 128, 256, 512, 1K, 2K, 4K, 8K, 16K, 32K, 64K` 字节。
- 分配时选最小的够用档位，从它的 free list 弹出一个对象返回；该档位空了就
  向系统要一大块、切成多个对象填回 free list。
- 每个对象头部带一个 cookie（记录归属档位），位于用户指针之前，所以
  `mp_free()` 能直接知道该还给哪个 free list，不需要额外查表。
  模式：`ptr[-sizeof(header)]`。
- 超过 64KB 的对象直接交给系统 `malloc`；其余内存池化，整个池销毁时才还给系统
  （和 memcached 的 slab 同样的取舍）。

它故意写得很简单、单线程——就像文章里说的「闲着蛋疼写来玩玩」。

## 用 Autotools 构建（本分支 `autotools`)

需要 autoconf + automake。先生成构建系统，再 configure / make：

```sh
./bootstrap.sh        # 等价于 autoreconf -i
./configure           # 想换编译器：CC=clang ./configure
make                  # 编译 libmempool.a 和测试程序
test_mempool 由 make check 编译并运行
make check            # 运行测试，退出码非 0 即失败
```

GitHub Actions（`.github/workflows/autotools.yml`）在 `autotools` 分支推送 /
PR 时，用 gcc 和 clang 自动跑上面的流程。

## 用 CMake 构建（见 `cmake` 分支）

需要 CMake 3.16+ 与支持 C11 的编译器：

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --verbose --output-on-failure
```

构建生成 `mempool` 静态库和 `test_mempool` 测试程序。CTest 检查程序退出码及
`ALL TESTS PASSED` 输出，覆盖基本分配、空指针处理、多种大小、内存复用及
20 万次随机分配/释放。`.github/workflows/cmake.yml` 在 `cmake` 分支推送、
Pull Request 或手动触发时，验证 Linux、Windows、macOS 的 Debug/Release 构建与运行。

## 用普通 Makefile 构建（见 `main` 分支）

```sh
make            # 用 gcc 编译测试程序
make run        # 运行测试
make CC=clang run
```

## API

```c
mempool_t *mempool_create(void);
void       mempool_destroy(mempool_t *pool);
void      *mp_alloc(mempool_t *pool, size_t size);
void       mp_free(mempool_t *pool, void *ptr);
size_t     mempool_allocated_count(mempool_t *pool);
```

## 目录结构

| 文件                              | 作用                                          |
|-----------------------------------|-----------------------------------------------|
| `mempool.h`                       | 公开 API 与设计说明。                         |
| `mempool.c`                       | 实现（free-list 池 + cookie 头）。            |
| `test_mempool.c`                  | 正确性与压测。                                |
| `configure.ac` / `Makefile.am`    | Autotools 构建定义（本分支）。                |
| `bootstrap.sh`                    | 运行 `autoreconf -i` 重新生成构建系统。       |
| `CMakeLists.txt`                  | CMake 构建定义（`cmake` 分支）。              |
| `.github/workflows/autotools.yml` | Autotools CI（gcc/clang + make check）。      |
| `.github/workflows/cmake.yml`     | CMake CI（跨平台，`cmake` 分支）。            |
| `.github/workflows/ci.yml`        | `main` 分支上基于普通 Makefile 的 CI。        |
