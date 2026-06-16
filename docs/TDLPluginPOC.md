# sophon-stream 自定义 TDL 插件 POC 记录

本文记录本次在 sophon-stream 中新增 `tdl` 自定义插件、调用 TDL SDK 模型、交叉编译并在 BM1684X SoC 盒子上验证的主要内容。

## 目标

验证 sophon-stream 可以通过自定义 Element 插件接入 TDL SDK，复用 sophon-stream 的解码、图像流转、结果回调和结果图片保存能力，同时由 TDL SDK 完成模型加载、预处理、TPU 推理和后处理。

## 新增和修改内容

### 新增插件

- `element/algorithm/tdl/include/tdl.h`
- `element/algorithm/tdl/src/tdl.cc`
- `element/algorithm/tdl/CMakeLists.txt`

插件名称为 `tdl`，编译产物为：

```text
build/lib/libtdl.so
```

当前 POC 支持的配置项：

```json
{
  "model_id": "YOLOV8N_DET_PERSON_VEHICLE",
  "task_type": "Detection",
  "model_path": "/data/tdl_models/bm1684x/yolov8n_det_person_vehicle_384_640_INT8_bm1684x.bmodel",
  "threshold": 0.5,
  "temp_dir": "/data/sophon-stream/tmp/tdl",
  "class_names": ["car", "bus", "truck", "rider with motorcycle", "person", "bike", "motorcycle"]
}
```

插件主流程：

1. `initInternal` 解析配置，创建 `TDLHandle`。
2. 根据 `model_id` 将字符串映射为 `TDLModel` 枚举。
3. 调用 `TDL_OpenModel` 加载 bmodel。
4. 从 sophon-stream 的 `ObjectMetadata` 中取得解码后的 `bm_image`。
5. POC 阶段先将 `bm_image` 临时编码为 JPEG，再通过 `TDL_ReadImage` 交给 TDL SDK。
6. `Detection` 任务调用 `TDL_Detection`，结果转换到 `mDetectedObjectMetadatas`。
7. `Classification` 任务已预留基础调用路径，结果转换到 `mRecognizedObjectMetadatas`。

注意：当前 POC 为了快速验证链路，采用 “`bm_image` -> 临时 JPEG -> `TDL_ReadImage`” 的输入适配方式。生产版本建议进一步改成 `TDL_WrapFrame` 或 TDL SDK 支持的零拷贝帧封装方式。

### 新增 sample

- `samples/tdl/config/decode.json`
- `samples/tdl/config/tdl.json`
- `samples/tdl/config/engine.json`
- `samples/tdl/config/tdl_demo.json`
- `samples/tdl/data/person_vehicle.names`
- `samples/tdl/README.md`

sample 拓扑：

```text
decode -> tdl -> main sink
```

其中 `decode` 继续复用 sophon-stream 原有解码插件，`tdl` 只负责调用 TDL SDK 做算法推理，`main sink` 复用 sample 框架中的结果绘制与保存逻辑。

### CMake 修改

根目录 `CMakeLists.txt` 增加：

```cmake
checkAndAddElement(element/algorithm/tdl)
```

为适配 BM1684X SoC 盒子的 Ubuntu 20.04/glibc 环境，以下 CMake 文件的 SoC 编译器前缀增加了 `SOPHON_CROSS_PREFIX` 参数，默认仍保持原来的 `aarch64-linux-gnu-`：

- `framework/CMakeLists.txt`
- `element/algorithm/tdl/CMakeLists.txt`
- `element/multimedia/decode/CMakeLists.txt`
- `element/multimedia/osd/CMakeLists.txt`
- `samples/CMakeLists.txt`
- `3rdparty/freetype2/CMakeLists.txt`

`samples/CMakeLists.txt` 还补充了 SoC 链接参数：

```cmake
-Wl,-rpath-link,${SOPHON_SDK_SOC}/lib -Wl,--allow-shlib-undefined
```

这样交叉链接 `samples/build/main` 时可以解析 SoC SDK 内部的间接动态库依赖。

## 编译流程

x86 编译机：

```text
IP: 192.168.205.136
sophon-stream: /root/projects/sophon-stream
SoC SDK: /root/soc-sdk
TDL SDK: /data/tdl_build_bm1684x/tdl_sdk/install/BM1684X
工具链: /data/tdl_build_bm1684x/host-tools/gcc/gcc-buildroot-9.3.0-aarch64-linux-gnu
```

编译命令：

```bash
cd /root/projects/sophon-stream

TOOL=/data/tdl_build_bm1684x/host-tools/gcc/gcc-buildroot-9.3.0-aarch64-linux-gnu
export PATH=$TOOL/bin:$PATH
export LD_LIBRARY_PATH=$TOOL/lib:${LD_LIBRARY_PATH:-}

rm -rf build samples/build
mkdir -p build
cd build

cmake .. \
  -DTARGET_ARCH=soc \
  -DSOPHON_SDK_SOC=/root/soc-sdk \
  -DTDL_SDK_PATH=/data/tdl_build_bm1684x/tdl_sdk/install/BM1684X \
  -DSOPHON_CROSS_PREFIX=$TOOL/bin/aarch64-buildroot-linux-gnu- \
  -DCMAKE_C_COMPILER=$TOOL/bin/aarch64-buildroot-linux-gnu-gcc \
  -DCMAKE_CXX_COMPILER=$TOOL/bin/aarch64-buildroot-linux-gnu-g++

make -j$(nproc) tdl decode main
```

说明：本次 POC 只需要 `tdl`、`decode`、`main` 三个目标。完整 `make all` 会继续编译 `encode` 等无关插件，其中 `encode` 依赖 Boost 头文件，当前 buildroot sysroot 下未补齐该依赖，因此 POC 编译采用目标编译。

编译后关键产物：

```text
build/lib/libtdl.so
build/lib/libdecode.so
build/lib/libframework.so
build/lib/libivslogger.so
build/lib/libcvunitext.so
samples/build/main
```

产物版本检查结果：

```text
GLIBC max: GLIBC_2.17
GLIBCXX max: GLIBCXX_3.4.26
```

该版本可以在 BM1684X SoC 盒子的 Ubuntu 20.04 环境运行，避免了 Ubuntu 22.04 gcc 11 交叉编译导致的 `GLIBC_2.34` / `GLIBCXX_3.4.30` 运行时报错。

## 盒子运行流程

边缘盒子：

```text
IP: 10.56.30.33
部署目录: /data/sophon-stream
TDL SDK: /data/tdl_isolated/tdl_sdk
模型目录: /data/tdl_models/bm1684x
测试图片目录: /data/tdl_isolated/images
```

运行命令：

```bash
cd /data/sophon-stream

export TDL_ROOT=/data/tdl_isolated/tdl_sdk
export LD_LIBRARY_PATH=/data/sophon-stream/build/lib:\
$TDL_ROOT/lib:\
$TDL_ROOT/sample/utils/lib:\
$TDL_ROOT/sample/tpu/lib:\
$TDL_ROOT/sample/3rd/libwebsockets/lib:\
/opt/sophon/libsophon-0.5.1/lib:\
/opt/sophon/sophon-ffmpeg_0.15.0/lib:\
/opt/sophon/sophon-opencv_0.15.0/lib:\
${LD_LIBRARY_PATH:-}

mkdir -p /data/sophon-stream/tmp/tdl
cd samples/build
./main --demo_config_path=../tdl/config/tdl_demo.json
```

结果图片输出目录：

```text
/data/sophon-stream/samples/build/results
```

本次测试结果：

```text
RUN_STATUS=0
tdl model opened: id=YOLOV8N_DET_PERSON_VEHICLE, input=640x384
tdl detection channel=0, frame=0, objects=5
tdl detection channel=0, frame=1, objects=5
frame count is 3
results/0_0_0.jpg
results/0_0_1.jpg
```

## 当前 POC 限制

1. 当前输入适配使用临时 JPEG 文件，功能验证通过，但不是最高性能方案。
2. 检测任务已完成端到端验证；分类任务保留了基础接口路径，但还需要针对具体分类模型补 sample 和绘制/输出逻辑。
3. 插件析构阶段暂时跳过显式 `TDL_CloseModel/TDL_DestroyHandle`，因为当前 SDK/运行环境在图停止后显式释放会触发 `BMRuntime internal error` 并导致进程 abort。POC 先保证运行链路和进程退出稳定，生产版本应结合目标 TDL SDK 版本重新确认释放时机。
4. 一个插件可以覆盖多种 TDL 模型的前提是：模型能够通过 TDL SDK 的统一 C API 调用，并且结果结构能够映射到 sophon-stream 的 metadata。不同任务类型仍需要分别补齐结果转换逻辑。
