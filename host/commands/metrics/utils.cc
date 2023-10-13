//
// Copyright (C) 2022 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <android-base/logging.h>
#include <android-base/strings.h>
#include <curl/curl.h>
#include <gflags/gflags.h>
#include <json/json.h>
#include <net/if.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/utsname.h>
#include <chrono>
#include <ctime>
#include <iostream>

#include "common/libs/utils/tee_logging.h"
#include "host/commands/metrics/metrics_defs.h"
#include "host/commands/metrics/proto/cf_metrics_proto.h"
#include "host/commands/metrics/utils.h"
#include "host/libs/config/cuttlefish_config.h"
#include "host/libs/vm_manager/crosvm_manager.h"
#include "host/libs/vm_manager/qemu_manager.h"

using cuttlefish::MetricsExitCodes;

namespace metrics {

static std::string Hashing(const std::string& input) {
  const std::hash<std::string> hasher;
  return std::to_string(hasher(input));
}

std::string GetOsName() {
  struct utsname buf;
  if (uname(&buf) != 0) {
    LOG(ERROR) << "failed to retrieve system information";
    return "Error";
  }
  return std::string(buf.sysname);
}

cuttlefish::MetricsEvent::OsType GetOsType() {
  struct utsname buf;
  if (uname(&buf) != 0) {
    LOG(ERROR) << "failed to retrieve system information";
    return cuttlefish::MetricsEvent::CUTTLEFISH_OS_TYPE_UNSPECIFIED;
  }
  std::string sysname(buf.sysname);
  std::string machine(buf.machine);

  if (sysname != "Linux") {
    return cuttlefish::MetricsEvent::CUTTLEFISH_OS_TYPE_UNSPECIFIED;
  }
  if (machine == "x86_64") {
    return cuttlefish::MetricsEvent::CUTTLEFISH_OS_TYPE_LINUX_X86_64;
  }
  if (machine == "x86") {
    return cuttlefish::MetricsEvent::CUTTLEFISH_OS_TYPE_LINUX_X86;
  }
  if (machine == "aarch64" || machine == "arm64") {
    return cuttlefish::MetricsEvent::CUTTLEFISH_OS_TYPE_LINUX_AARCH64;
  }
  if (machine[0] == 'a') {
    return cuttlefish::MetricsEvent::CUTTLEFISH_OS_TYPE_LINUX_AARCH32;
  }
  return cuttlefish::MetricsEvent::CUTTLEFISH_OS_TYPE_UNSPECIFIED;
}

std::string GenerateSessionId(uint64_t now_ms) {
  uint64_t now_day = now_ms / 1000 / 60 / 60 / 24;
  return Hashing(GetMacAddress() + std::to_string(now_day));
}

std::string GetCfVersion() {
  // TODO: per ellisr@ leave empty for now
  return "";
}

std::string GetOsVersion() {
  struct utsname buf;
  if (uname(&buf) != 0) {
    LOG(ERROR) << "failed to retrieve system information";
  }
  std::string version = buf.release;
  return version;
}

std::string GetMacAddress() {
  int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
  if (sock == -1) {
    LOG(ERROR) << "couldn't connect to socket";
    return "";
  }

  char buf2[1024];
  struct ifconf ifc;
  ifc.ifc_len = sizeof(buf2);
  ifc.ifc_buf = buf2;
  if (ioctl(sock, SIOCGIFCONF, &ifc) == -1) {
    LOG(ERROR) << "couldn't connect to socket";
    return "";
  }

  struct ifreq* it = ifc.ifc_req;
  const struct ifreq* const end = it + (ifc.ifc_len / sizeof(struct ifreq));

  unsigned char mac_address[6] = {0};
  struct ifreq ifr;
  for (; it != end; ++it) {
    strcpy(ifr.ifr_name, it->ifr_name);
    if (ioctl(sock, SIOCGIFFLAGS, &ifr) != 0) {
      LOG(ERROR) << "couldn't connect to socket";
      return "";
    }
    if (ifr.ifr_flags & IFF_LOOPBACK) {
      continue;
    }
    if (ioctl(sock, SIOCGIFHWADDR, &ifr) == 0) {
      memcpy(mac_address, ifr.ifr_hwaddr.sa_data, 6);
      break;
    }
  }

  char mac[100];
  sprintf(mac, "%02x:%02x:%02x:%02x:%02x:%02x", mac_address[0], mac_address[1],
          mac_address[2], mac_address[3], mac_address[4], mac_address[5]);
  return mac;
}

std::string GetCompany() {
  // TODO: per ellisr@ leave hard-coded for now
  return "GOOGLE";
}

cuttlefish::MetricsEvent::VmmType GetVmmManager() {
  auto config = cuttlefish::CuttlefishConfig::Get();
  CHECK(config) << "Could not open cuttlefish config";
  auto vmm = config->vm_manager();
  if (vmm == cuttlefish::vm_manager::CrosvmManager::name()) {
    return cuttlefish::MetricsEvent::CUTTLEFISH_VMM_TYPE_CROSVM;
  }
  if (vmm == cuttlefish::vm_manager::QemuManager::name()) {
    return cuttlefish::MetricsEvent::CUTTLEFISH_VMM_TYPE_QEMU;
  }
  return cuttlefish::MetricsEvent::CUTTLEFISH_VMM_TYPE_UNSPECIFIED;
}

std::string GetVmmVersion() {
  // TODO: per ellisr@ leave empty for now
  return "";
}

uint64_t GetEpochTimeMs() {
  auto now = std::chrono::system_clock::now().time_since_epoch();
  uint64_t milliseconds_since_epoch =
      std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
  return milliseconds_since_epoch;
}

// TODO (moelsherif@): remove this function in the future since it is not used
cuttlefish::CuttlefishLogEvent* sampleEvent() {
  cuttlefish::CuttlefishLogEvent* event = new cuttlefish::CuttlefishLogEvent();
  event->set_device_type(
      cuttlefish::CuttlefishLogEvent::CUTTLEFISH_DEVICE_TYPE_HOST);
  return event;
}

// TODO (moelsherif@): remove this function in the future since it is not used
std::string ProtoToString(LogEvent* event) {
  std::string output;
  if (!event->SerializeToString(&output)) {
    LOG(ERROR) << "failed to serialize proto LogEvent";
  }
  return output;
}

size_t curl_out_writer([[maybe_unused]] char* response, size_t size,
                       size_t nmemb, [[maybe_unused]] void* userdata) {
  return size * nmemb;
}

CURLUcode SetCurlUrlPart(CURLU* url, CURLUPart part, const char* value) {
  CURLUcode urc = curl_url_set(url, part, value, 0);
  if (urc != 0) {
    LOG(ERROR) << "Failed to set url part '" << part << "' to '" << value
               << "': Error '" << curl_url_strerror(urc) << "'";
  }
  return urc;
}

std::string ClearcutServerUrl(metrics::ClearcutServer server) {
  switch (server) {
    case metrics::kLocal:
      return "http://localhost:27910/log";

    case metrics::kStaging:
      return "https://play.googleapis.com:443/staging/log";

    case metrics::kProd:
      return "https://play.googleapis.com:443/log";

    default:
      LOG(FATAL) << "Invalid host configuration";
      return "";
  }
}

MetricsExitCodes PostRequest(const std::string& output,
                             metrics::ClearcutServer server) {
  std::string clearcut_url = ClearcutServerUrl(server);

  std::unique_ptr<CURLU, void (*)(CURLU*)> url(curl_url(), curl_url_cleanup);
  if (!url) {
    LOG(ERROR) << "Failed to initialize CURLU.";
    return cuttlefish::kMetricsError;
  }

  CURLUcode urc =
      curl_url_set(url.get(), CURLUPART_URL, clearcut_url.c_str(), 0);
  if (urc != 0) {
    LOG(ERROR) << "Failed to set url to " << url.get() << clearcut_url
               << "': " << curl_url_strerror(urc) << "'";
    return cuttlefish::kMetricsError;
  }
  curl_global_init(CURL_GLOBAL_ALL);

  std::unique_ptr<CURL, void (*)(CURL*)> curl(curl_easy_init(),
                                              curl_easy_cleanup);

  if (!curl) {
    LOG(ERROR) << "Failed to initialize CURL.";
    return cuttlefish::kMetricsError;
  }

  curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, &curl_out_writer);
  curl_easy_setopt(curl.get(), CURLOPT_SSL_VERIFYPEER, 0L);
  curl_easy_setopt(curl.get(), CURLOPT_CURLU, url.get());
  curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDS, output.data());
  curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDSIZE, output.size());
  CURLcode rc = curl_easy_perform(curl.get());
  long http_code = 0;
  curl_easy_getinfo(curl.get(), CURLINFO_RESPONSE_CODE, &http_code);

  if (rc == CURLE_ABORTED_BY_CALLBACK || http_code != 200) {
    LOG(ERROR) << "Metrics message failed: [" << output << "]";
    LOG(ERROR) << "http error code: " << http_code;
    LOG(ERROR) << "curl error code: " << rc << " | " << curl_easy_strerror(rc);
    return cuttlefish::kMetricsError;
  }
  LOG(INFO) << "Metrics posted to ClearCut";
  curl_global_cleanup();
  return cuttlefish::kSuccess;
}
}  // namespace metrics
