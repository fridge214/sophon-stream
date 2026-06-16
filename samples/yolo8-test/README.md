# yolo8-test Demo

本 demo 用于验证 `yolo8-test` 新插件是否能够在 sophon-stream 中完成动态库加载、ElementFactory 注册、Graph 初始化和流水线运行。

`yolo8-test` 复用了 `yolov8` 插件的算法实现，因此也可以通过配置文件切换 YOLOv8 的不同任务类型。当前目录提供了 Detect、Cls、Pose、Seg、Obb 五类配置模板。

## 目录结构

```text
samples/yolo8-test/
├── config/
│   ├── decode.json
│   ├── engine_group.json
│   ├── yolo8-test_demo.json
│   ├── yolo8-test_group.json
│   ├── engine_cls_group.json
│   ├── yolo8-test_cls_demo.json
│   ├── yolo8-test_cls_group.json
│   ├── engine_pose_group.json
│   ├── yolo8-test_pose_demo.json
│   ├── yolo8-test_pose_group.json
│   ├── engine_seg_group.json
│   ├── yolo8-test_seg_demo.json
│   ├── yolo8-test_seg_group.json
│   ├── engine_obb_group.json
│   ├── yolo8-test_obb_demo.json
│   ├── yolo8-test_obb_group.json
│   └── yolo8-test_classthresh_roi_example.json
├── data/
│   ├── coco.names
│   └── dotav1.names
└── scripts/
    └── download.sh
```

## 插件加载关系

所有 `yolo8-test_*_group.json` 都使用新插件动态库：

```json
"shared_object": "../../build/lib/libyolo8-test.so",
"name": "yolo8-test_group"
```

运行日志中如果出现下面内容，说明当前使用的是 `yolo8-test` 插件，而不是原始 `yolov8` 插件：

```text
"shared_object":"../../build/lib/libyolo8-test.so"
"name":"yolo8-test_group"
[fps_yolo8-test_pre]
[fps_yolo8-test_infer]
[fps_yolo8-test_post]
```

## Detect 目标检测

默认配置，复用 `samples/yolov8/data` 中的 YOLOv8 检测模型和视频：

```json
"model_path": "../yolov8/data/models/BM1684X/yolov8s_int8_1b.bmodel"
```

运行：

```bash
./main --demo_config_path=../yolo8-test/config/yolo8-test_demo.json
```

## Cls 图像分类

分类配置使用 `task_type = "Cls"`，默认输入为 `samples/yolov8/data/pics` 图片目录：

```json
"model_path": "../yolov8/data/models/BM1684X/yolov8n_cls_fp32_1b.bmodel",
"task_type": "Cls"
```

分类任务默认不保存图片，分类结果从日志中观察：

```bash
./main --demo_config_path=../yolo8-test/config/yolo8-test_cls_demo.json
```

## Pose 姿态检测

姿态检测配置使用 `task_type = "Pose"`，可视化函数为 `draw_yolov8_det_pose`：

```json
"model_path": "../yolov8/data/models/BM1684X/yolov8n_pose_int8_1b.bmodel",
"task_type": "Pose"
```

运行：

```bash
./main --demo_config_path=../yolo8-test/config/yolo8-test_pose_demo.json
```

结果图片会保存到 `samples/build/results`。

## Seg 实例分割

实例分割配置使用 `task_type = "Seg"`，可视化函数为 `draw_yolov8_seg`：

```json
"model_path": "../yolov8/data/models/BM1684X/yolov8s_seg_fp32_1b.bmodel",
"task_type": "Seg"
```

运行：

```bash
./main --demo_config_path=../yolo8-test/config/yolo8-test_seg_demo.json
```

结果图片会保存到 `samples/build/results`。

## Obb 旋转框检测

旋转框检测配置使用 `task_type = "Obb"`，可视化函数为 `draw_yolov8_obb_results`。

注意：OBB 模型和测试视频来自项目已有的 `samples/yolov8_obb/data`，不是普通 `samples/yolov8/data` 数据包的一部分。如果设备上没有该目录，需要先准备 `yolov8_obb` 示例的数据和模型。

```json
"model_path": "../yolov8_obb/data/models/BM1684X/yolov8s-obb_fp16_1b.bmodel",
"task_type": "Obb"
```

运行：

```bash
./main --demo_config_path=../yolo8-test/config/yolo8-test_obb_demo.json
```

结果图片会保存到 `samples/build/results`。

## 配置文件关系

每个任务都由三类文件组成：

```text
*_demo.json   # demo 输入配置，管理输入源、是否保存图片、绘制函数、engine 配置路径
engine_*.json # graph 配置，连接 decode 和 yolo8-test_group
*_group.json  # yolo8-test 插件配置，管理 model_path、task_type、阈值、预处理参数
```

如果要换成自己的模型，通常只需要修改对应 `*_group.json` 中的：

```json
"model_path": "...",
"task_type": "...",
"threshold_conf": 0.5,
"threshold_nms": 0.5,
"mean": [0, 0, 0],
"std": [255, 255, 255],
"bgr2rgb": true
```

同时根据任务类型修改 `*_demo.json` 中的 `draw_func_name`：

| 任务 | task_type | draw_func_name |
|---|---|---|
| 目标检测 | Detect | draw_yolov8_results |
| 图像分类 | Cls | default |
| 姿态检测 | Pose | draw_yolov8_det_pose |
| 实例分割 | Seg | draw_yolov8_seg |
| 旋转框检测 | Obb | draw_yolov8_obb_results |

如果开启 `download_image = true`，请确认 `class_names` 指向的类别文件存在。
