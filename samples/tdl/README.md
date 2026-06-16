# TDL 插件 POC 示例

该示例用于验证 sophon-stream 可以通过自定义 `tdl` 插件调用 TDL SDK 模型。

当前 POC 拓扑：

```text
decode -> tdl -> main sink
```

`tdl` 插件会从 `ObjectMetadata::mFrame->mSpData` 取得解码后的图像，临时编码成 JPEG，通过 `TDL_ReadImage` 调用 TDL SDK，并将检测结果写回 `mDetectedObjectMetadatas`。该方式优先验证功能链路，后续性能优化应替换为 `TDL_WrapFrame` 或其他零拷贝输入适配。

## 配置文件

- `config/tdl_demo.json`：sample 总配置。
- `config/engine.json`：graph 拓扑。
- `config/decode.json`：decode 插件配置。
- `config/tdl.json`：TDL 插件配置。
- `data/person_vehicle.names`：人车检测类别名。

默认模型：

```text
/data/tdl_models/bm1684x/yolov8n_det_person_vehicle_384_640_INT8_bm1684x.bmodel
```

默认测试图片目录：

```text
/data/tdl_isolated/images
```

## 运行

在 SoC 设备上的 sophon-stream 根目录中执行：

```bash
export TDL_ROOT=/data/tdl_isolated/tdl_sdk
export LD_LIBRARY_PATH=$PWD/build/lib:$TDL_ROOT/lib:$TDL_ROOT/sample/utils/lib:$TDL_ROOT/sample/tpu/lib:$TDL_ROOT/sample/3rd/libwebsockets/lib:$LD_LIBRARY_PATH

cd samples/build
./main --demo_config_path=../tdl/config/tdl_demo.json
```

如果 `download_image` 为 `true`，结果会保存到：

```text
samples/build/results
```

本 POC 默认将 TDL 临时 JPEG 文件写入 `/data/sophon-stream/tmp/tdl`，避免 SoC 设备 `/tmp` 空间不足导致运行失败。
