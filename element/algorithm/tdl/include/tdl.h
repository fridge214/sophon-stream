//===----------------------------------------------------------------------===//
//
// Copyright (C) 2022 Sophgo Technologies Inc.  All rights reserved.
//
// SOPHON-STREAM is licensed under the 2-Clause BSD License except for the
// third-party components.
//
//===----------------------------------------------------------------------===//

#ifndef SOPHON_STREAM_ELEMENT_TDL_H_
#define SOPHON_STREAM_ELEMENT_TDL_H_

#include <map>
#include <mutex>
#include <string>
#include <vector>

#include "c_apis/tdl_sdk.h"
#include "common/object_metadata.h"
#include "common/profiler.h"
#include "element_factory.h"
#include "element.h"
#include "nlohmann/json.hpp"

namespace sophon_stream {
namespace element {
namespace tdl {

enum class TdlTaskType { Detection = 0, Classification };

class Tdl : public ::sophon_stream::framework::Element {
 public:
  Tdl();
  ~Tdl() override;

  common::ErrorCode initInternal(const std::string& json) override;
  common::ErrorCode doWork(int dataPipeId) override;

 private:
  static constexpr const char* CONFIG_MODEL_ID = "model_id";
  static constexpr const char* CONFIG_MODEL_PATH = "model_path";
  static constexpr const char* CONFIG_MODEL_CONFIG_PATH = "model_config_path";
  static constexpr const char* CONFIG_MODEL_DIR = "model_dir";
  static constexpr const char* CONFIG_TASK_TYPE = "task_type";
  static constexpr const char* CONFIG_THRESHOLD = "threshold";
  static constexpr const char* CONFIG_CLASS_NAMES = "class_names";
  static constexpr const char* CONFIG_TEMP_DIR = "temp_dir";
  static constexpr const char* CONFIG_VPSS_DEV = "vpss_dev";

  common::ErrorCode initContext(const nlohmann::json& configure);
  common::ErrorCode processObject(
      std::shared_ptr<common::ObjectMetadata> objectMetadata);

  common::ErrorCode runDetection(
      std::shared_ptr<common::ObjectMetadata> objectMetadata,
      TDLImage image);
  common::ErrorCode runClassification(
      std::shared_ptr<common::ObjectMetadata> objectMetadata,
      TDLImage image);

  bool modelIdFromString(const std::string& modelId, TDLModel& out) const;
  bool parseTaskType(const std::string& taskType, TdlTaskType& out) const;
  std::string saveFrameAsJpeg(
      std::shared_ptr<common::ObjectMetadata> objectMetadata);
  void ensureTempDir();
  void fillDefaultClassNames();

  TDLHandle mHandle = nullptr;
  TDLModel mModelId = TDL_MODEL_INVALID;
  TdlTaskType mTaskType = TdlTaskType::Detection;
  std::string mModelIdName;
  std::string mModelPath;
  std::string mModelConfigPath;
  std::string mModelDir;
  std::string mTempDir = "/tmp/sophon_stream_tdl";
  float mThreshold = 0.5f;
  int mVpssDev = 0;
  int mMaxBatch = 1;
  std::vector<std::string> mClassNames;
  common::FpsProfiler mFpsProfiler;
  std::mutex mTdlMutex;
};

}  // namespace tdl
}  // namespace element
}  // namespace sophon_stream

#endif  // SOPHON_STREAM_ELEMENT_TDL_H_
