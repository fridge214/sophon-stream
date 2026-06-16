# yolo8-test Demo

This demo validates that the new `yolo8-test` plugin can be loaded, registered, initialized in a graph, and executed by sophon-stream.

The plugin reuses the YOLOv8 implementation, so this sample also provides configuration templates for Detect, Cls, Pose, Seg, and Obb tasks.

## Plugin Loading

All `yolo8-test_*_group.json` files load the new plugin library:

```json
"shared_object": "../../build/lib/libyolo8-test.so",
"name": "yolo8-test_group"
```

Runtime logs such as `[fps_yolo8-test_pre]`, `[fps_yolo8-test_infer]`, and `[fps_yolo8-test_post]` confirm that the `yolo8-test` plugin is being used.

## Run

Run from `samples/build` after building:

```bash
./main --demo_config_path=../yolo8-test/config/yolo8-test_demo.json
```

Other task templates:

```bash
./main --demo_config_path=../yolo8-test/config/yolo8-test_cls_demo.json
./main --demo_config_path=../yolo8-test/config/yolo8-test_pose_demo.json
./main --demo_config_path=../yolo8-test/config/yolo8-test_seg_demo.json
./main --demo_config_path=../yolo8-test/config/yolo8-test_obb_demo.json
```

Cls, Pose, and Seg reuse assets under `samples/yolov8/data`. Obb reuses assets under `samples/yolov8_obb/data`.
