# ble_adv_light

## Known issues

* Only tested with Marpou Ceiling CCT light, and a certain aftermarket LED driver (definitely doesn't support RGB lights currently, but that could be added in the future).
* All lights are controlled at the same time (does not support controlling different lamps individually - need help with ESPHome internals to figure this one out).
* ZhiJia based lights pairing functionality hasn't been tested - if controlling a light that's been paired using the ZhiJia app doesn't work, please let me know. Unpairing will definitely not work (not supported in the app).

## How to try it

1. Create an ESPHome configuration that references the repo (using `external_components`)
2. Add a light entity to the configuration with the `ble_adv_light` platform
3. Build the configuration and flash it to an ESP32 device (since BLE is used, ESP8266-based devices are a no-go)
4. Add the new ESPHome node to your Home Assistant instance
5. Use the newly exposed service (`esphome.<esphome-node-name>_pair`) to pair with your light (call the service withing 5 seconds of powering it with a switch).
6. Enjoy controlling your BLE light with Home Assistant!

## Example configuration (ZhiJia)

```yaml
light:
  - platform: ble_adv_light
    type: zhijia
    name: Kitchen Light
    duration: 750
    default_transition_length: 0s
```

## Example configuration (LampSmart Pro)

```yaml
light:
  - platform: ble_adv_light
    type: lampsmart_pro
    name: Kitchen Light
    duration: 750
    default_transition_length: 0s
```

## Potentially fixable issues

If this component works, but the cold and warm temperatures are reversed (that is, setting the temperature in Home Assistant to warm results in cold/blue light, and setting it to cold results in warm/yellow light), add a `reversed: true` line to your `ble_adv_light` config.

If the minimum brightness is too bright, and you know that your light can go darker - try changing the minimum brightness via the `min_brightness` configuration option (it takes a number between 1 and 255).
