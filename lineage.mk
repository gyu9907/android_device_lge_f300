# Inherit some common Lineage stuff.
$(call inherit-product, vendor/cm/config/common_full_phone.mk)

# Inherit device configuration
$(call inherit-product, device/lge/f300/f300.mk)

## Device identifier. This must come after all inclusions
PRODUCT_DEVICE := f300
PRODUCT_NAME := lineage_f300
PRODUCT_BRAND := LGE
PRODUCT_MODEL := LG-F300
PRODUCT_MANUFACTURER := lge

PRODUCT_BUILD_PROP_OVERRIDES += \
    BUILD_FINGERPRINT=lge/vu3_skt_kr/vu3:4.4.2/KOT49I.F300S20m/F300S20m.1442799082:user/release-keys \
    PRIVATE_BUILD_DESC="vu3_skt_kr-user 4.4.2 KOT49I.F300S20m F300S20m.1442799082 release-keys"
