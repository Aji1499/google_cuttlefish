#
# Copyright (C) 2022 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

$(call inherit-product, $(SRC_TARGET_DIR)/product/core_64_bit_only.mk)
$(call inherit-product, device/google/cuttlefish/shared/minidroid/device.mk)

$(call inherit-product, device/google/cuttlefish/vsoc_riscv64/bootloader.mk)

PRODUCT_NAME := aosp_cf_riscv64_minidroid
PRODUCT_DEVICE := vsoc_riscv64_minidroid
PRODUCT_MANUFACTURER := Google
PRODUCT_MODEL := Cuttlefish riscv64 minidroid

PRODUCT_ENFORCE_MAC80211_HWSIM := false

PRODUCT_VENDOR_PROPERTIES += \
    ro.soc.manufacturer=$(PRODUCT_MANUFACTURER) \
    ro.soc.model=$(PRODUCT_DEVICE)
