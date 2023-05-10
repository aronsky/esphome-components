# Known issues

* Only tested with Marpou Ceiling CCT light (definitely doesn't support RGB lights currently, but that could be added in the future).
* All lights are controlled at the same time (does not support controlling different lamps individually - need help with ESPHome internals to figure this one out).

# How to try it

1. Create an ESPHome configuration that references the repo (using `external_components`)
2. Add a light entity to the configuration with the `lampsmart_pro_light` platform
3. Build the configuration and flash it to an ESP32 device (since BLE is used, ESP8266-based devices are a no-go)
4. Add the new ESPHome node to your Home Assistant instance
5. Use the newly exposed service (`esphome.<esphome-node-name>_pair`) to pair with your light (call the service withing 5 seconds of powering it with a switch.
6. Enjoy controlling your Marpou light with Home Assistant!

# Example configuration

```yaml
light:
  - platform: lampsmart_pro_light
    name: Kitchen Light
    duration: 1000
    default_transition_length: 0s
```
