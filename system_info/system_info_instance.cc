// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "system_info/system_info_instance.h"

#include <stdlib.h>
#include <dlfcn.h>
#if defined(TIZEN_MOBILE)
#include <pkgmgr-info.h>
#include <sensors.h>
#include <system_info.h>
#endif

#include <string>
#include <utility>

#include "common/picojson.h"
#include "system_info/system_info_battery.h"
#include "system_info/system_info_build.h"
#include "system_info/system_info_cellular_network.h"
#include "system_info/system_info_cpu.h"
#include "system_info/system_info_device_orientation.h"
#include "system_info/system_info_display.h"
#include "system_info/system_info_locale.h"
#include "system_info/system_info_network.h"
#include "system_info/system_info_peripheral.h"
#include "system_info/system_info_sim.h"
#include "system_info/system_info_storage.h"
#include "system_info/system_info_utils.h"
#include "system_info/system_info_wifi_network.h"

#define MAXBUFSIZE 256
#define DUID_KEY_STRING 28
#define DUID_BUFFER_SIZE 100

namespace {
const char* DEVICE_CAPABILITIES_BLUETOOTH = "bluetooth";
const char* DEVICE_CAPABILITIES_NFC = "nfc";
const char* DEVICE_CAPABILITIES_NFC_RESERVED_PUSH = "nfcReservedPush";
const char* DEVICE_CAPABILITIES_MULTITOUCHCOUNT = "multiTouchCount";
const char* DEVICE_CAPABILITIES_INPUTKEYBOARD = "inputKeyboard";
const char* DEVICE_CAPABILITIES_INPUTKEYBOARD_LAYOUT = "inputKeyboardLayout";
const char* DEVICE_CAPABILITIES_WIFI = "wifi";
const char* DEVICE_CAPABILITIES_WIFIDIRECT = "wifiDirect";
const char* DEVICE_CAPABILITIES_OPENGLES = "opengles";
const char* DEVICE_CAPABILITIES_OPENGLES_TEXTURE_FORMAT = "openglestextureFormat";
const char* DEVICE_CAPABILITIES_OPENGLESVERSION1_1 = "openglesVersion1_1";
const char* DEVICE_CAPABILITIES_OPENGLESVERSION2_0 = "openglesVersion2_0";
const char* DEVICE_CAPABILITIES_FMRADIO = "fmRadio";
const char* DEVICE_CAPABILITIES_PLATFORMVERSION = "platformVersion";
const char* DEVICE_CAPABILITIES_PLATFORMNAME = "platformName";
const char* DEVICE_CAPABILITIES_WEBAPIVERSION = "webApiVersion";
const char* DEVICE_CAPABILITIES_NATIVEAPIVERSION = "nativeApiVersion";
const char* DEVICE_CAPABILITIES_CAMERA = "camera";
const char* DEVICE_CAPABILITIES_CAMERAFRONT = "cameraFront";
const char* DEVICE_CAPABILITIES_CAMERAFRONTFLASH = "cameraFrontFlash";
const char* DEVICE_CAPABILITIES_CAMERABACK = "cameraBack";
const char* DEVICE_CAPABILITIES_CAMERABACKFLASH = "cameraBackFlash";
const char* DEVICE_CAPABILITIES_LOCATION = "location";
const char* DEVICE_CAPABILITIES_LOCATIONGPS = "locationGps";
const char* DEVICE_CAPABILITIES_LOCATIONWPS = "locationWps";
const char* DEVICE_CAPABILITIES_MICROPHONE = "microphone";
const char* DEVICE_CAPABILITIES_USBHOST = "usbHost";
const char* DEVICE_CAPABILITIES_USBACCESSORY = "usbAccessory";
const char* DEVICE_CAPABILITIES_SCREENOUTPUTRCA = "screenOutputRca";
const char* DEVICE_CAPABILITIES_SCREENOUTPUTHDMI = "screenOutputHdmi";
const char* DEVICE_CAPABILITIES_PLATFORMCORECPUARCH = "platformCoreCpuArch";
const char* DEVICE_CAPABILITIES_PLATFORMCOREFPUARCH = "platformCoreFpuArch";
const char* DEVICE_CAPABILITIES_SIPVOIP = "sipVoip";
const char* DEVICE_CAPABILITIES_DUID = "duid";
const char* DEVICE_CAPABILITIES_SPEECH_ROCOGNITION = "speechRecognition";
const char* DEVICE_CAPABILITIES_SPEECH_SYNTHESIS = "speechSynthesis";
const char* DEVICE_CAPABILITIES_ACCELEROMETER = "accelerometer";
const char* DEVICE_CAPABILITIES_ACCELEROMETER_WAKEUP = "accelerometerWakeup";
const char* DEVICE_CAPABILITIES_BAROMETER = "barometer";
const char* DEVICE_CAPABILITIES_BAROMETER_WAKEUP = "barometerWakeup";
const char* DEVICE_CAPABILITIES_GYROSCOPE = "gyroscope";
const char* DEVICE_CAPABILITIES_GYROSCOPE_WAKEUP = "gyroscopeWakeup";
const char* DEVICE_CAPABILITIES_MAGNETOMETER = "magnetometer";
const char* DEVICE_CAPABILITIES_MAGNETOMETER_WAKEUP = "magnetometerWakeup";
const char* DEVICE_CAPABILITIES_PHOTOMETER = "photometer";
const char* DEVICE_CAPABILITIES_PHOTOMETER_WAKEUP = "photometerWakeup";
const char* DEVICE_CAPABILITIES_PROXIMITY = "proximity";
const char* DEVICE_CAPABILITIES_PROXIMITY_WAKEUP = "proximityWakeup";
const char* DEVICE_CAPABILITIES_TILTMETER = "tiltmeter";
const char* DEVICE_CAPABILITIES_TILTMETER_WAKEUP = "tiltmeterWakeup";
const char* DEVICE_CAPABILITIES_DATA_ENCRYPTION = "dataEncryption";
const char* DEVICE_CAPABILITIES_GRAPHICS_ACCELERATION = "graphicsAcceleration";
const char* DEVICE_CAPABILITIES_PUSH = "push";
const char* DEVICE_CAPABILITIES_TELEPHONY = "telephony";
const char* DEVICE_CAPABILITIES_TELEPHONY_MMS = "telephonyMms";
const char* DEVICE_CAPABILITIES_TELEPHONY_SMS = "telephonySms";
const char* DEVICE_CAPABILITIES_SCREENSIZE_NORMAL = "screenSizeNormal";
const char* DEVICE_CAPABILITIES_SCREENSIZE_480_800 = "screenSize480_800";
const char* DEVICE_CAPABILITIES_SCREENSIZE_720_1280 = "screenSize720_1280";
const char* DEVICE_CAPABILITIES_AUTO_ROTATION = "autoRotation";
const char* DEVICE_CAPABILITIES_SHELL_APP_WIDGET = "shellAppWidget";
const char* DEVICE_CAPABILITIES_VISION_IMAGE_RECOGNITION = "visionImageRecognition";
const char* DEVICE_CAPABILITIES_VISION_QRCODE_GENERATION = "visionQrcodeGeneration";
const char* DEVICE_CAPABILITIES_VISION_QRCODE_RECOGNITION = "visionQrcodeRecognition";
const char* DEVICE_CAPABILITIES_VISION_FACE_RECOGNITION = "visionFaceRecognition";
const char* DEVICE_CAPABILITIES_SECURE_ELEMENT = "secureElement";
const char* DEVICE_CAPABILITIES_NATIVE_OSP_COMPATIBLE = "nativeOspCompatible";
const char* DEVICE_CAPABILITIES_PROFILE = "profile";
}

const char* sSystemInfoFilePath = "/usr/etc/system-info.ini";

template <class T>
void SystemInfoInstance::RegisterClass() {
  classes_.insert(SysInfoClassPair(T::name_ , T::GetInstance()));
}

SystemInfoInstance::~SystemInfoInstance() {
  for (classes_iterator it = classes_.begin();
       it != classes_.end(); ++it) {
    (it->second).RemoveListener(this);
  }
}

void SystemInfoInstance::InstancesMapInitialize() {
  RegisterClass<SysInfoBattery>();
  RegisterClass<SysInfoBuild>();
  RegisterClass<SysInfoCellularNetwork>();
  RegisterClass<SysInfoCpu>();
  RegisterClass<SysInfoDeviceOrientation>();
  RegisterClass<SysInfoDisplay>();
  RegisterClass<SysInfoLocale>();
  RegisterClass<SysInfoNetwork>();
  RegisterClass<SysInfoPeripheral>();
  RegisterClass<SysInfoSim>();
  RegisterClass<SysInfoStorage>();
  RegisterClass<SysInfoWifiNetwork>();
}

void SystemInfoInstance::HandleGetPropertyValue(const picojson::value& input,
                                               picojson::value& output) {
  std::string reply_id = input.get("_reply_id").to_str();
  system_info::SetPicoJsonObjectValue(output, "_reply_id",
      picojson::value(reply_id));

  picojson::value error = picojson::value(picojson::object());
  picojson::value data = picojson::value(picojson::object());

  system_info::SetPicoJsonObjectValue(error, "message", picojson::value(""));
  std::string prop = input.get("prop").to_str();
  classes_iterator it= classes_.find(prop);

  if (it == classes_.end()) {
    system_info::SetPicoJsonObjectValue(error, "message",
        picojson::value("Property not supported: " + prop));
  } else {
    (it->second).Get(error, data);
  }

  if (!error.get("message").to_str().empty()) {
    system_info::SetPicoJsonObjectValue(output, "error", error);
  } else {
    system_info::SetPicoJsonObjectValue(output, "data", data);
  }

  std::string result = output.serialize();
  PostMessage(result.c_str());
}

void SystemInfoInstance::HandleStartListening(const picojson::value& input) {
  std::string prop = input.get("prop").to_str();
  classes_iterator it= classes_.find(prop);

  if (it != classes_.end()) {
    (it->second).AddListener(this);
  }
}

void SystemInfoInstance::HandleStopListening(const picojson::value& input) {
  std::string prop = input.get("prop").to_str();
  classes_iterator it= classes_.find(prop);

  if (it != classes_.end()) {
    (it->second).RemoveListener(this);
  }
}

void SystemInfoInstance::HandleMessage(const char* message) {
  picojson::value input;
  std::string err;

  picojson::parse(input, message, message + strlen(message), &err);
  if (!err.empty()) {
    std::cout << "Ignoring message.\n";
    return;
  }

  std::string cmd = input.get("cmd").to_str();
  if (cmd == "getPropertyValue") {
    picojson::value output = picojson::value(picojson::object());
    HandleGetPropertyValue(input, output);
  } else if (cmd == "startListening") {
    HandleStartListening(input);
  } else if (cmd == "stopListening") {
    HandleStopListening(input);
  }
}

void SystemInfoInstance::HandleSyncMessage(const char* message) {
  picojson::value v;

  std::string err;
  picojson::parse(v, message, message + strlen(message), &err);
  if (!err.empty()) {
    std::cout << "Ignoring message.\n";
    return;
  }

  std::string cmd = v.get("cmd").to_str();
  if (cmd == "getCapabilities") {
    HandleGetCapabilities();
  } else {
    std::cout << "Not supported sync api " << cmd << "().\n";
  }
}

void SystemInfoInstance::HandleGetCapabilities() {
  picojson::value::object o;

#if defined(TIZEN_MOBILE)
  bool b;
  int i;
  char* s;
  if(system_info_get_platform_bool("tizen.org/feature/network.bluetooth", &b) == SYSTEM_INFO_ERROR_NONE)
	o["bluetooth"] = picojson::value(b);

  if(system_info_get_platform_bool("tizen.org/feature/network.nfc", &b) == SYSTEM_INFO_ERROR_NONE)
    o["nfc"] = picojson::value(b);

  if(system_info_get_platform_bool("tizen.org/feature/network.nfc.reserved_push", &b) == SYSTEM_INFO_ERROR_NONE)
    o["nfcReservedPush"] = picojson::value(b);

  if(system_info_get_platform_int("tizen.org/feature/multi_point_touch.point_count", &i) == SYSTEM_INFO_ERROR_NONE)
    o["multiTouchCount"] = picojson::value(static_cast<double>(i));

  if(system_info_get_platform_bool("tizen.org/feature/input.keyboard", &b) == SYSTEM_INFO_ERROR_NONE)
    o["inputKeyboard"] = picojson::value(b);

  if(system_info_get_platform_string("tizen.org/feature/input.keyboard.layout", &s) == SYSTEM_INFO_ERROR_NONE)
    o["inputKeyboardLayout"] = picojson::value(b);

  if(system_info_get_platform_bool("tizen.org/feature/network.wifi", &b) == SYSTEM_INFO_ERROR_NONE)
    o["wifi"] = picojson::value(b);

  if(system_info_get_platform_bool("tizen.org/feature/network.wifi.direct", &b) == SYSTEM_INFO_ERROR_NONE)
    o["wifiDirect"] = picojson::value(b);

  if((system_info_get_platform_bool("tizen.org/feature/opengles", &b) == SYSTEM_INFO_ERROR_NONE) && b == true) {
    o["opengles"] = picojson::value(true);

    if((system_info_get_platform_bool("tizen.org/feature/opengles.version.1_1", &b) == SYSTEM_INFO_ERROR_NONE) && b == true)
      o["openglesVersion1_1"] = picojson::value(true);
    else
      o["openglesVersion1_1"] = picojson::value(false);

    if((system_info_get_platform_bool("tizen.org/feature/opengles.version.2_0", &b) == SYSTEM_INFO_ERROR_NONE) && b == true)
        o["openglesVersion2_0"] = picojson::value(true);
    else
      o["openglesVersion2_0"] = picojson::value(false);
  } else {
    o["opengles"] = picojson::value(false);
    o["openglesVersion1_1"] = picojson::value(false);
    o["openglesVersion2_0"] = picojson::value(false);
  }
  free(s);

  s = NULL;
  bool texture = false;
  bool data = false;
  char textureFormatFull[MAXBUFSIZE];
  textureFormatFull[0] = '\0';

  if (system_info_get_platform_bool("tizen.org/feature/opengles.texture_format.utc", &b) == SYSTEM_INFO_ERROR_NONE && b == true) {
	strcat(textureFormatFull, "utc");
	data = true;
  }
  if (system_info_get_platform_bool("tizen.org/feature/opengles.texture_format.ptc", &b) == SYSTEM_INFO_ERROR_NONE && b == true) {
	if (data)
		strcat(textureFormatFull, " | ");
	strcat(textureFormatFull, "ptc");
	data = true;
  }
  if (system_info_get_platform_bool("tizen.org/feature/opengles.texture_format.etc", &b) == SYSTEM_INFO_ERROR_NONE && b == true) {
	if (data)
		strcat(textureFormatFull, " | ");
	strcat(textureFormatFull, "etc");
	data = true;
  }
  if (system_info_get_platform_bool("tizen.org/feature/opengles.texture_format.3dc", &b) == SYSTEM_INFO_ERROR_NONE && b == true) {
	if (data) {
		strcat(textureFormatFull, " | ");
	}
	strcat(textureFormatFull, "3dc");
  }
  if (system_info_get_platform_bool("tizen.org/feature/opengles.texture_format.atc", &b) == SYSTEM_INFO_ERROR_NONE && b == true) {
	if (data)
		strcat(textureFormatFull, " | ");
	strcat(textureFormatFull, "atc");
	data = true;
  }
  if (system_info_get_platform_bool("tizen.org/feature/opengles.texture_format.pvrtc", &b) == SYSTEM_INFO_ERROR_NONE && b == true) {
	if (data)
		strcat(textureFormatFull, " | ");
	strcat(textureFormatFull, "pvrtc");
  }

  s = strdup(textureFormatFull);
  SetStringPropertyValue(o, "openglestextureFormat", s ? s : "");
  free(s);

  if(system_info_get_platform_bool("tizen.org/feature/fmradio", &b) == SYSTEM_INFO_ERROR_NONE)
    o["fmRadio"] = picojson::value(b);

  s = NULL;
  if(system_info_get_platform_string("tizen.org/feature/platform.version", &s) == SYSTEM_INFO_ERROR_NONE) {
	SetStringPropertyValue(o, "platformVersion", s ? s : "");
    free(s);
  }

  if(system_info_get_platform_string("tizen.org/feature/platform.web.api.version", &s) == SYSTEM_INFO_ERROR_NONE)
	SetStringPropertyValue(o, "webApiVersion", s);

  if(system_info_get_platform_string("tizen.org/feature/platform.native.api.version", &s) == SYSTEM_INFO_ERROR_NONE)
	SetStringPropertyValue(o, "nativeApiVersion", s);

  s = NULL;
  if(system_info_get_platform_string("tizen.org/system/platform.name", &s) == SYSTEM_INFO_ERROR_NONE) {
    SetStringPropertyValue(o, "platformName", s ? s : "");
    free(s);
  }

  if(system_info_get_platform_bool("tizen.org/feature/camera", &b) == SYSTEM_INFO_ERROR_NONE)
    o["camera"] = picojson::value(b);

  if(system_info_get_platform_bool("tizen.org/feature/camera.front", &b) == SYSTEM_INFO_ERROR_NONE)
    o["cameraFront"] = picojson::value(b);

  if(system_info_get_platform_bool("tizen.org/feature/camera.front.flash", &b) == SYSTEM_INFO_ERROR_NONE)
    o["cameraFrontFlash"] = picojson::value(b);

  if(system_info_get_platform_bool("tizen.org/feature/camera.back", &b) == SYSTEM_INFO_ERROR_NONE)
    o["cameraBack"] = picojson::value(b);

  if(system_info_get_platform_bool("tizen.org/feature/camera.back.flash", &b) == SYSTEM_INFO_ERROR_NONE)
    o["cameraBackFlash"] = picojson::value(b);

  if(system_info_get_platform_bool("tizen.org/feature/location", &b) == SYSTEM_INFO_ERROR_NONE)
    o["location"] = picojson::value(b);

  if(system_info_get_platform_bool("tizen.org/feature/location.gps", &b) == SYSTEM_INFO_ERROR_NONE)
    o["locationGps"] = picojson::value(b);

  if(system_info_get_platform_bool("tizen.org/feature/location.wps", &b) == SYSTEM_INFO_ERROR_NONE)
    o["locationWps"] = picojson::value(b);

  if(system_info_get_platform_bool("tizen.org/feature/microphone", &b) == SYSTEM_INFO_ERROR_NONE)
    o["microphone"] = picojson::value(b);

  if(system_info_get_platform_bool("tizen.org/feature/usb.host", &b) == SYSTEM_INFO_ERROR_NONE)
    o["usbHost"] = picojson::value(b);

  if(system_info_get_platform_bool("tizen.org/feature/usb.accessory", &b) == SYSTEM_INFO_ERROR_NONE)
    o["usbAccessory"] = picojson::value(b);

  if(system_info_get_platform_bool("tizen.org/feature/screen.output.rca", &b) == SYSTEM_INFO_ERROR_NONE)
    o["screenOutputRca"] = picojson::value(b);

  if(system_info_get_platform_bool("tizen.org/feature/screen.output.hdmi", &b) == SYSTEM_INFO_ERROR_NONE)
    o["screenOutputHdmi"] = picojson::value(b);

  s = NULL;



  char platformCoreCpuArchFull[MAXBUFSIZE];
  platformCoreCpuArchFull[0] = '\0';

  if (system_info_get_platform_bool("tizen.org/feature/platform.core.cpu.arch.armv6", &b) == SYSTEM_INFO_ERROR_NONE && b == true) {
	strcat(platformCoreCpuArchFull, "armv6");
	data = true;
  }
  if (system_info_get_platform_bool("tizen.org/feature/platform.core.cpu.arch.armv7", &b) == SYSTEM_INFO_ERROR_NONE && b == true) {
	if (data)
		strcat(platformCoreCpuArchFull, " | ");
	strcat(platformCoreCpuArchFull, "armv7");
	data = true;
  }
  if (system_info_get_platform_bool("tizen.org/feature/platform.core.cpu.arch.x86", &b) == SYSTEM_INFO_ERROR_NONE && b == true) {
	if (data)
		strcat(platformCoreCpuArchFull, " | ");
	strcat(platformCoreCpuArchFull, "x86");
  }

  s = strdup(platformCoreCpuArchFull);
  SetStringPropertyValue(o, "platformCoreCpuArch", s ? s : "");
  free(s);

  s = NULL;
  char platformCoreFpuArchFull[MAXBUFSIZE];
  platformCoreFpuArchFull[0] = '\0';

  if (system_info_get_platform_bool("tizen.org/feature/platform.core.fpu.arch.sse2", &b) == SYSTEM_INFO_ERROR_NONE && b == true) {
	data = true;
	strcat(platformCoreFpuArchFull, "sse2");
  }
  if (system_info_get_platform_bool("tizen.org/feature/platform.core.fpu.arch.sse3", &b) == SYSTEM_INFO_ERROR_NONE && b == true) {
	 if(data)
		strcat(platformCoreFpuArchFull, " | ");
	strcat(platformCoreFpuArchFull, "sse3");
	data = true;
  }
  if (system_info_get_platform_bool("tizen.org/feature/platform.core.fpu.arch.ssse3", &b) == SYSTEM_INFO_ERROR_NONE && b == true) {
	if(data)
		strcat(platformCoreFpuArchFull, " | ");
	strcat(platformCoreFpuArchFull, "ssse3");
	data = true;
  }
  if (system_info_get_platform_bool("tizen.org/feature/platform.core.fpu.arch.vfpv2", &b) == SYSTEM_INFO_ERROR_NONE && b == true) {
	if(data)
		strcat(platformCoreFpuArchFull, " | ");
	strcat(platformCoreFpuArchFull, "vfpv2");
	data = true;
  }
  if (system_info_get_platform_bool("tizen.org/feature/platform.core.fpu.arch.vfpv3", &b) == SYSTEM_INFO_ERROR_NONE && b == true) {
	if(data)
		strcat(platformCoreFpuArchFull, " | ");
	strcat(platformCoreFpuArchFull, "vfpv3");
  }
  s = strdup(platformCoreFpuArchFull);
  SetStringPropertyValue(o, "platformCoreFpuArch", s ? s : "");
  free(s);

  if(system_info_get_platform_bool("tizen.org/feature/sip.voip", &b) == SYSTEM_INFO_ERROR_NONE)
    o["sipVoip"] = picojson::value(b);

  s = NULL;
  FILE *fp = NULL;
  char duid[DUID_BUFFER_SIZE] = {0,};
  fp = fopen("/opt/usr/etc/system_info_cache.ini", "r");

  if(fp) {
    while(fgets(duid, DUID_BUFFER_SIZE-1, fp)) {
	  if (strncmp(duid, "http://tizen.org/system/duid", DUID_KEY_STRING) == 0) {
		char* token = NULL;
		char* ptr = NULL;
		token = strtok_r(duid, "=\r\n", &ptr);
		if (token != NULL) {
			token = strtok_r(NULL, "=\r\n", &ptr);
			if (token != NULL)
			  s = token;
		}
		break;
	  }
    }
    fclose(fp);
  }
  SetStringPropertyValue(o, "duid", s ? s : "");
  free(s);

  if(system_info_get_platform_bool("tizen.org/feature/speech.recognition", &b) == SYSTEM_INFO_ERROR_NONE)
    o["speechRecognition"] = picojson::value(b);

  if(system_info_get_platform_bool("tizen.org/feature/speech.synthesis", &b) == SYSTEM_INFO_ERROR_NONE)
    o["speechSynthesis"] = picojson::value(b);

  if(system_info_get_platform_bool("tizen.org/feature/sensor.accelerometer", &b) == SYSTEM_INFO_ERROR_NONE)
    o["accelerometer"] = picojson::value(b);

  if(system_info_get_platform_bool("tizen.org/feature/sensor.accelerometer.wakeup", &b) == SYSTEM_INFO_ERROR_NONE)
    o["accelerometerWakeup"] = picojson::value(b);

  if(system_info_get_platform_bool("tizen.org/feature/sensor.barometer", &b) == SYSTEM_INFO_ERROR_NONE)
    o["barometer"] = picojson::value(b);

  if(system_info_get_platform_bool("tizen.org/feature/sensor.barometer.wakeup", &b) == SYSTEM_INFO_ERROR_NONE)
    o["barometerWakeup"] = picojson::value(b);

  if(system_info_get_platform_bool("tizen.org/feature/sensor.gyroscope", &b) == SYSTEM_INFO_ERROR_NONE)
    o["gyroscope"] = picojson::value(b);

  if(system_info_get_platform_bool("tizen.org/feature/sensor.gyroscope.wakeup", &b) == SYSTEM_INFO_ERROR_NONE)
    o["gyroscopeWakeup"] = picojson::value(b);

  if(system_info_get_platform_bool("tizen.org/feature/sensor.magnetometer", &b) == SYSTEM_INFO_ERROR_NONE)
    o["magnetometer"] = picojson::value(b);

  if(system_info_get_platform_bool("tizen.org/feature/sensor.magnetometer.wakeup", &b) == SYSTEM_INFO_ERROR_NONE)
    o["magnetometerWakeup"] = picojson::value(b);

  if(system_info_get_platform_bool("tizen.org/feature/sensor.photometer", &b) == SYSTEM_INFO_ERROR_NONE)
    o["photometer"] = picojson::value(b);

  if(system_info_get_platform_bool("tizen.org/feature/sensor.photometer.wakeup", &b) == SYSTEM_INFO_ERROR_NONE)
    o["photometerWakeup"] = picojson::value(b);

  if(system_info_get_platform_bool("tizen.org/feature/sensor.proximity", &b) == SYSTEM_INFO_ERROR_NONE)
    o["proximity"] = picojson::value(b);

  if(system_info_get_platform_bool("tizen.org/feature/sensor.proximity.wakeup", &b) == SYSTEM_INFO_ERROR_NONE)
  o["proximityWakeup"] = picojson::value(b);

  if(system_info_get_platform_bool("tizen.org/feature/sensor.tiltmeter", &b) == SYSTEM_INFO_ERROR_NONE)
    o["tiltmeter"] = picojson::value(b);

  if(system_info_get_platform_bool("tizen.org/feature/sensor.tiltmeter.wakeup", &b) == SYSTEM_INFO_ERROR_NONE)
    o["tiltmeterWakeup"] = picojson::value(b);

  if(system_info_get_platform_bool("tizen.org/feature/database.encryption", &b) == SYSTEM_INFO_ERROR_NONE)
    o["dataEncryption"] = picojson::value(b);

  if(system_info_get_platform_bool("tizen.org/feature/graphics.acceleration", &b) == SYSTEM_INFO_ERROR_NONE)
    o["graphicsAcceleration"] = picojson::value(b);

  if(system_info_get_platform_bool("tizen.org/feature/network.push", &b) == SYSTEM_INFO_ERROR_NONE)
    o["push"] = picojson::value(b);

  if(system_info_get_platform_bool("tizen.org/feature/network.telephony", &b) == SYSTEM_INFO_ERROR_NONE)
    o["telephony"] = picojson::value(b);

  if(system_info_get_platform_bool("tizen.org/feature/network.telephony.mms", &b) == SYSTEM_INFO_ERROR_NONE)
    o["telephonyMms"] = picojson::value(b);

  if(system_info_get_platform_bool("tizen.org/feature/network.telephony.sms", &b) == SYSTEM_INFO_ERROR_NONE)
    o["telephonySms"] = picojson::value(b);

  std::string screensize_normal =
      system_info::GetPropertyFromFile(
          sSystemInfoFilePath,
          "http://tizen.org/feature/screen.coordinate_system.size.normal");
  o["screenSizeNormal"] =
      picojson::value(system_info::ParseBoolean(screensize_normal));

  int height;
  int width;
  system_info_get_value_int(SYSTEM_INFO_KEY_SCREEN_HEIGHT, &height);
  system_info_get_value_int(SYSTEM_INFO_KEY_SCREEN_WIDTH, &width);
  o["screenSize480_800"] = picojson::value(false);
  o["screenSize720_1280"] = picojson::value(false);
  if ((width == 480) && (height == 800)) {
    o["screenSize480_800"] = picojson::value(true);
  } else if ((width == 720) && (height == 1280)) {
    o["screenSize720_1280"] = picojson::value(true);
  }

  if(system_info_get_platform_bool("tizen.org/feature/screen.auto_rotation", &b) == SYSTEM_INFO_ERROR_NONE)
    o["autoRotation"] = picojson::value(b);

  pkgmgrinfo_pkginfo_h handle;
  if (pkgmgrinfo_pkginfo_get_pkginfo("gi2qxenosh", &handle) == PMINFO_R_OK)
    o["shellAppWidget"] = picojson::value(true);
  else
    o["shellAppWidget"] = picojson::value(false);

  b = system_info::PathExists("/usr/lib/osp/libarengine.so");
  o["visionImageRecognition"] = picojson::value(b);
  o["visionQrcodeGeneration"] = picojson::value(b);
  o["visionQrcodeRecognition"] = picojson::value(b);
  o["visionFaceRecognition"] = picojson::value(b);

  b = system_info::PathExists("/usr/bin/smartcard-daemon");
  o["secureElement"] = picojson::value(b);

  std::string osp_compatible =
      system_info::GetPropertyFromFile(
          sSystemInfoFilePath,
          "http://tizen.org/feature/platform.native.osp_compatible");
  o["nativeOspCompatible"] =
      picojson::value(system_info::ParseBoolean(osp_compatible));

  // FIXME(halton): Not supported until Tizen 2.2
  o["profile"] = picojson::value("MOBILE_WEB");

  o["error"] = picojson::value("");
#elif defined(GENERIC_DESKTOP)
  o["error"] = picojson::value("getCapabilities is not supported on desktop.");
#endif

  picojson::value v(o);
  SendSyncReply(v.serialize().c_str());
}
