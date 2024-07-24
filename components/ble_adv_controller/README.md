# ble_adv_controller

## Known issues

* Only tested with Marpou Ceiling CCT light, and a certain aftermarket LED driver (definitely doesn't support RGB lights currently, but that could be added in the future).
* ZhiJia based lights pairing functionality hasn't been tested - if controlling a light that's been paired using the ZhiJia app doesn't work, please let me know. Unpairing will definitely not work (not supported in the app).

## How to try it

1. Create an ESPHome configuration that references the repo (using `external_components`)
2. Add a lamp controller `ble_adv_controller` specifying its ID to be referenced by entities it controls. The ID is the referenced used to pair with the device: if it is changed the device needs to be re-paired with the new ID.
3. Add one or several light or fan entities to the configuration with the `ble_adv_controller` platform
4. Add a `pair` configuration button to ease the pairing action from HA
5. Build the configuration and flash it to an ESP32 device (since BLE is used, ESP8266-based devices are a no-go)
6. Add the new ESPHome node to your Home Assistant instance
7. Use the newly created button to pair with your light (press the button withing 5 seconds of powering it with a switch).
6. Enjoy controlling your BLE light with Home Assistant!

## Example configuration: basic lamp using ZhiJia encoding v2 and Pair button

```yaml
ble_adv_controller:
  - id: my_controller
    encoding: zhijia
    duration: 500

light:
  - platform: ble_adv_controller
    ble_adv_controller_id: my_controller
    name: Kitchen Light

button:
  - platform: ble_adv_controller
    ble_adv_controller_id: my_controller
    name: Pair
    cmd: pair
```

## Example configuration: Complex composed lamp using fanlamp_pro with 2 lights, a fan and a Pair button

```yaml
ble_adv_controller:
  # A controller per device, or per remote in fact as it has the same role
  - id: my_controller
    # encoding: could be any of 'zhijia', 'fanlamp_pro', 'smartlamp_pro' (the 2 last are the same)
    encoding: fanlamp_pro
    # variant: variant of the encoding 
    # For ZhiJia: Can be v0 (MSC16), v1 (MSC26) or v2 (MSC26A), default is v2
    # For Fanlamp: Can be any of 'v1a', 'v1b', 'v2' or 'v3', depending on how old your lamp is... Default is 'v3'
    variant: v3
    # duration: the duration during which the command is sent. 
    # Increasing this parameter will make the combination of commands slower, 
    # but it may be needed if your light is taking time to process a command
    duration: 200
    # reversed: reversing the cold / warm at encoding time, needed for some controllers
    # default to false
    reversed: false
    # forced_id: provide the 4 bytes identifier key extracted from your app phone traffic 
    # to share the same key than the phone
    # example: 0xBFF62757
    # For ZhiJia, default to 0xC630B8 which was the value hard-coded in ble_adv_light component. Max 0xFFFFFF.
    # For FanLamp: default to 0, uses the hash id computed by esphome from the id/name of the controller
    forced_id: 0

light:
  - platform: ble_adv_controller
    # ble_adv_controller_id: the ID of your controller
    ble_adv_controller_id: my_controller
    # name: the name as it will appear in 1A
    name: First Light
    # min_brightness: % minimum brightness supported by the light before it shuts done
    # just setup this value to 0, then test your lamp by decreasing the brightness percent by percent. 
    # when it switches off, you have the min_brightness to setup here.
    # Default to 21%
    min_brightness: 21%

  - platform: ble_adv_controller
    ble_adv_controller_id: my_controller
    name: Secondary Light
    # secondary: true. Qualifies this light as the secondary light to be controlled for FanLamp Lamp
    # exclusive with any options for brightness / cold / warm 
    secondary: true

fan:
  - platform: ble_adv_controller
    ble_adv_controller_id: my_controller
    name: my fan
    # speed_count: the number of speed level available on your remote / app. Can be 0 / 3 / 6.
    # if not properly setup the remote and this component does not behave properly together
    # only speed 6 is available for zhijia, and this is the default
    speed_count: 6
    # use_direction: ability to change the fan direction forward / reverse.
    # default to true, not available for zhijia
    use_direction: true

button:
  - platform: ble_adv_controller
    ble_adv_controller_id: my_controller
    name: Pair
    # cmd: the action to be executed when the button is pressed
    # any of 'pair', 'unpair', 'custom', 'light_on', ...
    cmd: pair
```

## Potentially fixable issues

If this component works, but the cold and warm temperatures are reversed (that is, setting the temperature in Home Assistant to warm results in cold/blue light, and setting it to cold results in warm/yellow light), add a `reversed: true` line to your `ble_adv_controller` config.

If the minimum brightness is too bright, and you know that your light can go darker - try changing the minimum brightness via the `min_brightness` configuration option (it takes a percentage).

## For the very tecki ones

If you want to discover new features for your lamp and that you are able to understand the code of this component as well as the code of the applications that generate commands, you can try to send custom commands, details [here](CUSTOM.md). 