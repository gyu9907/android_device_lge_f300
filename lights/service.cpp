/*
 * Copyright (C) 2017 The LineageOS Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "android.hardware.light@2.0-service.vu3"

#include <hidl/HidlTransportSupport.h>
#include <utils/Errors.h>

#include "Light.h"

// libhwbinder:
using android::hardware::configureRpcThreadpool;
using android::hardware::joinRpcThreadpool;

// Generated HIDL files
using android::hardware::light::V2_0::ILight;
using android::hardware::light::V2_0::implementation::Light;

const static std::string kBacklightPath = "/sys/class/leds/lcd-backlight/brightness";
const static std::string kPatternBlinkPath = "/sys/class/leds/R/device/led_blink";
const static std::string kRedLedPath = "/sys/devices/f9967000.i2c/i2c-0/0-0032/leds/R/brightness";
const static std::string kGreenLedPath = "/sys/devices/f9967000.i2c/i2c-0/0-0032/leds/G/brightness";
const static std::string kBlueLedPath = "/sys/devices/f9967000.i2c/i2c-0/0-0032/leds/B/brightness";
const static std::string kButtonBacklight1Path = "/sys/class/leds/button-backlight1/brightness";
const static std::string kButtonBacklight2Path = "/sys/class/leds/button-backlight2/brightness";

int main() {
    std::ofstream backlight(kBacklightPath);
    if (!backlight) {
        int error = errno;
        ALOGE("Failed to open %s (%d): %s", kBacklightPath.c_str(), error, strerror(error));
        return -error;
    }

    std::ofstream blinkPattern(kPatternBlinkPath);
    if (!blinkPattern) {
        int error = errno;
        ALOGE("Failed to open %s (%d): %s", kPatternBlinkPath.c_str(), error, strerror(error));
        return -error;
    }
    
    std::ofstream redLed(kRedLedPath);
    if (!redLed) {
        int error = errno;
        ALOGE("Failed to open %s (%d): %s", kRedLedPath.c_str(), error, strerror(error));
        return -error;
    }
    
    std::ofstream greenLed(kGreenLedPath);
    if (!greenLed) {
        int error = errno;
        ALOGE("Failed to open %s (%d): %s", kGreenLedPath.c_str(), error, strerror(error));
        return -error;
    }
    
    std::ofstream blueLed(kBlueLedPath);
    if (!blueLed) {
        int error = errno;
        ALOGE("Failed to open %s (%d): %s", kBlueLedPath.c_str(), error, strerror(error));
        return -error;
    }
    
    std::ofstream buttonBacklight1(kButtonBacklight1Path);
    if (!buttonBacklight1) {
        int error = errno;
        ALOGE("Failed to open %s (%d): %s", kButtonBacklight1Path.c_str(), error, strerror(error));
        return -error;
    }
    
    std::ofstream buttonBacklight2(kButtonBacklight2Path);
    if (!buttonBacklight2) {
        int error = errno;
        ALOGE("Failed to open %s (%d): %s", kButtonBacklight2Path.c_str(), error, strerror(error));
        return -error;
    }

    android::sp<ILight> service = new Light(std::move(backlight), std::move(blinkPattern),
                                            std::move(redLed),
                                            std::move(greenLed),
                                            std::move(blueLed),
                                            std::move(buttonBacklight1),
                                            std::move(buttonBacklight2));

    configureRpcThreadpool(1, true);

    android::status_t status = service->registerAsService();

    if (status != android::OK) {
        ALOGE("Cannot register Light HAL service");
        return 1;
    }

    ALOGI("Light HAL Ready.");
    joinRpcThreadpool();
    // Under normal cases, execution will not reach this line.
    ALOGE("Light HAL failed to join thread pool.");
    return 1;
}
