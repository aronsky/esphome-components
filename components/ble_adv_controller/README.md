# ble_adv_controller

## Goal and requirements
The goal of this component is to build a hardware proxy [ESPHome based](https://esphome.io/) in between Home Assistant and Ceiling Fans and Lamps controlled using Bluetooth Low Energy(BLE) Advertising. If your Ceiling Fan or lamp is working with one of the following Android App, then you should be able to control it:
* LampSmart Pro or FanLamp Pro (tested against Marpou Ceiling Light / IRALAN Lamp and Fan)
* ZhiJia (tested against aftermarket LED drivers)

This component is an [ESPHome external component](https://esphome.io/components/external_components.html). In order to use it you will need to have:
* A basic knowledge of [ESPHome](https://esphome.io/). A good entry point is [here](https://esphome.io/guides/getting_started_hassio.html).
* The [ESPHome integration](https://www.home-assistant.io/integrations/esphome/) in Home Assistant
* An [Espressif](https://www.espressif.com/) microcontroller supporting Bluetooth v4, such as any [ESP32](https://www.espressif.com/en/products/socs/esp32) based model. You can find some for a few dollars on any online marketplace searching for ESP32.

When the setup will be completed you will have a new ESPHome device available in Home assistant, exposing standard entities such as:
* Light(s) entity allowing the control of Color Temperature and Brightness
* Fan entity allowing the control of Speed ( 3 or 6 levels) and direction forward / reverse
* Button entity, for pairing

## Known Limitations
The technical solution implemented by manufacturers to control those devices is `BLE Advertising` and it comes with limitations:
* The communication is unidirectional meaning any change done by one of the controlling element will never be known by the other elements: for example if you switch on the light using the remote, then neither Home Assistant nor the phone App will know it and will then not show an updated state.
* Each command needs to be maintained for a given `duration` which is customizable by configuration but has drawbacks:
  * If the value is too small, the targetted device may not receive it and then not process the command
  * If the value is too high, each command is queued one after the other and then sending commands at a high rate will make the controller go wild.
  * The use of ESPHome light `transitions` is highly discouraged (and deactivated by default) as it generates high command rate.
* Some commands are the same for ON and OFF, working as a Toggle in fact. Sending high rate commands will cause the mix of ON and OFF commands and result in flickering and desynchronization of states.

## How to try it

1. As a preliminary step, be sure to be able to create a base ESPHome configuration from the ESPHome Dashboard, install it to your ESP32, have it available in Home Assistant and be able to access the logs (needed in case of issue). This is a big step if you are new to ESPHome but on top of [ESPHome doc](https://esphome.io/guides/getting_started_hassio.html) you will find tons of tutorial on the net for that.
2. Add to your up and running ESPHome configuration the reference to this repo ([ESPHome external component](https://esphome.io/components/external_components.html))
3. Add a lamp controller `ble_adv_controller` specifying:
   * its `id` to be referenced by entities it controls. The `id` is also the reference used to pair with the device: if it is changed the device needs to be re-paired with the new `id`.
   * its `encoding`, this is fully known from the controlling app, see the possible values in the examples below.
   * its `variant`, this is the version of the encoding. keep the default value (last version) as a first step.
4. Add one or several light or fan entities to the configuration with the `ble_adv_controller` platform
5. Add a `pair` configuration button to ease the pairing action from HA
6. Install and flash the ESP32 device
7. Use the newly created button to pair with your light (press the button withing 5 seconds of powering it with a switch, the same way you would have done with the phone app). If the pairing works you should be able to control your light / fan from Home Assistant. If you are sure you followed the pairing procedure but you are **NOT** able to control ityour lamp from Home Assistant it means your Lamp is not using the latest version of the encoding, and that you will have to specify another `variant`, see the possible values in the examples. Once specified, go back to step 6.
8. Enjoy controlling your BLE light with Home Assistant!

## Known issues and not implemented or tested features

* Does not support RGB lights for now, request it if needed.
* ZhiJia encoding v0 and v1 have not been tested (as no end user available to test it and help debugging) and then may not work. Contact us if you have such device and we will make it work together!
* Zhijia Fan feature has not been tested yet (same as previous).

## Example configuration: basic lamp using ZhiJia encoding v2 and Pair button

```yaml
ble_adv_controller:
  - id: my_controller
    encoding: zhijia
    duration: 200

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

## Example configuration: Composed lamp using fanlamp_pro with a main light, a secondary light, a fan and a Pair button

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
    # name: the name as it will appear in Home Assistant
    name: First Light
    # min_brightness: % minimum brightness supported by the light before it shuts done
    # just setup this value to 0, then test your lamp by decreasing the brightness percent by percent. 
    # when it switches off, you have the min_brightness to setup here.
    # Default to 21%
    min_brightness: 21%

  - platform: ble_adv_controller
    ble_adv_controller_id: my_controller
    name: Secondary Light
    # secondary: true. Qualifies this light as the secondary light to be controlled for FanLamp Lamp.
    # Exclusive with any options for brightness / cold / warm 
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