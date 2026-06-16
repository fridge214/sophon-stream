# BM1684X 使用 TDL_SDK 编译运行指南

本文档说明如何在 BM1684X 设备上使用 `tdl_sdk` 调用 `tdl_models` 中的模型，并尽量避免影响当前 `sophon-stream` 的编译和运行环境。

## 1. 背景说明

`tdl_models` 仓库中的模型并不一定都能直接放进 `sophon-stream` 的 YOLO 插件中运行。以 `yolov8n_det_person_vehicle_384_640_INT8_bm1684x.bmodel` 为例，它的输出是多个原始特征图，需要配套的 TDL_SDK 前处理和后处理逻辑；而当前 `sophon-stream` 的 `yolov8` / `yolo8-test` 插件期望的是 YOLOv8 导出后的特定输出格式。

因此，如果目标是验证 `tdl_models` 中模型本身能否在 BM1684X 上跑通，推荐先使用 `tdl_sdk` 的示例程序独立验证。确认模型、输入、输出和后处理逻辑都正确后，再考虑是否将 TDL_SDK 的逻辑封装为 sophon-stream 插件。

## 2. 目录规划

为了不影响 `sophon-stream`，建议把 TDL_SDK 放到独立目录中，例如：

```bash
/data/tdl_isolated/
├── tdl_sdk/              # 部署后的 TDL_SDK
├── tdl_models/           # BM1684X 模型
├── test_images/          # 测试图片
└── run_scripts/          # 独立运行脚本
```

不要把 TDL_SDK 的库文件复制到：

```bash
/data/sophon-stream/build/lib
/usr/lib
/usr/local/lib
/opt/sophon
```

也不要把 TDL_SDK 的环境变量长期写入 `~/.bashrc`。建议用独立脚本临时设置 `LD_LIBRARY_PATH`。

## 3. 需要准备的内容

### 3.1 x86 编译主机

建议使用 Ubuntu x86_64 主机进行交叉编译。需要安装：

```bash
sudo apt update
sudo apt install -y git cmake ninja-build gcc g++ device-tree-compiler \
  libssl-dev ssh bison flex unzip
```

### 3.2 SOPHON SDK 包

TDL_SDK 官方文档通常以 `SDK-24.04.01` 为示例。如果你手头是 `SDK-26.03.01`，也可以尝试使用，但需要确认 SDK 包中包含 BM1684X SoC 交叉编译所需文件。

在 x86 主机上执行：

```bash
unzip -l SDK-26.03.01.zip | grep -E "libsophon_.*aarch64|sophon-mw-soc_.*aarch64"
```

需要能看到类似文件：

```text
libsophon_..._aarch64.tar.gz
sophon-mw-soc_..._aarch64.tar.gz
```

如果 SDK 包中有这些文件，说明它具备 TDL_SDK 编译脚本通常需要的 libsophon 和 sophon-mw 依赖。

### 3.2.1 只上传 SDK 中必要文件

如果完整 `SDK-26.03.01.zip` 太大，不方便传到 Linux x86 编译机，可以只上传 BM1684X 编译需要的两个压缩包。

根据 TDL_SDK 的 `scripts/extract_sophon_sdk.sh`，BM1684X 分支实际查找：

```bash
libsophon_*_aarch64.tar.gz
sophon-mw-soc_*_aarch64.tar.gz
```

如果目标是 BM1684X SoC 盒子，建议优先使用 `sophon-img` 目录中的 SoC 版本 libsophon：

```bash
libsophon_soc_*_aarch64.tar.gz
sophon-mw-soc_*_aarch64.tar.gz
```

原因是 sophon-stream 在 SoC 交叉编译时通常也是使用 `sophon-img` 中的 `libsophon_soc_*_aarch64.tar.gz` 来整理目标端 SDK。`libsophon_soc_*` 也能匹配 TDL_SDK 脚本中的 `libsophon_*_aarch64.tar.gz` 通配符，因此不需要修改 TDL_SDK 官方脚本。

也就是说，如果你已经在本地解压了 SDK 目录，只需要从 SDK 目录中找到并传输这两个文件即可，不需要传完整 SDK.zip。

在 Windows PowerShell 中可以这样查找：

```powershell
Get-ChildItem -Path "D:\SDK-26.03.01" -Recurse -Filter "libsophon_soc_*_aarch64.tar.gz"
Get-ChildItem -Path "D:\SDK-26.03.01" -Recurse -Filter "sophon-mw-soc_*_aarch64.tar.gz"
```

请把 `"D:\SDK-26.03.01"` 替换成你本地实际 SDK 解压目录。

找到后，建议将这两个文件上传到 x86 编译机的统一目录：

```bash
/data/tdl_build_bm1684x/sdk_minimal/
├── libsophon_soc_..._aarch64.tar.gz
└── sophon-mw-soc_..._aarch64.tar.gz
```

如果从 Windows 上传，可以使用：

```powershell
scp "D:\SDK-26.03.01\...\libsophon_soc_..._aarch64.tar.gz" root@192.168.205.136:/data/tdl_build_bm1684x/sdk_minimal/
scp "D:\SDK-26.03.01\...\sophon-mw-soc_..._aarch64.tar.gz" root@192.168.205.136:/data/tdl_build_bm1684x/sdk_minimal/
```

上传到 x86 编译机后，在 x86 编译机上重新打一个小 zip：

```bash
cd /data/tdl_build_bm1684x/sdk_minimal
zip -r ../SDK-26.03.01-bm1684x-minimal.zip \
  libsophon_soc_*_aarch64.tar.gz \
  sophon-mw-soc_*_aarch64.tar.gz
```

后续仍然可以使用官方脚本：

```bash
cd /data/tdl_build_bm1684x/tdl_sdk
./scripts/extract_sophon_sdk.sh /data/tdl_build_bm1684x/SDK-26.03.01-bm1684x-minimal.zip BM1684X
```

这种方式的好处是：

- 不需要上传完整 SDK.zip。
- 不需要修改 TDL_SDK 官方脚本。
- 编译输入文件集中在 `/data/tdl_build_bm1684x`，便于清理和复现。

注意：如果你的 SDK 解压目录中没有这两个 `.tar.gz` 文件，说明 SDK 包结构可能不同。此时需要先在 SDK 目录中搜索：

```powershell
Get-ChildItem -Path "D:\SDK-26.03.01" -Recurse | Where-Object {
  $_.Name -match "libsophon.*aarch64|sophon-mw.*aarch64"
}
```

如果找到的是已经解压后的目录，而不是 `.tar.gz` 文件，也可以在 Linux x86 机器上手动整理为 TDL_SDK 期望的依赖目录：

```bash
/data/tdl_build_bm1684x/tdl_sdk/dependency/BM1684X/
├── libsophon/
├── sophon-ffmpeg/
└── sophon-opencv/
```

但优先推荐上传两个原始 `.tar.gz` 文件，再重新打 minimal zip，因为这条路线最接近官方脚本。

### 3.3 TDL_SDK 源码

```bash
mkdir -p /data/tdl_build_bm1684x
cd /data/tdl_build_bm1684x

git clone https://github.com/sophgo/host-tools.git
git clone https://github.com/sophgo/tdl_sdk.git
```

如果网络较慢，可以只下载 `tdl_sdk`，但 `host-tools` 建议保留在同级目录。

### 3.4 BM1684X 模型

从 `tdl_models` 获取 BM1684X 模型，例如：

```bash
tdl_models/
└── bm1684x/
    └── yolov8n_det_person_vehicle_384_640_INT8_bm1684x.bmodel
```

如果无法完整 clone 大仓库，可以只下载需要的 `.bmodel` 文件。

## 4. 编译 TDL_SDK

以下步骤在 x86 编译主机上执行。

为了便于整理，本文后续统一使用以下工作目录：

```bash
/data/tdl_build_bm1684x
```

目录结构建议如下：

```bash
/data/tdl_build_bm1684x/
├── SDK-26.03.01-bm1684x-minimal.zip
├── host-tools/
├── sdk_minimal/
├── tdl_models/
└── tdl_sdk/
```

### 4.0 Ubuntu 版本与 Docker 编译建议

如果你的 x86 编译主机是 Ubuntu 22.04，而 BM1684X 盒子系统是 Ubuntu 20.04，编译 TDL_SDK 时确实可能遇到和编译 sophon-stream 类似的环境差异问题，但两者的风险点略有不同。

TDL_SDK 的 BM1684X 编译流程本质上是交叉编译：先用 `extract_sophon_sdk.sh` 从 SOPHON SDK 包中抽取目标平台依赖，再用 `build_tdl_sdk.sh BM1684X` 生成可部署到 BM1684X 的产物。只要编译过程中使用的是 SDK 包提供的 aarch64 目标库和交叉工具链，主机 Ubuntu 22.04 通常可以作为编译环境使用。

但是仍需注意以下风险：

- 主机系统包版本过新，导致 CMake、Python、编译脚本行为和官方验证环境不完全一致。
- 编译时链接的目标端运行库版本与盒子 `/opt/sophon` 中实际预装版本不一致。
- 使用 `SDK-26.03.01` 编译，但盒子系统中预装的 libsophon、sophon-mw 版本较旧，运行时可能出现 `symbol not found`、`GLIBCXX_xxx not found` 或动态库加载错误。
- 如果在主机上全局安装过其他版本 Sophon SDK，可能被 CMake 或环境变量误引用。

因此，推荐使用 Docker 镜像固定编译环境。Docker 只用于 x86 主机侧编译，不需要在 BM1684X 盒子上运行 Docker。

#### 4.0.1 推荐 Dockerfile

在 x86 主机上新建目录：

```bash
mkdir -p ~/tdl_build_docker
cd ~/tdl_build_docker
```

创建 `Dockerfile`：

```dockerfile
FROM ubuntu:20.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates \
    git \
    cmake \
    ninja-build \
    build-essential \
    device-tree-compiler \
    libssl-dev \
    ssh \
    bison \
    flex \
    unzip \
    python3 \
    python3-pip \
    file \
    rsync \
    pkg-config \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace
```

构建镜像：

```bash
docker build -t tdl-sdk-bm1684x:ubuntu20.04 .
```

这里选择 Ubuntu 20.04，是为了让编译环境更接近 BM1684X 盒子的系统版本。虽然是交叉编译，但这能减少脚本工具、CMake、Python 和系统库差异带来的问题。

如果你希望验证 Ubuntu 22.04 主机环境是否也能编译，可以另做一个 `ubuntu:22.04` 镜像，但建议优先用 `ubuntu:20.04` 作为稳定基线。

#### 4.0.2 使用 Docker 编译

假设你的目录结构为：

```bash
/data/tdl_build_bm1684x/
├── SDK-26.03.01-bm1684x-minimal.zip
├── host-tools/
└── tdl_sdk/
```

进入容器：

```bash
docker run --rm -it \
  -v /data/tdl_build_bm1684x:/workspace/tdl_build_bm1684x \
  tdl-sdk-bm1684x:ubuntu20.04 \
  bash
```

容器内执行：

```bash
cd /workspace/tdl_build_bm1684x/tdl_sdk

rm -rf dependency/BM1684X build/BM1684X install/BM1684X

./scripts/extract_sophon_sdk.sh /workspace/tdl_build_bm1684x/SDK-26.03.01-bm1684x-minimal.zip BM1684X

./build_tdl_sdk.sh BM1684X
```

如果需要使用 BmCV 做图像预处理，可以在编译前增加：

```bash
export USE_BMCV=1
./build_tdl_sdk.sh BM1684X
```

编译结果仍会出现在宿主机的：

```bash
/data/tdl_build_bm1684x/tdl_sdk/install/BM1684X
```

#### 4.0.3 Docker 编译的注意事项

Docker 镜像只解决“编译主机环境可复现”的问题，不会自动解决“目标设备运行库版本不一致”的问题。运行时仍建议：

- 不要覆盖盒子的 `/opt/sophon`。
- 不要把 TDL_SDK 的库复制到 sophon-stream 的 `build/lib`。
- 使用 `/data/tdl_isolated/run_scripts` 中的独立脚本临时设置 `LD_LIBRARY_PATH`。
- 如果运行时报动态库符号错误，优先确认盒子上的 `/opt/sophon` 版本和 SDK-26.03.01 是否匹配。

如果你的盒子系统已经稳定运行 sophon-stream，不建议为了 TDL_SDK 直接升级或替换盒子系统 SDK。更稳妥的方式是先隔离部署 TDL_SDK，验证无误后再考虑统一 SDK 版本。

### 4.1 解压 SDK 依赖

进入 `tdl_sdk`：

```bash
cd /data/tdl_build_bm1684x/tdl_sdk
```

如果之前尝试过其他 SDK 版本，建议先清理 TDL_SDK 自己的编译和依赖输出：

```bash
rm -rf dependency/BM1684X build/BM1684X install/BM1684X
```

使用 SDK-26.03.01：

```bash
./scripts/extract_sophon_sdk.sh /data/tdl_build_bm1684x/SDK-26.03.01-bm1684x-minimal.zip BM1684X
```

如果使用官方示例版本 SDK-24.04.01，则命令类似：

```bash
./scripts/extract_sophon_sdk.sh /data/tdl_build_bm1684x/SDK-24.04.01.zip BM1684X
```

执行成功后，通常会在 `dependency/BM1684X` 下生成交叉编译依赖。

### 4.2 编译

```bash
./build_tdl_sdk.sh BM1684X
```

编译成功后，重点检查：

```bash
ls install/BM1684X
```

通常会包含：

```text
bin/
configs/
include/
lib/
scripts/
```

其中：

- `bin/c`：C API 示例程序。
- `bin/cpp`：C++ API 示例程序。
- `lib`：TDL_SDK 运行库。
- `include`：TDL_SDK 头文件。
- `configs`：模型配置、预处理、后处理、类型映射等配置。

### 4.3 x86 编译机一键整理和编译

如果 x86 编译机 IP 为 `192.168.205.136`，建议在该机器上统一使用：

```bash
/data/tdl_build_bm1684x
```

先登录 x86 编译机，创建目录：

```bash
mkdir -p /data/tdl_build_bm1684x/sdk_minimal
cd /data/tdl_build_bm1684x
```

确认已经上传两个文件：

```bash
ls -lh sdk_minimal
```

应该能看到：

```text
libsophon_soc_..._aarch64.tar.gz
sophon-mw-soc_..._aarch64.tar.gz
```

创建一键编译脚本：

```bash
cat > /data/tdl_build_bm1684x/build_tdl_bm1684x.sh <<'EOF'
#!/bin/bash
set -e

WORK_ROOT=/data/tdl_build_bm1684x
SDK_MINI_ZIP=${WORK_ROOT}/SDK-26.03.01-bm1684x-minimal.zip

mkdir -p ${WORK_ROOT}/sdk_minimal
cd ${WORK_ROOT}

echo "[1/6] Check minimal SDK files"
ls ${WORK_ROOT}/sdk_minimal/libsophon_soc_*_aarch64.tar.gz
ls ${WORK_ROOT}/sdk_minimal/sophon-mw-soc_*_aarch64.tar.gz

echo "[2/6] Create minimal SDK zip"
rm -f ${SDK_MINI_ZIP}
cd ${WORK_ROOT}/sdk_minimal
zip -r ${SDK_MINI_ZIP} \
  libsophon_soc_*_aarch64.tar.gz \
  sophon-mw-soc_*_aarch64.tar.gz

echo "[3/6] Prepare source repos"
cd ${WORK_ROOT}
if [ ! -d host-tools ]; then
  git clone https://github.com/sophgo/host-tools.git
fi

if [ ! -d tdl_sdk ]; then
  git clone https://github.com/sophgo/tdl_sdk.git
fi

echo "[4/6] Clean previous BM1684X build"
cd ${WORK_ROOT}/tdl_sdk
rm -rf dependency/BM1684X build/BM1684X install/BM1684X

echo "[5/6] Extract SOPHON SDK dependency"
./scripts/extract_sophon_sdk.sh ${SDK_MINI_ZIP} BM1684X

echo "[6/6] Build TDL_SDK for BM1684X"
./build_tdl_sdk.sh BM1684X

echo "Build done:"
ls -lh ${WORK_ROOT}/tdl_sdk/install/BM1684X
EOF

chmod +x /data/tdl_build_bm1684x/build_tdl_bm1684x.sh
```

执行：

```bash
/data/tdl_build_bm1684x/build_tdl_bm1684x.sh
```

如果希望使用 Docker 编译，可以把脚本中的实际编译部分放到容器中执行，或者直接使用 4.0.2 中的 Docker 命令。无论是否使用 Docker，输入和输出都集中在 `/data/tdl_build_bm1684x`。

如果 x86 编译机无法访问 GitHub，可以提前将 `host-tools` 和 `tdl_sdk` 目录上传到 `/data/tdl_build_bm1684x`，脚本检测到目录已存在后不会重新 clone。

#### 4.3.1 手动准备 host-tools

如果 x86 编译机通过 HTTPS 访问 GitHub 不稳定，但可以使用 SSH clone，可以手动将 `host-tools` 放到固定位置：

```bash
cd /data/tdl_build_bm1684x
rm -rf host-tools
git clone git@github.com:sophgo/host-tools.git host-tools
```

确认官方 BM1684X 交叉工具链存在：

```bash
ls -lh /data/tdl_build_bm1684x/host-tools/gcc/gcc-buildroot-9.3.0-aarch64-linux-gnu/bin/aarch64-linux-gcc

/data/tdl_build_bm1684x/host-tools/gcc/gcc-buildroot-9.3.0-aarch64-linux-gnu/bin/aarch64-linux-gcc -dumpmachine
```

正常情况下，`dumpmachine` 应输出：

```text
aarch64-buildroot-linux-gnu
```

如果执行交叉编译器时提示缺少 `libisl.so.22`，需要把官方工具链自带的 `lib` 加入 `LD_LIBRARY_PATH`：

```bash
export OFFICIAL_TOOLCHAIN=/data/tdl_build_bm1684x/host-tools/gcc/gcc-buildroot-9.3.0-aarch64-linux-gnu
export LD_LIBRARY_PATH=${OFFICIAL_TOOLCHAIN}/lib:${LD_LIBRARY_PATH}
```

可以用最小 C 文件测试工具链：

```bash
printf 'int main(){return 0;}\n' > /tmp/test_tdl_toolchain.c
${OFFICIAL_TOOLCHAIN}/bin/aarch64-linux-gcc -c /tmp/test_tdl_toolchain.c -o /tmp/test_tdl_toolchain.o
file /tmp/test_tdl_toolchain.o
```

期望输出中包含：

```text
ELF 64-bit LSB relocatable, ARM aarch64
```

#### 4.3.2 离线 third-party 依赖缓存

TDL_SDK 编译时会从 `sophgo/oss` 下载 third-party 包。如果 x86 编译机下载失败，可以在网络较好的机器上下载以下文件：

```text
curl.tar.gz
eigen.tar.gz
googletest.tar.gz
kaldi-native-fbank.tar.gz
kissfft.tar.gz
libwebsockets.tar.gz
nlohmannjson.tar.gz
openssl.tar.gz
stb.tar.gz
zlib.tar.gz
```

下载地址格式为：

```text
https://github.com/sophgo/oss/raw/refs/heads/master/oss_release_tarball/64bit/<package>.tar.gz
```

上传到 x86 编译机后放入：

```bash
/data/tdl_build_bm1684x/tdl_sdk/dependency/thirdparty/
```

同时确认 `scripts/download_thirdparty.sh` 中 `TARGET_DIR` 指向源码根目录下的 `dependency/thirdparty`：

```bash
TARGET_DIR="$(cd "$(dirname "$0")/.." && pwd)/dependency/thirdparty"
```

如果 third-party 包已存在，编译时会显示：

```text
eigen.tar.gz already exists, skipping
...
third-party packages download completed.
```

#### 4.3.3 推荐的本地构建脚本

如果已手动准备好：

- `/data/tdl_build_bm1684x/host-tools`
- `/data/tdl_build_bm1684x/tdl_sdk`
- `/data/tdl_build_bm1684x/sdk_minimal/libsophon_soc_*_aarch64.tar.gz`
- `/data/tdl_build_bm1684x/sdk_minimal/sophon-mw-soc_*_aarch64.tar.gz`

可以使用下面的构建脚本：

```bash
cat > /data/tdl_build_bm1684x/build_tdl_bm1684x_local.sh <<'EOF'
#!/bin/bash
set -euo pipefail

WORK_ROOT=/data/tdl_build_bm1684x
SDK_MINI_ZIP=${WORK_ROOT}/SDK-26.03.01-bm1684x-soc-minimal.zip
cd ${WORK_ROOT}

echo "[1/5] Check minimal SDK files"
ls -lh ${WORK_ROOT}/sdk_minimal/libsophon_soc_*_aarch64.tar.gz
ls -lh ${WORK_ROOT}/sdk_minimal/sophon-mw-soc_*_aarch64.tar.gz

echo "[2/5] Create minimal SDK zip"
rm -f ${SDK_MINI_ZIP}
cd ${WORK_ROOT}/sdk_minimal
zip -r ${SDK_MINI_ZIP} \
  libsophon_soc_*_aarch64.tar.gz \
  sophon-mw-soc_*_aarch64.tar.gz

echo "[3/5] Clean previous BM1684X build"
cd ${WORK_ROOT}/tdl_sdk
rm -rf dependency/BM1684X build/BM1684X install/BM1684X
chmod +x scripts/*.sh build_tdl_sdk.sh || true

echo "[4/5] Extract SOPHON SDK dependency"
./scripts/extract_sophon_sdk.sh ${SDK_MINI_ZIP} BM1684X

echo "[5/5] Build TDL_SDK for BM1684X"
export OFFICIAL_TOOLCHAIN=${WORK_ROOT}/host-tools/gcc/gcc-buildroot-9.3.0-aarch64-linux-gnu
export LD_LIBRARY_PATH=${OFFICIAL_TOOLCHAIN}/lib:${LD_LIBRARY_PATH:-}
export BM1684X_LIBS=${WORK_ROOT}/tdl_sdk/dependency/BM1684X
export LIBRARY_PATH=${BM1684X_LIBS}/libsophon/lib:${BM1684X_LIBS}/sophon-opencv/lib:${BM1684X_LIBS}/sophon-ffmpeg/lib:${LIBRARY_PATH:-}
export LDFLAGS="-Wl,-rpath-link,${BM1684X_LIBS}/libsophon/lib -Wl,-rpath-link,${BM1684X_LIBS}/sophon-opencv/lib -Wl,-rpath-link,${BM1684X_LIBS}/sophon-ffmpeg/lib ${LDFLAGS:-}"

./build_tdl_sdk.sh BM1684X

echo "Build done:"
find ${WORK_ROOT}/tdl_sdk/install/BM1684X -maxdepth 2 -type f | head -n 50
EOF

chmod +x /data/tdl_build_bm1684x/build_tdl_bm1684x_local.sh
/data/tdl_build_bm1684x/build_tdl_bm1684x_local.sh
```

### 4.4 远程自动编译前的 SSH 准备

如果需要从本机远程控制 x86 编译机自动编译，推荐使用 SSH key，而不是把 root 密码写入脚本。

在本机生成 SSH key：

```bash
ssh-keygen -t ed25519 -C "tdl-build" -f ~/.ssh/tdl_build_ed25519
```

将公钥复制到 x86 编译机：

```bash
ssh-copy-id -i ~/.ssh/tdl_build_ed25519.pub root@192.168.205.136
```

之后可以非交互执行：

```bash
ssh -i ~/.ssh/tdl_build_ed25519 root@192.168.205.136 \
  "mkdir -p /data/tdl_build_bm1684x/sdk_minimal && ls -lh /data/tdl_build_bm1684x"
```

上传 minimal SDK 文件：

```bash
scp -i ~/.ssh/tdl_build_ed25519 \
  libsophon_soc_*_aarch64.tar.gz \
  root@192.168.205.136:/data/tdl_build_bm1684x/sdk_minimal/

scp -i ~/.ssh/tdl_build_ed25519 \
  sophon-mw-soc_*_aarch64.tar.gz \
  root@192.168.205.136:/data/tdl_build_bm1684x/sdk_minimal/
```

远程执行编译：

```bash
ssh -i ~/.ssh/tdl_build_ed25519 root@192.168.205.136 \
  "/data/tdl_build_bm1684x/build_tdl_bm1684x.sh"
```

如果暂时不配置 SSH key，也可以手动登录 x86 编译机执行 4.3 中的一键编译脚本。不要把密码直接写入 markdown、shell 脚本或 git 仓库文件。

## 5. 部署到 BM1684X 设备

以下假设 BM1684X 设备 IP 为 `192.168.1.100`，请按实际情况替换。

### 5.1 创建隔离目录

在设备上执行：

```bash
mkdir -p /data/tdl_isolated/tdl_sdk
mkdir -p /data/tdl_isolated/tdl_models
mkdir -p /data/tdl_isolated/test_images
mkdir -p /data/tdl_isolated/run_scripts
```

### 5.2 拷贝 TDL_SDK

在 x86 编译主机执行：

```bash
scp -r /data/tdl_build_bm1684x/tdl_sdk/install/BM1684X/* \
  root@192.168.1.100:/data/tdl_isolated/tdl_sdk/
```

### 5.3 拷贝模型

```bash
scp -r /data/tdl_build_bm1684x/tdl_models/bm1684x \
  root@192.168.1.100:/data/tdl_isolated/tdl_models/
```

或者只拷贝单个模型：

```bash
scp /data/tdl_build_bm1684x/tdl_models/bm1684x/yolov8n_det_person_vehicle_384_640_INT8_bm1684x.bmodel \
  root@192.168.1.100:/data/tdl_isolated/tdl_models/bm1684x/
```

### 5.4 拷贝测试图片

```bash
scp test.jpg root@192.168.1.100:/data/tdl_isolated/test_images/
```

## 6. 运行前检查

在 BM1684X 设备上执行。

### 6.1 检查 TPU 设备

```bash
bm-smi
```

如果 `bm-smi` 不存在，说明设备基础 SDK 或系统环境可能未正确安装。

### 6.2 检查 TDL_SDK 示例程序

```bash
cd /data/tdl_isolated/tdl_sdk
find bin -maxdepth 3 -type f | sort
```

重点查找类似程序：

```text
bin/c/sample_object_detection
bin/cpp/sample_object_detection
```

不同版本 TDL_SDK 的示例程序名称可能略有差异，如果没有完全相同名称，以实际 `bin/c`、`bin/cpp` 目录中的文件为准。

### 6.3 检查动态库依赖

```bash
cd /data/tdl_isolated/tdl_sdk
ldd bin/c/sample_object_detection | grep "not found" || true
```

如果存在 `not found`，不要全局修改系统库路径，优先使用下一节的隔离运行脚本。

## 7. 隔离运行脚本

在 BM1684X 设备上创建：

```bash
vi /data/tdl_isolated/run_scripts/run_tdl_object_detection.sh
```

写入：

```bash
#!/bin/bash
set -e

TDL_ROOT=/data/tdl_isolated/tdl_sdk
MODEL_ROOT=/data/tdl_isolated/tdl_models/bm1684x
IMAGE_ROOT=/data/tdl_isolated/test_images

export LD_LIBRARY_PATH=${TDL_ROOT}/lib:${TDL_ROOT}/sample/tpu/lib:${TDL_ROOT}/sample/middleware/lib:${TDL_ROOT}/sample/utils/lib:${LD_LIBRARY_PATH}

cd ${TDL_ROOT}/bin/c

./sample_object_detection \
  -m ${MODEL_ROOT}/yolov8n_det_person_vehicle_384_640_INT8_bm1684x.bmodel \
  -i ${IMAGE_ROOT}/test.jpg \
  -o ${IMAGE_ROOT}/test_result.jpg
```

授权并运行：

```bash
chmod +x /data/tdl_isolated/run_scripts/run_tdl_object_detection.sh
/data/tdl_isolated/run_scripts/run_tdl_object_detection.sh
```

运行后检查输出：

```bash
ls -lh /data/tdl_isolated/test_images/test_result.jpg
```

如果示例程序参数不是 `-m -i -o`，先查看帮助：

```bash
cd /data/tdl_isolated/tdl_sdk/bin/c
./sample_object_detection --help
```

或者：

```bash
./sample_object_detection -h
```

再按实际帮助信息调整脚本。

## 8. TDL_Detection 接口的使用方式

`TDL_Detection` 是 TDL_SDK 的 C API 检测接口。它不是 `sophon-stream` 中的接口。

典型调用流程如下：

```c
TDLHandle handle = NULL;
TDLImage image = NULL;
TDLObjectMeta object_meta = {0};

TDL_CreateHandle(&handle, 0);

TDL_OpenModel(
    handle,
    TDL_MODEL_YOLOV8N_DET_PERSON_VEHICLE,
    "/data/tdl_isolated/tdl_models/bm1684x/yolov8n_det_person_vehicle_384_640_INT8_bm1684x.bmodel");

TDL_ReadImage(
    "/data/tdl_isolated/test_images/test.jpg",
    &image,
    PIXEL_FORMAT_RGB_888);

TDL_Detection(
    handle,
    TDL_MODEL_YOLOV8N_DET_PERSON_VEHICLE,
    image,
    &object_meta);

TDL_ReleaseObjectMeta(&object_meta);
TDL_DestroyImage(image);
TDL_CloseModel(handle, TDL_MODEL_YOLOV8N_DET_PERSON_VEHICLE);
TDL_DestroyHandle(handle);
```

实际枚举名称、图像格式名称可能随 TDL_SDK 版本变化，请以当前 SDK 的头文件为准，重点查看：

```bash
include/c_apis/tdl_sdk.h
include/nn/tdl_model_list.h
configs/model/model_factory.json
```

## 9. 如何确认模型应该使用哪个接口

优先查看 TDL_SDK 的模型工厂配置：

```bash
cd /data/tdl_isolated/tdl_sdk
grep -R "YOLOV8N_DET_PERSON_VEHICLE" -n configs include
grep -R "MBV2_DET_PERSON" -n configs include
```

通常需要确认三类信息。

### 9.1 模型 ID

例如：

```text
TDL_MODEL_YOLOV8N_DET_PERSON_VEHICLE
TDL_MODEL_MBV2_DET_PERSON
```

这个 ID 决定 `TDL_OpenModel` 和 `TDL_Detection` 调用哪个模型逻辑。

### 9.2 模型文件名

例如：

```text
yolov8n_det_person_vehicle_384_640_INT8_bm1684x.bmodel
mbv2_det_person_256_448_INT8_bm1684x.bmodel
```

模型文件名通常能看出：

- 算法类型，例如 `yolov8n_det`、`mbv2_det`。
- 任务类型，例如 `person_vehicle`、`person`。
- 输入尺寸，例如 `384_640`、`256_448`。
- 精度类型，例如 `INT8`。
- 芯片平台，例如 `bm1684x`。

### 9.3 类别列表和后处理

例如 `YOLOV8N_DET_PERSON_VEHICLE` 可能包含：

```text
car
bus
truck
rider with motorcycle
person
bike
motorcycle
```

类别、阈值、NMS、输入颜色顺序、resize 策略等通常由 TDL_SDK 的配置和对应算法代码决定，不建议只凭 `.bmodel` 文件名手写后处理。

## 10. 与 sophon-stream 共存的注意事项

### 10.1 环境变量隔离

运行 TDL_SDK 时只在脚本中临时设置：

```bash
export LD_LIBRARY_PATH=/data/tdl_isolated/tdl_sdk/lib:$LD_LIBRARY_PATH
```

不要执行：

```bash
echo "export LD_LIBRARY_PATH=..." >> ~/.bashrc
```

也不要在启动 sophon-stream 的 shell 中长期 source TDL_SDK 的环境脚本。

### 10.2 库文件隔离

不要把 TDL_SDK 的 `.so` 文件拷贝到 sophon-stream 的构建目录：

```bash
/data/sophon-stream/build/lib
```

sophon-stream 运行时需要的库路径仍按原方式设置，例如：

```bash
export LD_LIBRARY_PATH=/data/sophon-stream/build/lib:$LD_LIBRARY_PATH
```

TDL_SDK 运行时使用自己的脚本：

```bash
/data/tdl_isolated/run_scripts/run_tdl_object_detection.sh
```

这样两个程序的动态库搜索路径不会长期互相污染。

### 10.3 设备资源隔离

环境隔离只能避免库冲突，不能避免 TPU 算力和内存竞争。如果 TDL_SDK 和 sophon-stream 同时运行，仍可能出现：

- TPU 显存不足。
- 设备利用率过高。
- 推理延迟抖动。

建议先单独运行 TDL_SDK 验证模型，再单独运行 sophon-stream。如果确实需要同时运行，使用：

```bash
bm-smi
top
free -h
```

观察资源占用。

## 11. 常见问题

### 11.1 `sample_object_detection: not found`

先确认示例程序真实名称：

```bash
find /data/tdl_isolated/tdl_sdk/bin -maxdepth 3 -type f | sort
```

不同版本 TDL_SDK 的 sample 名称可能不同。找到对应检测示例后，修改运行脚本中的可执行文件名。

### 11.2 `error while loading shared libraries`

使用 `ldd` 查看缺失库：

```bash
ldd /data/tdl_isolated/tdl_sdk/bin/c/sample_object_detection | grep "not found"
```

然后把缺失库所在目录追加到运行脚本的 `LD_LIBRARY_PATH`，不要直接改系统全局路径。

### 11.3 SDK-26.03.01 编译失败

先确认 SDK 包结构：

```bash
unzip -l SDK-26.03.01.zip | grep -E "libsophon_.*aarch64|sophon-mw-soc_.*aarch64"
```

如果文件名存在但脚本识别失败，可能是 TDL_SDK 的 `extract_sophon_sdk.sh` 对文件名匹配较严格。建议优先检查脚本中对 `libsophon` 和 `sophon-mw-soc` 的匹配规则，而不是修改系统环境。

如果编译通过但设备运行时报符号错误，通常是编译 SDK 与设备系统预装 SDK 版本不一致。解决思路有两个：

- 使用与设备系统一致的 SDK 版本重新编译 TDL_SDK。
- 将 SDK-26.03.01 对应运行库隔离部署，并只在 TDL_SDK 运行脚本中临时加载。

### 11.4 模型能在 TDL_SDK 跑，但不能在 yolo8-test 跑

这是正常现象。`yolo8-test` 复用了 sophon-stream 的 YOLOv8 插件后处理逻辑，它只支持该插件认可的 YOLOv8 输出格式。`tdl_models` 中部分模型输出的是原始 head 特征图，必须使用 TDL_SDK 对应的后处理。

如果后续要接入 sophon-stream，有两条路线：

1. 将模型重新导出或转换为 sophon-stream `yolov8` 插件支持的输出格式。
2. 新建 sophon-stream 插件，在插件中复用或移植 TDL_SDK 的前处理、推理、后处理逻辑。

## 12. 推荐验证顺序

建议按以下顺序推进：

1. 在 x86 主机上使用 `SDK-26.03.01` 编译 TDL_SDK。
2. 将 `install/BM1684X` 独立部署到 `/data/tdl_isolated/tdl_sdk`。
3. 将 BM1684X 模型独立部署到 `/data/tdl_isolated/tdl_models/bm1684x`。
4. 用 `sample_object_detection` 跑通一张图片。
5. 确认输出图片、检测框、类别和置信度正确。
6. 再决定是否把 TDL_SDK 模型逻辑封装进 sophon-stream 新插件。

## 13. 参考入口

- TDL_SDK 仓库：`https://github.com/sophgo/tdl_sdk`
- TDL Models 仓库：`https://github.com/sophgo/tdl_models`
- sophon-stream 环境文档：`docs/EnvironmentInstallGuide.md`
- sophon-stream 编译文档：`docs/HowToMake.md`
