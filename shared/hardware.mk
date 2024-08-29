# aidl_interface
PRODUCT_PACKAGES += vendor.hardware.fancontroller

# cc_binary
PRODUCT_PACKAGES += vendor.hardware.fancontroller-service

DEVICE_FRAMEWORK_COMPATIBILITY_MATRIX_FILE := device/google/cuttlefish/shared/device_framework_matrix.xml

# android_app
PRODUCT_PACKAGES += Fancontroller

PRODUCT_ARTIFACT_PATH_REQUIREMENT_ALLOWED_LIST+= \
    system/app/Fancontroller/%