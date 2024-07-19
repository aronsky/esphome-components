# Lev's ESPHome Components

Custom components for ESPHome

## Lamps controlled by BLE Advertising

Use this for various Chinese lamps that are controlled via BLE advertising packets.
Includes Fan and Light Control as well as pairing button and Custom service.
Supported apps:

* LampSmart Pro (tested against Marpou Ceiling Light), including variant v1a / v1b / v2 / v3
* FanLamp Pro (including Fan / multiple lights), including variant v1a / v1b / v2 / v3
* ZhiJia (including Fan / light) including variant v0, v1, v2 (only v2 Tested against aftermarket LED drivers)

Details can be found [here](components/ble_adv_controller/README.md).

## Lamps based on BLE Advertising (deprecated)

Use this for various Chinese lamps that are controlled via BLE advertising packets. Supported apps:

* LampSmart Pro (tested against Marpou Ceiling Light)
* ZhiJia (tested against aftermarket LED drivers; only the latest version is currently supported)

Details can be found [here](components/ble_adv_light/README.md).

## LampSmart Pro (deprecated)

Using this component directly is deprecated, and it will be removed in the future. Please switch to
the above component with the LampSmart Pro configuration.
Used for Marpou Ceiling Light - see details [here](components/lampsmart_pro_light/README.md).
