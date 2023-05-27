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

#define LOG_TAG "light"

#include "Light.h"

#include <log/log.h>

#define LCD_BRIGHTNESS_MIN 20 // Matches config_screenBrightnessSettingMinimum
#define LCD_BRIGHTNESS_MAX 255
#define LCD_BRIGHTNESS_DELTA (LCD_BRIGHTNESS_MAX - LCD_BRIGHTNESS_MIN)

//#define KPDBL_ID_OFF  "0"
//#define KPDBL_ID_NOTI "6"

namespace {
using android::hardware::light::V2_0::LightState;

static uint32_t rgbToBrightness(const LightState& state) {
    uint32_t color = state.color & 0x00ffffff;
    return ((77 * ((color >> 16) & 0xff)) + (150 * ((color >> 8) & 0xff)) +
            (29 * (color & 0xff))) >> 8;
}

static bool isLit(const LightState& state) {
    return (state.color & 0x00ffffff);
}

static uint32_t applyGamma(const uint32_t brightness){
    if(brightness < LCD_BRIGHTNESS_MIN)
        return 0;

    return LCD_BRIGHTNESS_MIN + LCD_BRIGHTNESS_DELTA *
        cbrt(((double)brightness - LCD_BRIGHTNESS_MIN)/LCD_BRIGHTNESS_DELTA);
}
} // anonymous namespace

namespace android {
namespace hardware {
namespace light {
namespace V2_0 {
namespace implementation {

Light::Light(std::ofstream&& backlight, std::ofstream&& blinkPattern,
             std::ofstream&& redLed,
             std::ofstream&& greenLed,
             std::ofstream&& blueLed,
             std::ofstream&& buttonBacklight1,
             std::ofstream&& buttonBacklight2) :
    mBacklight(std::move(backlight)),
    mBlinkPattern(std::move(blinkPattern)),
    mRedLed(std::move(redLed)),
    mGreenLed(std::move(greenLed)),
    mBlueLed(std::move(blueLed)),
    mButtonBacklight1(std::move(buttonBacklight1)),
    mButtonBacklight2(std::move(buttonBacklight2)) {
    auto attnFn(std::bind(&Light::setAttentionLight, this, std::placeholders::_1));
    auto backlightFn(std::bind(&Light::setBacklight, this, std::placeholders::_1));
    auto buttonsFn(std::bind(&Light::setButtonsBacklight, this, std::placeholders::_1));
    auto batteryFn(std::bind(&Light::setBatteryLight, this, std::placeholders::_1));
    auto notifFn(std::bind(&Light::setNotificationLight, this, std::placeholders::_1));
    mLights.emplace(std::make_pair(Type::ATTENTION, attnFn));
    mLights.emplace(std::make_pair(Type::BACKLIGHT, backlightFn));
    mLights.emplace(std::make_pair(Type::BUTTONS, buttonsFn));
    mLights.emplace(std::make_pair(Type::BATTERY, batteryFn));
    mLights.emplace(std::make_pair(Type::NOTIFICATIONS, notifFn));
}

// Methods from ::android::hardware::light::V2_0::ILight follow.
Return<Status> Light::setLight(Type type, const LightState& state) {
    auto it = mLights.find(type);
    if (it == mLights.end()) {
        return Status::LIGHT_NOT_SUPPORTED;
    }
    it->second(state);
    return Status::SUCCESS;
}

Return<void> Light::getSupportedTypes(getSupportedTypes_cb _hidl_cb) {
    std::vector<Type> types;
    for (auto const& light : mLights) {
        types.push_back(light.first);
    }
    _hidl_cb(types);
    return Void();
}

void Light::setAttentionLight(const LightState& state) {
    std::lock_guard<std::mutex> lock(mLock);
    mAttentionState = state;
    setSpeakerBatteryLightLocked();
}

void Light::setBacklight(const LightState& state) {
    std::lock_guard<std::mutex> lock(mLock);
    uint32_t brightness = rgbToBrightness(state);
    brightness = applyGamma(brightness);

    // Prevent backlight from being turned off completely
    if(brightness == 0) {
        brightness = LCD_BRIGHTNESS_MIN;
    }

    mBacklight << brightness << std::endl;
}

void Light::setButtonsBacklight(const LightState& state) {
    std::lock_guard<std::mutex> lock(mLock);
    bool on = isLit(state);
    int brightness = on?80:0;
    mButtonBacklight1 << brightness << std::endl;
    mButtonBacklight2 << brightness << std::endl;
}

void Light::setBatteryLight(const LightState& state) {
    std::lock_guard<std::mutex> lock(mLock);
    mBatteryState = state;
    setSpeakerBatteryLightLocked();
}

void Light::setNotificationLight(const LightState& state) {
    std::lock_guard<std::mutex> lock(mLock);
    mNotificationState = state;
    setSpeakerBatteryLightLocked();
}

void Light::setSpeakerBatteryLightLocked() {
    if (isLit(mNotificationState)) {
        setSpeakerLightLocked(mNotificationState);
    } else if (isLit(mAttentionState)) {
        setSpeakerLightLocked(mAttentionState);
    } else if (isLit(mBatteryState)) {
        setSpeakerLightLocked(mBatteryState);
    } else {
        /* Lights off */
        mBlinkPattern << "0x0,0,0" << std::endl;
        mRedLed << 0 << std::endl;
        mGreenLed << 0 << std::endl;
        mBlueLed << 0 << std::endl;
    }
}

void Light::setSpeakerLightLocked(const LightState& state) {
    int onMS, offMS;
    uint32_t color;
    char blink_pattern[PAGE_SIZE];

    switch (state.flashMode) {
        case Flash::TIMED:
            onMS = state.flashOnMs;
            offMS = state.flashOffMs;
            break;
        case Flash::NONE:
        default:
            onMS = 0;
            offMS = 0;
            break;
    }

    color = state.color & 0x00ffffff;
    
    if (onMS > 0 && offMS > 0) {
            mBlinkPattern << 0 << std::endl;
            mRedLed << 0 << std::endl;
            mGreenLed << 0 << std::endl;
            mBlueLed << 0 << std::endl;
            sprintf(blink_pattern,"0x%x,%d,%d",color,onMS,offMS);

            mBlinkPattern << blink_pattern << std::endl;
        } else {
            int red, green, blue;
            red = (color >> 16) & 0xFF;
            green = (color >> 8) & 0xFF;
            blue = color & 0xFF;

            mBlinkPattern << 0 << std::endl;
            mRedLed << red << std::endl;
            mGreenLed << green << std::endl;
            mBlueLed << blue << std::endl;
        }
}

}  // namespace implementation
}  // namespace V2_0
}  // namespace light
}  // namespace hardware
}  // namespace android
