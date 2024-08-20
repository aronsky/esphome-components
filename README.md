# BLE ADV ESPHome Components

Custom components for ESPHome using BLE Advertising

## Fans / Lamps controlled by BLE Advertising

Use this for various Chinese lamps that are controlled via BLE advertising packets.
Supported apps:

* LampSmart Pro, including variant v1 / v3
* FanLamp Pro, including variant v1 / v2 / v3
* Zhi Jia, including variant v0 / v1 / v2
* Other (Legacy), including variant v1a / v1b

Details can be found [here](components/ble_adv_controller/README.md).

## Credits
Based on the initial work from:
* @MasterDevX, [lampify](https://github.com/MasterDevX/lampify)
* @flicker581, [lampsmart_pro_light](https://github.com/flicker581/esphome-lampsmart)
* @aronsky, [ble_adv_light](https://github.com/aronsky/esphome-components)
* @14roiron, [zhijia encoders](https://github.com/aronsky/esphome-components/issues/11), [investigations](https://github.com/aronsky/esphome-components/issues/18)
* All testers and bug reporters from the initial threads:
  * https://community.home-assistant.io/t/controlling-ble-ceiling-light-with-ha/520612/199
  * https://github.com/aronsky/esphome-components/pull/17
