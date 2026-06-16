//===----------------------------------------------------------------------===//
//
// Copyright (C) 2022 Sophgo Technologies Inc.  All rights reserved.
//
// SOPHON-STREAM is licensed under the 2-Clause BSD License except for the
// third-party components.
//
//===----------------------------------------------------------------------===//

#include "tdl.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <exception>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>
#include <unordered_map>

#include "common/logger.h"
#include "common/object_metadata.h"

using namespace std::chrono_literals;

namespace sophon_stream {
namespace element {
namespace tdl {

namespace {

static int clampInt(float v, int low, int high) {
  int iv = static_cast<int>(v + 0.5f);
  return std::max(low, std::min(iv, high));
}

static void mkdirRecursive(const std::string& path) {
  if (path.empty()) return;
  std::string cur;
  for (char c : path) {
    cur.push_back(c);
    if (c == '/' && cur.size() > 1) mkdir(cur.c_str(), 0755);
  }
  mkdir(path.c_str(), 0755);
}

}  // namespace

Tdl::Tdl() {}

Tdl::~Tdl() {
  // POC note: TDL SDK/BMRuntime may throw during explicit teardown after the
  // stream graph has stopped. Keep the process exit clean and let the OS reclaim
  // the handle. A production plugin should revisit this with the target SDK.
  if (mHandle != nullptr) {
    IVS_WARN("skip explicit tdl handle cleanup for poc");
    mHandle = nullptr;
  }
}

bool Tdl::parseTaskType(const std::string& taskType, TdlTaskType& out) const {
  std::string type = taskType;
  std::transform(type.begin(), type.end(), type.begin(), ::tolower);
  if (type == "detect" || type == "detection") {
    out = TdlTaskType::Detection;
    return true;
  }
  if (type == "classify" || type == "classification" || type == "cls") {
    out = TdlTaskType::Classification;
    return true;
  }
  return false;
}

bool Tdl::modelIdFromString(const std::string& modelId, TDLModel& out) const {
  static const std::unordered_map<std::string, TDLModel> kModelIds = {
      {"MBV2_DET_PERSON", TDL_MODEL_MBV2_DET_PERSON},
      {"YOLOV8N_DET_HAND", TDL_MODEL_YOLOV8N_DET_HAND},
      {"YOLOV8N_DET_PET_PERSON", TDL_MODEL_YOLOV8N_DET_PET_PERSON},
      {"YOLOV8N_DET_BICYCLE_MOTOR_EBICYCLE",
       TDL_MODEL_YOLOV8N_DET_BICYCLE_MOTOR_EBICYCLE},
      {"YOLOV8N_DET_PERSON_VEHICLE",
       TDL_MODEL_YOLOV8N_DET_PERSON_VEHICLE},
      {"YOLOV8N_DET_HAND_FACE_PERSON",
       TDL_MODEL_YOLOV8N_DET_HAND_FACE_PERSON},
      {"YOLOV8N_DET_FACE_HEAD_PERSON_PET",
       TDL_MODEL_YOLOV8N_DET_FACE_HEAD_PERSON_PET},
      {"YOLOV8N_DET_HEAD_PERSON", TDL_MODEL_YOLOV8N_DET_HEAD_PERSON},
      {"YOLOV8N_DET_HEAD_HARDHAT", TDL_MODEL_YOLOV8N_DET_HEAD_HARDHAT},
      {"YOLOV8N_DET_FIRE_SMOKE", TDL_MODEL_YOLOV8N_DET_FIRE_SMOKE},
      {"YOLOV8N_DET_FIRE", TDL_MODEL_YOLOV8N_DET_FIRE},
      {"YOLOV8N_DET_HEAD_SHOULDER", TDL_MODEL_YOLOV8N_DET_HEAD_SHOULDER},
      {"YOLOV8N_DET_LICENSE_PLATE", TDL_MODEL_YOLOV8N_DET_LICENSE_PLATE},
      {"YOLOV8N_DET_TRAFFIC_LIGHT", TDL_MODEL_YOLOV8N_DET_TRAFFIC_LIGHT},
      {"YOLOV8N_DET_MONITOR_PERSON", TDL_MODEL_YOLOV8N_DET_MONITOR_PERSON},
      {"YOLOV5_DET_COCO80", TDL_MODEL_YOLOV5_DET_COCO80},
      {"YOLOV6_DET_COCO80", TDL_MODEL_YOLOV6_DET_COCO80},
      {"YOLOV7_DET_COCO80", TDL_MODEL_YOLOV7_DET_COCO80},
      {"YOLOV8_DET_COCO80", TDL_MODEL_YOLOV8_DET_COCO80},
      {"YOLOV10_DET_COCO80", TDL_MODEL_YOLOV10_DET_COCO80},
      {"PPYOLOE_DET_COCO80", TDL_MODEL_PPYOLOE_DET_COCO80},
      {"YOLOX_DET_COCO80", TDL_MODEL_YOLOX_DET_COCO80},
      {"SCRFD_DET_FACE", TDL_MODEL_SCRFD_DET_FACE},
      {"CLS_RGBLIVENESS", TDL_MODEL_CLS_RGBLIVENESS},
      {"CLS_HAND_GESTURE", TDL_MODEL_CLS_HAND_GESTURE},
      {"CLS_KEYPOINT_HAND_GESTURE",
       TDL_MODEL_CLS_KEYPOINT_HAND_GESTURE},
      {"CLS_SOUND_BABAY_CRY", TDL_MODEL_CLS_SOUND_BABAY_CRY},
      {"CLS_SOUND_COMMAND_NIHAOSHIYUN",
       TDL_MODEL_CLS_SOUND_COMMAND_NIHAOSHIYUN},
      {"CLS_SOUND_COMMAND_XIAOAIXIAOAI",
       TDL_MODEL_CLS_SOUND_COMMAND_XIAOAIXIAOAI},
      {"CLS_ATTRIBUTE_GENDER_AGE_GLASS",
       TDL_MODEL_CLS_ATTRIBUTE_GENDER_AGE_GLASS},
      {"CLS_ATTRIBUTE_GENDER_AGE_GLASS_MASK",
       TDL_MODEL_CLS_ATTRIBUTE_GENDER_AGE_GLASS_MASK},
      {"CLS_ATTRIBUTE_GENDER_AGE_GLASS_EMOTION",
       TDL_MODEL_CLS_ATTRIBUTE_GENDER_AGE_GLASS_EMOTION},
      {"KEYPOINT_FACE_V2", TDL_MODEL_KEYPOINT_FACE_V2},
      {"KEYPOINT_LICENSE_PLATE", TDL_MODEL_KEYPOINT_LICENSE_PLATE},
      {"KEYPOINT_HAND", TDL_MODEL_KEYPOINT_HAND},
      {"KEYPOINT_YOLOV8POSE_PERSON17",
       TDL_MODEL_KEYPOINT_YOLOV8POSE_PERSON17},
      {"KEYPOINT_SIMCC_PERSON17", TDL_MODEL_KEYPOINT_SIMCC_PERSON17},
      {"LSTR_DET_LANE", TDL_MODEL_LSTR_DET_LANE},
      {"RECOGNITION_LICENSE_PLATE",
       TDL_MODEL_RECOGNITION_LICENSE_PLATE},
      {"YOLOV8_SEG_COCO80", TDL_MODEL_YOLOV8_SEG_COCO80},
      {"TOPFORMER_SEG_PERSON_FACE_VEHICLE",
       TDL_MODEL_TOPFORMER_SEG_PERSON_FACE_VEHICLE},
      {"TOPFORMER_SEG_MOTION", TDL_MODEL_TOPFORMER_SEG_MOTION},
      {"FEATURE_CVIFACE", TDL_MODEL_FEATURE_CVIFACE},
      {"FEATURE_BMFACE_R34", TDL_MODEL_FEATURE_BMFACE_R34},
      {"FEATURE_CLIP_TEXT", TDL_MODEL_FEATURE_CLIP_TEXT},
      {"FEATURE_MOBILECLIP2_IMG", TDL_MODEL_FEATURE_MOBILECLIP2_IMG},
      {"FEATURE_MOBILECLIP2_TEXT", TDL_MODEL_FEATURE_MOBILECLIP2_TEXT},
      {"TRACKING_FEARTRACK", TDL_MODEL_TRACKING_FEARTRACK},
  };

  std::string key = modelId;
  std::transform(key.begin(), key.end(), key.begin(), ::toupper);
  auto it = kModelIds.find(key);
  if (it == kModelIds.end()) return false;
  out = it->second;
  return true;
}

void Tdl::fillDefaultClassNames() {
  if (!mClassNames.empty()) return;
  if (mModelId == TDL_MODEL_YOLOV8N_DET_PERSON_VEHICLE) {
    mClassNames = {"car", "bus", "truck", "rider with motorcycle",
                   "person", "bike", "motorcycle"};
  } else if (mModelId == TDL_MODEL_YOLOV8N_DET_PET_PERSON) {
    mClassNames = {"cat", "dog", "person"};
  } else if (mModelId == TDL_MODEL_YOLOV8N_DET_HAND_FACE_PERSON) {
    mClassNames = {"hand", "face", "person"};
  } else if (mModelId == TDL_MODEL_YOLOV8N_DET_HAND) {
    mClassNames = {"hand"};
  } else if (mModelId == TDL_MODEL_MBV2_DET_PERSON ||
             mModelId == TDL_MODEL_YOLOV8N_DET_MONITOR_PERSON) {
    mClassNames = {"person"};
  }
}

void Tdl::ensureTempDir() { mkdirRecursive(mTempDir); }

common::ErrorCode Tdl::initContext(const nlohmann::json& configure) {
  if (!configure.contains(CONFIG_MODEL_ID) ||
      !configure.contains(CONFIG_MODEL_PATH)) {
    IVS_ERROR("tdl config requires model_id and model_path");
    return common::ErrorCode::PARSE_CONFIGURE_FAIL;
  }

  mModelIdName = configure[CONFIG_MODEL_ID].get<std::string>();
  if (!modelIdFromString(mModelIdName, mModelId)) {
    IVS_ERROR("unsupported tdl model_id: {}", mModelIdName);
    return common::ErrorCode::PARSE_CONFIGURE_FAIL;
  }

  mModelPath = configure[CONFIG_MODEL_PATH].get<std::string>();
  if (configure.contains(CONFIG_TASK_TYPE)) {
    TdlTaskType taskType;
    if (!parseTaskType(configure[CONFIG_TASK_TYPE].get<std::string>(),
                       taskType)) {
      IVS_ERROR("unsupported tdl task_type: {}",
                configure[CONFIG_TASK_TYPE].get<std::string>());
      return common::ErrorCode::PARSE_CONFIGURE_FAIL;
    }
    mTaskType = taskType;
  }
  if (configure.contains(CONFIG_MODEL_CONFIG_PATH))
    mModelConfigPath = configure[CONFIG_MODEL_CONFIG_PATH].get<std::string>();
  if (configure.contains(CONFIG_MODEL_DIR))
    mModelDir = configure[CONFIG_MODEL_DIR].get<std::string>();
  if (configure.contains(CONFIG_THRESHOLD))
    mThreshold = configure[CONFIG_THRESHOLD].get<float>();
  if (configure.contains(CONFIG_TEMP_DIR))
    mTempDir = configure[CONFIG_TEMP_DIR].get<std::string>();
  if (configure.contains(CONFIG_VPSS_DEV))
    mVpssDev = configure[CONFIG_VPSS_DEV].get<int>();
  if (configure.contains(CONFIG_CLASS_NAMES))
    mClassNames = configure[CONFIG_CLASS_NAMES].get<std::vector<std::string>>();
  fillDefaultClassNames();
  ensureTempDir();

  mHandle = TDL_CreateHandle(getDeviceId());
  if (mHandle == nullptr) {
    IVS_ERROR("TDL_CreateHandle failed, device_id={}", getDeviceId());
    return common::ErrorCode::UNKNOWN;
  }
  if (!mModelDir.empty() && TDL_SetModelDir(mHandle, mModelDir.c_str()) != 0) {
    IVS_WARN("TDL_SetModelDir failed: {}", mModelDir);
  }
  if (!mModelConfigPath.empty() &&
      TDL_LoadModelConfig(mHandle, mModelConfigPath.c_str()) != 0) {
    IVS_WARN("TDL_LoadModelConfig failed: {}", mModelConfigPath);
  }

  const char* modelConfig =
      mModelConfigPath.empty() ? nullptr : mModelConfigPath.c_str();
  if (TDL_OpenModel(mHandle, mModelId, mModelPath.c_str(), modelConfig,
                    mVpssDev) != 0) {
    IVS_ERROR("TDL_OpenModel failed, model_id={}, model_path={}",
              mModelIdName, mModelPath);
    return common::ErrorCode::UNKNOWN;
  }
  TDL_SetModelThreshold(mHandle, mModelId, mThreshold);

  TDLPreprocessParams preParam;
  if (TDL_GetPreprocessParameters(mHandle, mModelId, &preParam) == 0) {
    IVS_INFO(
        "tdl model opened: id={}, path={}, input={}x{}, format={}, dtype={}, "
        "keep_aspect={}, threshold={}",
        mModelIdName, mModelPath, preParam.dst_width, preParam.dst_height,
        static_cast<int>(preParam.dst_image_format),
        static_cast<int>(preParam.dst_pixdata_type),
        static_cast<int>(preParam.keep_aspect_ratio), mThreshold);
  } else {
    IVS_INFO("tdl model opened: id={}, path={}, threshold={}", mModelIdName,
             mModelPath, mThreshold);
  }
  return common::ErrorCode::SUCCESS;
}

common::ErrorCode Tdl::initInternal(const std::string& json) {
  auto configure = nlohmann::json::parse(json, nullptr, false);
  if (!configure.is_object()) {
    return common::ErrorCode::PARSE_CONFIGURE_FAIL;
  }
  mFpsProfiler.config("fps_tdl", 100);
  return initContext(configure);
}

std::string Tdl::saveFrameAsJpeg(
    std::shared_ptr<common::ObjectMetadata> objectMetadata) {
  if (!objectMetadata || !objectMetadata->mFrame ||
      !objectMetadata->mFrame->mSpData) {
    return "";
  }

  bm_image src = *objectMetadata->mFrame->mSpData;
  bm_image imageStorage;
  int width = objectMetadata->mFrame->mWidth > 0 ? objectMetadata->mFrame->mWidth
                                                 : src.width;
  int height = objectMetadata->mFrame->mHeight > 0
                   ? objectMetadata->mFrame->mHeight
                   : src.height;
  bm_image_create(objectMetadata->mFrame->mHandle, height, width,
                  FORMAT_YUV420P, src.data_type, &imageStorage);
  bmcv_rect_t cropRect = {0, 0, static_cast<unsigned int>(src.width),
                          static_cast<unsigned int>(src.height)};
  bm_status_t ret = bmcv_image_vpp_convert(objectMetadata->mFrame->mHandle, 1,
                                           src, &imageStorage, &cropRect);
  if (ret != BM_SUCCESS) {
    IVS_ERROR("tdl frame convert to yuv failed, ret={}", ret);
    bm_image_destroy(imageStorage);
    return "";
  }

  void* jpegData = nullptr;
  size_t jpegSize = 0;
  ret = bmcv_image_jpeg_enc(objectMetadata->mFrame->mHandle, 1, &imageStorage,
                            &jpegData, &jpegSize);
  bm_image_destroy(imageStorage);
  if (ret != BM_SUCCESS || jpegData == nullptr || jpegSize == 0) {
    IVS_ERROR("tdl frame jpeg encode failed, ret={}", ret);
    if (jpegData) free(jpegData);
    return "";
  }

  std::ostringstream oss;
  oss << mTempDir << "/g" << objectMetadata->mGraphId << "_c"
      << objectMetadata->mFrame->mChannelId << "_f"
      << objectMetadata->mFrame->mFrameId << ".jpg";
  std::string path = oss.str();
  FILE* fp = fopen(path.c_str(), "wb");
  if (!fp) {
    IVS_ERROR("tdl open temp image failed: {}", path);
    free(jpegData);
    return "";
  }
  fwrite(jpegData, jpegSize, 1, fp);
  fclose(fp);
  free(jpegData);
  return path;
}

common::ErrorCode Tdl::runDetection(
    std::shared_ptr<common::ObjectMetadata> objectMetadata,
    TDLImage image) {
  TDLObject objectMeta;
  memset(&objectMeta, 0, sizeof(objectMeta));
  int ret = TDL_Detection(mHandle, mModelId, image, &objectMeta);
  if (ret != 0) {
    IVS_ERROR("TDL_Detection failed, model_id={}, ret={}", mModelIdName, ret);
    return common::ErrorCode::UNKNOWN;
  }

  objectMetadata->mDetectedObjectMetadatas.clear();
  int frameW = objectMetadata->mFrame->mWidth > 0
                   ? objectMetadata->mFrame->mWidth
                   : static_cast<int>(objectMeta.width);
  int frameH = objectMetadata->mFrame->mHeight > 0
                   ? objectMetadata->mFrame->mHeight
                   : static_cast<int>(objectMeta.height);

  for (uint32_t i = 0; i < objectMeta.size; ++i) {
    TDLObjectInfo& info = objectMeta.info[i];
    int label = std::max(0, info.class_id);
    int x1 = clampInt(info.box.x1, 0, std::max(0, frameW - 1));
    int y1 = clampInt(info.box.y1, 0, std::max(0, frameH - 1));
    int x2 = clampInt(info.box.x2, 0, std::max(0, frameW - 1));
    int y2 = clampInt(info.box.y2, 0, std::max(0, frameH - 1));
    if (x2 <= x1 || y2 <= y1) continue;

    auto det = std::make_shared<common::DetectedObjectMetadata>();
    det->mBox.mX = x1;
    det->mBox.mY = y1;
    det->mBox.mWidth = x2 - x1;
    det->mBox.mHeight = y2 - y1;
    det->mScores.resize(label + 1, 0.0f);
    det->mScores[label] = info.score;
    det->mTopKLabels.push_back(label);
    if (label >= 0 && label < static_cast<int>(mClassNames.size()))
      det->mLabelName = mClassNames[label];
    else if (info.name[0] != '\0')
      det->mLabelName = info.name;
    det->mClassify = label;
    det->mClassifyName = det->mLabelName;
    objectMetadata->mDetectedObjectMetadatas.push_back(det);
  }
  IVS_INFO("tdl detection channel={}, frame={}, objects={}",
           objectMetadata->mFrame->mChannelId, objectMetadata->mFrame->mFrameId,
           objectMetadata->mDetectedObjectMetadatas.size());
  TDL_ReleaseObjectMeta(&objectMeta);
  return common::ErrorCode::SUCCESS;
}

common::ErrorCode Tdl::runClassification(
    std::shared_ptr<common::ObjectMetadata> objectMetadata,
    TDLImage image) {
  TDLClassInfo classInfo;
  memset(&classInfo, 0, sizeof(classInfo));
  int ret = TDL_Classification(mHandle, mModelId, image, &classInfo);
  if (ret != 0) {
    IVS_ERROR("TDL_Classification failed, model_id={}, ret={}", mModelIdName,
              ret);
    return common::ErrorCode::UNKNOWN;
  }

  objectMetadata->mRecognizedObjectMetadatas.clear();
  int label = std::max(0, classInfo.class_id);
  auto recog = std::make_shared<common::RecognizedObjectMetadata>();
  recog->mScores.resize(label + 1, 0.0f);
  recog->mScores[label] = classInfo.score;
  recog->mTopKLabels.push_back(label);
  if (label >= 0 && label < static_cast<int>(mClassNames.size()))
    recog->mLabelName = mClassNames[label];
  objectMetadata->mRecognizedObjectMetadatas.push_back(recog);
  IVS_INFO("tdl classification channel={}, frame={}, class={}, score={}",
           objectMetadata->mFrame->mChannelId, objectMetadata->mFrame->mFrameId,
           label, classInfo.score);
  return common::ErrorCode::SUCCESS;
}

common::ErrorCode Tdl::processObject(
    std::shared_ptr<common::ObjectMetadata> objectMetadata) {
  if (!objectMetadata || !objectMetadata->mFrame ||
      objectMetadata->mFrame->mEndOfStream || objectMetadata->mFilter) {
    return common::ErrorCode::SUCCESS;
  }

  std::string imagePath = saveFrameAsJpeg(objectMetadata);
  if (imagePath.empty()) return common::ErrorCode::UNKNOWN;

  TDLImage image = TDL_ReadImage(imagePath.c_str());
  unlink(imagePath.c_str());
  if (image == nullptr) {
    IVS_ERROR("TDL_ReadImage failed: {}", imagePath);
    return common::ErrorCode::UNKNOWN;
  }

  common::ErrorCode ret = common::ErrorCode::SUCCESS;
  {
    std::lock_guard<std::mutex> lock(mTdlMutex);
    if (mTaskType == TdlTaskType::Detection)
      ret = runDetection(objectMetadata, image);
    else if (mTaskType == TdlTaskType::Classification)
      ret = runClassification(objectMetadata, image);
  }
  TDL_DestroyImage(image);
  return ret;
}

common::ErrorCode Tdl::doWork(int dataPipeId) {
  std::vector<int> inputPorts = getInputPorts();
  int inputPort = inputPorts[0];
  int outputPort = 0;
  if (!getSinkElementFlag()) {
    std::vector<int> outputPorts = getOutputPorts();
    outputPort = outputPorts[0];
  }

  common::ObjectMetadatas pendingObjectMetadatas;
  while (pendingObjectMetadatas.size() < static_cast<size_t>(mMaxBatch) &&
         getThreadStatus() == ThreadStatus::RUN) {
    auto data = popInputData(inputPort, dataPipeId);
    if (!data) {
      std::this_thread::sleep_for(10ms);
      continue;
    }
    auto objectMetadata =
        std::static_pointer_cast<common::ObjectMetadata>(data);
    pendingObjectMetadatas.push_back(objectMetadata);
    if (objectMetadata->mFrame && objectMetadata->mFrame->mEndOfStream) break;
  }

  for (auto& objectMetadata : pendingObjectMetadatas) {
    common::ErrorCode ret = processObject(objectMetadata);
    if (ret != common::ErrorCode::SUCCESS) objectMetadata->mErrorCode = ret;
  }

  for (auto& objectMetadata : pendingObjectMetadatas) {
    int channelIdInternal = objectMetadata->mFrame->mChannelIdInternal;
    int outDataPipeId =
        getSinkElementFlag()
            ? 0
            : (channelIdInternal % getOutputConnectorCapacity(outputPort));
    auto errorCode = pushOutputData(
        outputPort, outDataPipeId, std::static_pointer_cast<void>(objectMetadata));
    if (errorCode != common::ErrorCode::SUCCESS) {
      IVS_WARN("tdl send data fail, element id: {}, output port: {}", getId(),
               outputPort);
    }
  }
  mFpsProfiler.add(pendingObjectMetadatas.size());
  return common::ErrorCode::SUCCESS;
}

REGISTER_WORKER("tdl", Tdl)

}  // namespace tdl
}  // namespace element
}  // namespace sophon_stream
