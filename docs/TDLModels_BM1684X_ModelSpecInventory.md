# TDL Models BM1684X 模型规格排查清单

本文档基于本地目录 `C:\project\tdl_models\bm1684x` 中的 64 个 `.bmodel` 文件、`C:\project\tdl_models\README.md`，以及 TDL SDK 官方 `model_factory.json` / `tdl_model_list.h` 进行整理。

注意：当前 Windows 本机没有 `bmrt_test` 或等价 bmodel 查看工具，因此表中的输入规格主要来自文件名和 README。真正的 tensor 名称、输入输出 shape、dtype、scale、mean、颜色顺序，应在 BM1684X 设备上通过文末验证方法确认。

已在 BM1684X 边缘盒子上完成 `tpu_model --info` 和 `bmrt_test --loopnum 1` 批量验证，完整实测 I/O 清单见 [TDLModels_BM1684X_BModelIOInventory.md](TDLModels_BM1684X_BModelIOInventory.md)，原始日志保存在 [tdl_model_logs](tdl_model_logs/)。

## 总体结论

1. 这些模型的输入规格不相同，不能用一套固定前处理参数覆盖所有模型。
2. 可以做一个通用 `tdl` 插件，但插件内部必须按 `task_type` / `model_id` 分发到不同 TDL C API。
3. `decode`、`osd`、`encode` 等 sophon-stream 现有模块可以继续复用；新增插件主要负责把 `ObjectMetadata::mFrame->mSpData` 转成 TDL 输入，再把 TDL 输出写回 sophon-stream metadata。
4. 第一阶段建议只接入 full-frame image 类型模型，例如检测、人脸检测、语义分割、实例分割、车道线。音频、文本、向量、跟踪、依赖裁剪的属性/识别任务放到第二阶段。

## 模型规格清单

| 模型文件 | TDL C API | 模型 ID / 备注 | 输入规格 | 类别 / 输出含义 | 输入形态 |
|---|---|---|---|---|---|
| cls_attribute_gender_age_glass_112_112_INT8_bm1684x.bmodel | TDL_FaceAttribute | CLS_ATTRIBUTE_GENDER_AGE_GLASS | 112x112 | age, gender, glass | face crop or face_meta |
| cls_attribute_gender_age_glass_emotion_112_112_INT8_bm1684x.bmodel | TDL_FaceAttribute | CLS_ATTRIBUTE_GENDER_AGE_GLASS_EMOTION | 112x112 | age, gender, glass, emotion | face crop or face_meta |
| cls_attribute_gender_age_glass_emotion_tiny_112_112_INT8_bm1684x.bmodel | TDL_FaceAttribute | CLS_ATTRIBUTE_GENDER_AGE_GLASS_EMOTION variant | 112x112 | age, gender, glass, emotion | face crop or face_meta |
| cls_attribute_gender_age_glass_mask_112_112_INT8_bm1684x.bmodel | TDL_FaceAttribute | CLS_ATTRIBUTE_GENDER_AGE_GLASS_MASK | 112x112 | age, gender, glass, mask | face crop or face_meta |
| cls_hand_gesture_128_128_INT8_bm1684x.bmodel | TDL_Classification | CLS_HAND_GESTURE | 128x128 | fist, five, none, two | crop/full image by model |
| cls_keypoint_hand_gesture_1_42_INT8_bm1684x.bmodel | TDL_Classification | CLS_KEYPOINT_HAND_GESTURE | 1x42 vector | fist, five, four, none, ok, one, three, three2, two | keypoint vector |
| cls_rgbliveness_256_256_INT8_bm1684x.bmodel | TDL_Classification | CLS_RGBLIVENESS | 256x256 | live, spoof | crop/full image by model |
| cls_sound_babay_cry_188_40_INT8_bm1684x.bmodel | TDL_Classification | CLS_SOUND_BABAY_CRY | 188x40 audio feature | background, cry | audio feature/bin |
| cls_sound_dakaiqianlu_126_40_INT8_bm1684x.bmodel | TDL_Classification | CLS_SOUND_COMMAND 或本地 SDK 扩展 ID 待确认 | 126x40 audio feature | background, dakaiqianlu 待确认 | audio feature/bin |
| cls_sound_nihaoshiyun_126_40_INT8_bm1684x.bmodel | TDL_Classification | CLS_SOUND_COMMAND_NIHAOSHIYUN | 126x40 audio feature | background, nihaoshiyun | audio feature/bin |
| cls_sound_xiaoaixiaoai_126_40_INT8_bm1684x.bmodel | TDL_Classification | CLS_SOUND_COMMAND_XIAOAIXIAOAI | 126x40 audio feature | background, xiaoaixiaoai | audio feature/bin |
| feature_bmface_r34_112_112_INT8_bm1684x.bmodel | TDL_FeatureExtraction | FEATURE_BMFACE_R34 | 112x112 | 512-d BMFace feature | face crop |
| feature_clip_text_1_77_W4BF16_bm1684x.bmodel | TDL_FeatureExtraction | FEATURE_CLIP_TEXT | 1x77 text tokens | text CLIP feature | text tokens |
| feature_cviface_112_112_INT8_bm1684x.bmodel | TDL_FeatureExtraction | FEATURE_CVIFACE | 112x112 | 256-d face feature | face crop |
| feature_mobileclip2_B_img_224_224_INT8_bm1684x.bmodel | TDL_FeatureExtraction | FEATURE_MOBILECLIP2_IMG | 224x224 | image feature | image |
| feature_mobileclip2_B_text_1_77_INT8_bm1684x.bmodel | TDL_FeatureExtraction | FEATURE_MOBILECLIP2_TEXT | 1x77 text tokens | text feature | text tokens |
| keypoint_face_v2_64_64_INT8_bm1684x.bmodel | TDL_Keypoint | KEYPOINT_FACE_V2 | 64x64 | face keypoints | face crop |
| keypoint_hand_128_128_INT8_bm1684x.bmodel | TDL_Keypoint | KEYPOINT_HAND | 128x128 | 21 hand keypoints | hand crop |
| keypoint_license_plate_64_128_INT8_bm1684x.bmodel | TDL_Keypoint | KEYPOINT_LICENSE_PLATE | 64x128 | plate top-left/top-right/bottom-left/bottom-right | license plate crop |
| keypoint_simcc_person17_256_192_INT8_bm1684x.bmodel | TDL_Keypoint | KEYPOINT_SIMCC_PERSON17 | 256x192 | 17 person keypoints | person crop |
| keypoint_yolov8pose_person17_384_640_INT8_bm1684x.bmodel | TDL_Keypoint | KEYPOINT_YOLOV8POSE_PERSON17 | 384x640 | 17 person keypoints + box | full-frame image |
| lstr_det_lane_360_640_MIX_bm1684x.bmodel | TDL_LaneDetection | LSTR_DET_LANE | 360x640 | lane keypoints | full-frame image |
| mbv2_det_person_256_384_INT8_bm1684x.bmodel | TDL_Detection | MBV2_DET_PERSON variant | 256x384 | person | full-frame image |
| mbv2_det_person_256_448_INT8_bm1684x.bmodel | TDL_Detection | MBV2_DET_PERSON | 256x448 | person | full-frame image |
| mbv2_det_person_512_896_INT8_bm1684x.bmodel | TDL_Detection | MBV2_DET_PERSON variant | 512x896 | person | full-frame image |
| mbv2_det_person_896_896_INT8_bm1684x.bmodel | TDL_Detection | MBV2_DET_PERSON variant | 896x896 | person | full-frame image |
| ppyoloe_det_coco80_640_640_INT8_bm1684x.bmodel | TDL_Detection | PPYOLOE_DET_COCO80 | 640x640 | COCO80 | full-frame image |
| recognition_license_plate_24_96_MIX_bm1684x.bmodel | TDL_CharacterRecognition | RECOGNITION_LICENSE_PLATE | 24x96 | 7 license plate characters | license plate crop |
| scrfd_det_face_432_768_INT8_bm1684x.bmodel | TDL_FaceDetection | SCRFD_DET_FACE | 432x768 | face | full-frame image |
| topformer_seg_motion_512_960_INT8_bm1684x.bmodel | TDL_SemanticSegmentation | TOPFORMER_SEG_MOTION | 512x960 | static, transition, motion | full-frame image |
| topformer_seg_person_face_vehicle_384_640_INT8_bm1684x.bmodel | TDL_SemanticSegmentation | TOPFORMER_SEG_PERSON_FACE_VEHICLE | 384x640 | background, person, face, vehicle, license plate | full-frame image |
| tracking_feartrack_128_128_256_256_INT8_bm1684x.bmodel | TDL_Tracking | TRACKING_FEARTRACK | 128x128 + 256x256 | single object tracking | template + search images |
| yolov10n_det_coco80_640_640_INT8_bm1684x.bmodel | TDL_Detection | YOLOV10_DET_COCO80 | 640x640 | COCO80 | full-frame image |
| yolov5m_det_coco80_640_640_INT8_bm1684x.bmodel | TDL_Detection | YOLOV5_DET_COCO80 或 YOLOV5 custom 待确认 | 640x640 | COCO80 | full-frame image |
| yolov5s_det_coco80_640_640_INT8_bm1684x.bmodel | TDL_Detection | YOLOV5_DET_COCO80 | 640x640 | COCO80 | full-frame image |
| yolov6n_det_coco80_640_640_INT8_bm1684x.bmodel | TDL_Detection | YOLOV6_DET_COCO80 | 640x640 | COCO80 | full-frame image |
| yolov6s_det_coco80_640_640_INT8_bm1684x.bmodel | TDL_Detection | YOLOV6_DET_COCO80 或 YOLOV6 custom 待确认 | 640x640 | COCO80 | full-frame image |
| yolov7_tiny_det_coco80_640_640_INT8_bm1684x.bmodel | TDL_Detection | YOLOV7_DET_COCO80 | 640x640 | COCO80 | full-frame image |
| yolov8n_det_bicycle_motor_ebicycle_384_640_INT8_bm1684x.bmodel | TDL_Detection | YOLOV8N_DET_BICYCLE_MOTOR_EBICYCLE | 384x640 | bicycle, motorcycle, ebicycle | full-frame image |
| yolov8n_det_bicycle_motor_ebicycle_mbv2_384_640_INT8_bm1684x.bmodel | TDL_Detection | YOLOV8N_DET_BICYCLE_MOTOR_EBICYCLE variant | 384x640 | bicycle, motorcycle, ebicycle | full-frame image |
| yolov8n_det_coco80_640_640_INT8_bm1684x.bmodel | TDL_Detection | YOLOV8_DET_COCO80 | 640x640 | COCO80 | full-frame image |
| yolov8n_det_face_head_person_pet_384_640_INT8_bm1684x.bmodel | TDL_Detection | YOLOV8N_DET_FACE_HEAD_PERSON_PET | 384x640 | face, head, person, pet | full-frame image |
| yolov8n_det_fire_384_640_INT8_bm1684x.bmodel | TDL_Detection | YOLOV8N_DET_FIRE | 384x640 | fire | full-frame image |
| yolov8n_det_fire_smoke_384_640_INT8_bm1684x.bmodel | TDL_Detection | YOLOV8N_DET_FIRE_SMOKE | 384x640 | fire, smoke | full-frame image |
| yolov8n_det_hand_384_640_INT8_bm1684x.bmodel | TDL_Detection | YOLOV8N_DET_HAND | 384x640 | hand | full-frame image |
| yolov8n_det_hand_face_person_384_640_INT8_bm1684x.bmodel | TDL_Detection | YOLOV8N_DET_HAND_FACE_PERSON | 384x640 | hand, face, person | full-frame image |
| yolov8n_det_hand_mv3_384_640_INT8_bm1684x.bmodel | TDL_Detection | YOLOV8N_DET_HAND variant | 384x640 | hand | full-frame image |
| yolov8n_det_head_hardhat_576_960_INT8_bm1684x.bmodel | TDL_Detection | YOLOV8N_DET_HEAD_HARDHAT | 576x960 | head, hardhat | full-frame image |
| yolov8n_det_head_person_384_640_INT8_bm1684x.bmodel | TDL_Detection | YOLOV8N_DET_HEAD_PERSON | 384x640 | head, person | full-frame image |
| yolov8n_det_head_shoulder_384_640_INT8_bm1684x.bmodel | TDL_Detection | YOLOV8N_DET_HEAD_SHOULDER | 384x640 | head shoulder | full-frame image |
| yolov8n_det_head_shoulder_mbv2_384_640_INT8_bm1684x.bmodel | TDL_Detection | YOLOV8N_DET_HEAD_SHOULDER variant | 384x640 | head shoulder | full-frame image |
| yolov8n_det_ir_person_384_640_INT8_bm1684x.bmodel | TDL_Detection | YOLOV8N_DET_MONITOR_PERSON same-family | 384x640 | person | full-frame image |
| yolov8n_det_ir_person_mbv2_384_640_INT8_bm1684x.bmodel | TDL_Detection | YOLOV8N_DET_MONITOR_PERSON variant | 384x640 | person | full-frame image |
| yolov8n_det_license_plate_384_640_INT8_bm1684x.bmodel | TDL_Detection | YOLOV8N_DET_LICENSE_PLATE | 384x640 | license plate | full-frame image |
| yolov8n_det_monitor_person_256_448_INT8_bm1684x.bmodel | TDL_Detection | YOLOV8N_DET_MONITOR_PERSON | 256x448 | person | full-frame image |
| yolov8n_det_overlook_person_256_448_INT8_bm1684x.bmodel | TDL_Detection | YOLOV8N_DET_MONITOR_PERSON same-family | 256x448 | person | full-frame image |
| yolov8n_det_person_vehicle_384_640_INT8_bm1684x.bmodel | TDL_Detection | YOLOV8N_DET_PERSON_VEHICLE | 384x640 | car, bus, truck, rider with motorcycle, person, bike, motorcycle | full-frame image |
| yolov8n_det_person_vehicle_mv2_035_384_640_INT8_bm1684x.bmodel | TDL_Detection | YOLOV8N_DET_PERSON_VEHICLE variant | 384x640 | car, bus, truck, rider with motorcycle, person, bike, motorcycle | full-frame image |
| yolov8n_det_pet_person_035_384_640_INT8_bm1684x.bmodel | TDL_Detection | YOLOV8N_DET_PET_PERSON variant | 384x640 | cat, dog, person | full-frame image |
| yolov8n_det_pet_person_384_640_INT8_bm1684x.bmodel | TDL_Detection | YOLOV8N_DET_PET_PERSON | 384x640 | cat, dog, person | full-frame image |
| yolov8n_det_traffic_light_384_640_INT8_bm1684x.bmodel | TDL_Detection | YOLOV8N_DET_TRAFFIC_LIGHT | 384x640 | red, yellow, green, off, wait on | full-frame image |
| yolov8n_seg_coco80_640_640_INT8_bm1684x.bmodel | TDL_InstanceSegmentation | YOLOV8_SEG_COCO80 | 640x640 | COCO80 instance mask | full-frame image |
| yolov8s_det_coco80_640_640_INT8_bm1684x.bmodel | TDL_Detection | YOLOV8_DET_COCO80 或 YOLOV8 custom 待确认 | 640x640 | COCO80 | full-frame image |
| yolox_m_det_coco80_640_640_INT8_bm1684x.bmodel | TDL_Detection | YOLOX_DET_COCO80 | 640x640 | COCO80 | full-frame image |
| yolox_s_det_coco80_640_640_INT8_bm1684x.bmodel | TDL_Detection | YOLOX_DET_COCO80 或 YOLOX custom 待确认 | 640x640 | COCO80 | full-frame image |

## 规格分组

| 输入形态 | 代表规格 | 代表模型 | sophon-stream 接入难度 |
|---|---|---|---|
| full-frame image | 256x384, 256x448, 360x640, 384x640, 432x768, 512x960, 576x960, 640x640, 896x896 | 检测、人脸检测、分割、车道线 | 最适合第一阶段接入 |
| crop image | 24x96, 64x64, 64x128, 112x112, 128x128, 256x192, 256x256 | 人脸属性、车牌识别、关键点、特征 | 需要上游检测或 ROI 裁剪 |
| vector | 1x42 | 手势关键点分类 | 不能直接复用 decode，需要上游 keypoint 结果 |
| text tokens | 1x77 | CLIP text / MobileCLIP text | 不能复用 decode，需要文本 tokenizer |
| audio feature | 126x40, 188x40 | 声音分类 | 不能复用视频 decode，需要音频前处理 |
| multi-input tracking | 128x128 + 256x256 | feartrack | 需要状态管理和模板帧/搜索帧 |

## 对插件设计的影响

建议通用 TDL 插件内部至少抽象出三层：

1. `TdlContext`：管理 `TDLHandle`、`TDLModel`、`model_path`、`model_config_path`、`threshold`、`vpss_dev`。
2. `TdlInputAdapter`：把 sophon-stream 的 `bm_image`、裁剪图、向量、音频特征、文本 token 转成 TDL 输入。
3. `TdlResultAdapter`：把 `TDLObject`、`TDLFace`、`TDLClass`、`TDLKeypoint`、`TDLSegmentation` 等转换为 `ObjectMetadata` 的标准字段。

第一阶段建议实现：

```text
decode -> tdl_detection -> osd -> encode
```

支持的 API：

```text
TDL_Detection
TDL_FaceDetection
TDL_SemanticSegmentation
TDL_InstanceSegmentation
TDL_LaneDetection
```

第二阶段再扩展：

```text
TDL_FaceAttribute
TDL_Classification
TDL_Keypoint
TDL_CharacterRecognition
TDL_FeatureExtraction
TDL_Tracking
```

## 需要继续验证的问题和方法

### 1. 精确输入输出 tensor 规格

目的：确认 `.bmodel` 的真实 input/output 名称、shape、dtype、batch、scale。

在 BM1684X 设备上执行：

```bash
bmrt_test --bmodel /data/tdl_models/bm1684x/yolov8n_det_person_vehicle_384_640_INT8_bm1684x.bmodel --loopnum 1
```

对所有模型批量执行：

```bash
for m in /data/tdl_models/bm1684x/*.bmodel; do
  echo "===== $m ====="
  bmrt_test --bmodel "$m" --loopnum 1 2>&1 | tee -a /data/tdl_models/bm1684x_bmrt_info.log
done
```

如果设备没有 `bmrt_test`，需要从 libsophon / SDK 工具包中补齐。

### 2. TDL 运行时预处理参数

目的：确认每个 `TDLModel` 的 `dst_width`、`dst_height`、`mean`、`scale`、`dst_image_format`、`keep_aspect_ratio`。

写一个最小 C++ 程序：

```cpp
TDLHandle handle = TDL_CreateHandle(0);
TDL_OpenModel(handle, model_id, model_path, nullptr, 0);
TDLPreprocessParams p;
TDL_GetPreprocessParameters(handle, model_id, &p);
printf("w=%d h=%d mean=(%f,%f,%f) scale=(%f,%f,%f) fmt=%d keep=%d\n",
       p.dst_width, p.dst_height,
       p.mean[0], p.mean[1], p.mean[2],
       p.scale[0], p.scale[1], p.scale[2],
       p.dst_image_format, p.keep_aspect_ratio);
TDL_CloseModel(handle, model_id);
TDL_DestroyHandle(handle);
```

这是插件初始化阶段最值得加入的日志。

### 3. `bm_image` 能否零拷贝包装成 TDLImage

目的：确认 sophon-stream 解码输出是否能直接被 TDL SDK 使用。

已知 TDL SDK 有：

```cpp
TDLImage TDL_WrapFrame(void *frame, bool own_memory, bool is_preprocessed);
```

但注释要求 `frame` 是 `VIDEO_FRAME_INFO_S`。sophon-stream decode 输出是 `bm_image`，位于：

```cpp
objectMetadata->mFrame->mSpData
```

验证方法：

1. 在 TDL SDK 源码中查 `TDL_WrapFrame` 实现，确认 `void *frame` 的真实结构。
2. 写一个小程序，用 sophon-stream decode 得到一帧 `bm_image`。
3. 尝试构造 TDL 所需 frame 并调用 `TDL_WrapFrame`。
4. 与 `TDL_ReadImage(path)` 的检测结果对比。

如果无法直接包装，临时 POC 可以先走 JPEG/CPU 中转，但实时视频插件不建议长期这样做。

### 4. 结果释放接口

目的：避免长视频运行时内存泄漏。

TDL 输出结构有动态指针，例如 `TDLObject.info`、`TDLClass.info`、`TDLSegmentation.class_id`。使用后需要匹配释放：

```cpp
TDL_ReleaseObjectMeta(&object_meta);
TDL_ReleaseClassMeta(&class_meta);
TDL_ReleaseSemanticSegMeta(&seg_meta);
TDL_ReleaseInstanceSegMeta(&inst_seg_meta);
TDL_ReleaseKeypointMeta(&keypoint_meta);
TDL_DestroyImage(image);
```

验证方法：

```bash
top -p $(pidof main)
cat /proc/$(pidof main)/status | grep -E "VmRSS|VmSize"
```

连续运行 30 分钟，观察内存是否持续上涨。

### 5. 多线程安全

目的：确认一个 `TDLHandle` 是否能被 sophon-stream 多线程 element 共享。

验证方法：

1. 单线程 `thread_number=1` 跑通。
2. 多线程共享一个 `TDLHandle` 跑 1000 帧。
3. 多线程每个 worker 独立 `TDLHandle` 跑 1000 帧。
4. 比较崩溃、结果一致性和性能。

推荐初版使用每个 element 实例一个 `TDLHandle`，避免共享状态问题。

### 6. 下游 OSD / Encode 兼容性

目的：确认 TDL 插件输出能被 sophon-stream 现有模块消费。

检测任务需要写入：

```cpp
objectMetadata->mDetectedObjectMetadatas
```

分类任务需要写入：

```cpp
objectMetadata->mRecognizedObjectMetadatas
```

分割任务需要写入：

```cpp
objectMetadata->mSegmentedObjectMetadatas
```

验证 pipeline：

```text
decode -> tdl -> osd -> encode
```

若输出图片中能看到框/类别，说明 metadata 适配成功。

## 参考来源

- 本地模型目录：`C:\project\tdl_models\bm1684x`
- 本地 README：`C:\project\tdl_models\README.md`
- TDL SDK model factory: https://raw.githubusercontent.com/sophgo/tdl_sdk/master/configs/model/model_factory.json
- TDL SDK C API: https://raw.githubusercontent.com/sophgo/tdl_sdk/master/include/c_apis/tdl_sdk.h
- TDL SDK types: https://raw.githubusercontent.com/sophgo/tdl_sdk/master/include/c_apis/tdl_types.h
- TDL model list: https://raw.githubusercontent.com/sophgo/tdl_sdk/master/include/nn/tdl_model_list.h
