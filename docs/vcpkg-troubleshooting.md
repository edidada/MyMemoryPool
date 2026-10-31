# vcpkg 构建（memorypool port）踩坑记录

> 记录本仓库 `.github/workflows/vcpkg.yml` 以及 `edidada/vcpkg` 的
> `memorypool-port` 分支 (`ports/memorypool/`) 的所有报错、根因与解决方案。
> 全部问题最终解决，PR run: MyMemoryPool `main` vcpkg workflow 41187373537 全绿。

---

## 目录

1. [port 下载报错 `unexpected hash` —— SHA512 与 GitHub tag tarball 不匹配](#1)
2. [用 PowerShell 重定向 `>` 保存二进制 tarball 导致算出错误哈希](#2)
3. [本地 git clone 直连 github.com 失败（Connection reset）](#3)
4. [改完 port 后 CI 仍用旧代码 —— 忘了 bump submodule 指针](#4)
5. [其他 CI 报错（同族）：shell 脚本缺可执行位](#5)
6. [版本发布的标准流程（checklist）](#6)

---

## <a id="1"></a>1. port 下载报错 `unexpected hash` —— SHA512 与 GitHub tag tarball 不匹配

### 报错（六个 triplet 全部出现，症状一致）

```text
Downloading https://github.com/edidada/MyMemoryPool/archive/1.0.0.tar.gz
  -> edidada-MyMemoryPool-1.0.0.tar.gz
error: download from https://github.com/edidada/MyMemoryPool/archive/1.0.0.tar.gz
  had an unexpected hash
CMake Error at scripts/cmake/vcpkg_download_distfile.cmake:134 (message):
  Download failed, halting portfile.
error: building memorypool:x64-linux failed with: BUILD_FAILED
```

### 根因

`portfile.cmake` 固定的 SHA512 与 `tag` tarball 实际内容不符（port 初版填的哈希
来自旧提交/手工生成，tag `1.0.0` 的 tarball 换过内容）：

```cmake
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO edidada/MyMemoryPool
    REF "${VERSION}"              # = 1.0.0 (tag)
    SHA512 6df5c0929b70...        # <-- 错误哈希，实际应为 80a088b8...
    HEAD_REF main
)
```

### 解决

先用 curl 重新下载 tag tarball 并算出真实 SHA512（见下一节的二进制坑）：

```powershell
curl.exe -sSL -o "$env:TEMP\mempool-1.0.0.tar.gz" `
  "https://github.com/edidada/MyMemoryPool/archive/1.0.0.tar.gz"
(Get-FileHash "$env:TEMP\mempool-1.0.0.tar.gz" -Algorithm SHA512).Hash.ToLower()
# 80a088b87ebef9cb3f4d7f4e39d121cef90ebf38a8232d1aa150e8a8501ceb824ec2071...
# 329f59736057f303e6fcc8f2af46a2166812931087a802d9d53fd3ad4
```

> 先校验下载完整性再上传哈希：

```powershell
tar -tzf "$env:TEMP\mempool-1.0.0.tar.gz" | Select-Object -First 5
# MyMemoryPool-1.0.0/...  路径正确且能解开才算成功
```

然后把新哈希写回 `portfile.cmake` 并推送 `memorypool-port` 分支
（参考第 4 节的远程编辑方式）。

### 关键点

- GitHub 的 `archive/<tag>.tar.gz` 与 `gh api .../tarball/<tag>`（codeload）
  对同一 commit 是同一份字节流，但**下载过程必须二进制安全**。
- 三条命全部报同一个 hash 错而不是 404，说明 tag 存在但内容哈希错——
  先查"我这台机器存下来的文件是不是完好的"，再怀疑远端。

---

## <a id="2"></a>2. 用 PowerShell 重定向 `>` 保存二进制 tarball 导致算出错误哈希

### 现象

修复了 SHA512 之后 CI 重新跑，依然报一样的 `unexpected hash`，且本地没有
任何 CMake/编译错误——问题只能在"哈希来源"这一层。

### 根因

```powershell
gh api repos/edidada/MyMemoryPool/tarball/1.0.0 > "$env:TEMP\mempool-1.0.0.tar.gz"
(Get-FileHash "$env:TEMP\mempool-1.0.0.tar.gz" -Algorithm SHA512).Hash.ToLower()
# b337b9d1d1f0fcc2971353dfb258cff73ff675c35942...  <-- 错的！
```

PowerShell 的原生命令重定向 `>` 走的是**文本编码管道**，二进制 tarball 在
解码/重编码时被转码损坏，文件内容已经不等于远端真实内容，算出的
SHA512 自然也不对。

### 解决

必须用二进制安全的方式下载：

```powershell
curl.exe -sSL -o "$env:TEMP\mempool-1.0.0.tar.gz" `
  "https://github.com/edidada/MyMemoryPool/archive/1.0.0.tar.gz"
# 或者
Invoke-WebRequest -Uri "..." -OutFile "$env:TEMP\mempool-1.0.0.tar.gz"
```

### 关键点

- 永远不要用 PowerShell 的 `>` 重定向保存二进制文件。
- 校验手段：`Get-Item` 看 `Length`、`tar -tzf` 能列出内容、
  `git -C <repo> diff --stat` 之类交叉验证。

---

## <a id="3"></a>3. 本地 git clone 直连 github.com 失败（Connection reset）

### 报错

```text
fatal: unable to access 'https://github.com/edidada/vcpkg.git/':
  Recv failure: Connection was reset
fatal: unable to access 'https://github.com/edidada/vcpkg.git/':
  Failed to connect to github.com:443 after 21065 ms: Could not connect to server
fatal: clone of 'vcpkg' into submodule path 'D:/develops/git/github/c/MyMemoryPool/vcpkg' failed
```

### 根因

本机 TCP 443 直连 github.com 时被重置（git over HTTPS 不通），但：

- SSH 22（`git@github.com:...`）正常（推、克隆都好）
- `gh api`（HTTPS）正常（其内的 curl/own authentication 路径可用）
- `curl.exe`（HTTPS）也正常

### 解决（本机不 clone，只要 GitHub Actions 能拉到就用起来）

**方案 A：本机临时走 SSH 重写（保留 .gitmodules 里的 HTTPS，CI 不受影响）**

```powershell
git config --global url.git@github.com:.insteadOf https://github.com/
# 之后 git clone / submodule update 会自动换走 SSH
```

**方案 B（本分支实际采用）：完全不在本机 clone vcpkg，一切通过 gh api 操作远端 repo**

- 改文件：`gh api -X PUT repos/edidada/vcpkg/contents/<path> --input put-body.json`
  （需要带上当前文件的 blob `sha`，见第 4 节脚本）
- 看文件：`gh api repos/edidada/vcpkg/contents/<path>?ref=<branch> --jq '.content'`
  + Base64 解码
- 本仓库 `git submodule update --init` **不跑**，本地只放一个空 `vcpkg/`
  目录让 `git status` 干净；CI 侧 `actions/checkout` `submodules: recursive`
  会自己在 runner 上 clone（runner 与 GitHub 网络是内网，无此问题）。

### 关键点

- `.gitmodules` 里 URL 保持 `https://github.com/edidada/vcpkg.git`——
  Actions 的 checkout 有 `GITHUB_TOKEN`，HTTPS 能拉；写成 SSH 反而没有。
- 如果某些机器上一定要能本地构建，用方案 A 的 insteadOf 配置，而不是
  把 `.gitmodules` 改成 SSH。

---

## <a id="4"></a>4. 改完 port 后 CI 仍用旧代码 —— 忘了 bump submodule 指针

用 submodule 方案时，`.github/workflows/vcpkg.yml` 里的
`submodules: recursive` 拉的是超仓库 `main` 提交里**固定的** gitlink SHA。
port 改完之后，照常跑 CI 仍旧：

```text
Checkout (with submodules)   ✓  <-- 拉的是旧 pin，port 还是错的哈希
```

### 解决：不用本地 clone 也能 bump submodule 指针

```powershell
# 1. 先用 gh api 改远端 vcpkg 仓库的 port 文件（先取现有 blob sha）
$oldBlobSha = gh api `
  "repos/edidada/vcpkg/contents/ports/memorypool/portfile.cmake?ref=memorypool-port" `
  --jq '.sha'

# 2. 用 gh api PUT 提交新内容（新提交记下返回的 commit sha）
$portfile = Get-Content -Raw portfile.cmake          # 或用 here-string 写好内容
$b64 = [Convert]::ToBase64String([Text.Encoding]::UTF8.GetBytes($portfile))
@{ message    = "fix(port/memorypool): correct SHA512"
   content    = $b64
   sha        = $oldBlobSha
   branch     = "memorypool-port" } | ConvertTo-Json |
  Set-Content "$env:TEMP\put-body.json" -Encoding utf8

$ff = gh api -X PUT repos/edidada/vcpkg/contents/ports/memorypool/portfile.cmake `
       --input "$env:TEMP\put-body.json" --jq '.commit.sha'
# e.g. f71649fc65ed243ea4c2bc414112f83fc25ca5d3

# 3. 本仓库不 clone submodule，直接改索引里的 gitlink 再提交
git update-index --cacheinfo 160000,$ff,vcpkg
git commit -m "ci: bump vcpkg submodule port SHA fix"
git push origin main
```

### 关键点

- `--cacheinfo 160000` 只改索引里的 gitlink 元数据，不要本地 clone。
- 本地 `vcpkg/` 目录必须是存在的（哪怕是空的），否则 `git status`
  会报 worktree 缺失 `D vcpkg`。

---

## <a id="5"></a>5. 其他 CI 报错（同族）：shell 脚本缺可执行位

Build iOS / Build Android (standalone scripts) 步骤里的经典错误：

```text
/Users/runner/work/_temp/xxx.sh: line 1: ./scripts/build-ios.sh: Permission denied
##[error]Process completed with exit code 1.
```

### 根因与解决

Windows 上 `git add` 收进来的 shell 脚本 mode 默认 `100644`，CI 没法
`./scripts/xxx.sh` 直接执行：

```powershell
git update-index --chmod=+x scripts/build-ios.sh      # 100644 -> 100755，不 clone 也可
git commit -m "ci: make build-ios.sh executable"
```

或者在 workflow 里用 `bash ./scripts/xxx.sh`（不依赖执行位）。

### 关键点

- Windows 提交的脚本，看 `git ls-files -s scripts/xxx.sh` 是否 `100755`。

---

## <a id="6"></a>6. 版本发布的标准流程（checklist）

`main` 分支往前走的同时，port 构建用的是**锁死的 tag tarball**，改了源码
不能只推 `main`——port 会继续构建旧 tag，必须走一遍：

1. `main` 提交并推送新代码；
2. 从 `main` 打新版本 tag：`git tag 1.1.0 && git push origin 1.1.0`；
3. 用 curl 二进制方式下载新 tag 的 tarball，算出 SHA512（见第 1 节）；
4. 用 `gh api -X PUT` 把 `edidada/vcpkg` 的 `ports/memorypool/vcpkg.json`
   `version` 改成 `1.1.0`，`portfile.cmake` 的 SHA512 换成新值
   （`REF "${VERSION}"` 会取到新 tag）；
5. 用 `git update-index --cacheinfo 160000,blob,vcpkg` 把本仓库 submodule
   指针 bump 成第 4 步产生的新 commit，提交并推送；
6. 跑 `.github/workflows/vcpkg.yml`（push 触发或 `workflow_dispatch`），
   用 `gh run watch <run-id> --exit-status` 看六个 triplet
   (`x64-linux / x64-windows / x64-osx / arm64-osx / arm64-ios / arm64-android`)
   是否全部绿。

### 涨版本还要注意

- 本仓库 `CMakeLists.txt` 里 `project(... VERSION ...)` 也要一起改，保持一致；
- port 里的 `REF "${VERSION}"` 依赖 tag 的名字与 `vcpkg.json` 的 `version`
  完全一致（tag：`1.1.0`，不要写成 `v1.1.0`）。

---

## 附：辅助命令速查

```powershell
# 查 CI 失败日志（只看失败步骤）
gh run list --workflow=vcpkg.yml --limit 5
gh run view <run-id> --log-failed

# 查远端 vcpkg 仓库某文件
gh api "repos/edidada/vcpkg/contents/ports/memorypool/portfile.cmake?ref=memorypool-port" `
  --jq '.content' | % { [Text.Encoding]::UTF8.GetString([Convert]::FromBase64String($_)) }

# 查本仓库 pin 的 submodule 提交
git ls-tree main vcpkg
```
