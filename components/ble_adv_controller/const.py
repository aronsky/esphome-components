CONF_BLE_ADV_CONTROLLER_ID = "ble_adv_controller_id"
CONF_BLE_ADV_CMD = "cmd"
CONF_BLE_ADV_ARGS = "args"
CONF_BLE_ADV_NB_ARGS = "nb_args"
CONF_BLE_ADV_ENCODING = "encoding"
CONF_BLE_ADV_COMMANDS = {
  "pair" : {CONF_BLE_ADV_CMD: 1, CONF_BLE_ADV_NB_ARGS : 0},
  "unpair" : {CONF_BLE_ADV_CMD: 2, CONF_BLE_ADV_NB_ARGS : 0},
  "custom" : {CONF_BLE_ADV_CMD: 3, CONF_BLE_ADV_NB_ARGS : 5},
  "light_on" : {CONF_BLE_ADV_CMD: 13, CONF_BLE_ADV_NB_ARGS : 0},
  "light_off" : {CONF_BLE_ADV_CMD: 14, CONF_BLE_ADV_NB_ARGS : 0},
  "light_dim" : {CONF_BLE_ADV_CMD: 15, CONF_BLE_ADV_NB_ARGS : 1},
  "light_cct" : {CONF_BLE_ADV_CMD: 16, CONF_BLE_ADV_NB_ARGS : 1},
  "light_wcolor" : {CONF_BLE_ADV_CMD: 17, CONF_BLE_ADV_NB_ARGS : 2},
  "light_sec_on" : {CONF_BLE_ADV_CMD: 18, CONF_BLE_ADV_NB_ARGS : 0},
  "light_sec_off" : {CONF_BLE_ADV_CMD: 19, CONF_BLE_ADV_NB_ARGS : 0},
  "fan_on" : {CONF_BLE_ADV_CMD: 30, CONF_BLE_ADV_NB_ARGS : 0},
  "fan_off" : {CONF_BLE_ADV_CMD: 31, CONF_BLE_ADV_NB_ARGS : 0},
  "fan_speed" : {CONF_BLE_ADV_CMD: 32, CONF_BLE_ADV_NB_ARGS : 1},
  "fan_onoff_speed" : {CONF_BLE_ADV_CMD: 33, CONF_BLE_ADV_NB_ARGS : 2},
  "fan_dir" : {CONF_BLE_ADV_CMD: 34, CONF_BLE_ADV_NB_ARGS : 1},
  "fan_osc" : {CONF_BLE_ADV_CMD: 35, CONF_BLE_ADV_NB_ARGS : 1},
}
CONF_BLE_ADV_SPEED_COUNT = "speed_count"
CONF_BLE_ADV_DIRECTION_SUPPORTED = "use_direction"
CONF_BLE_ADV_OSCILLATION_SUPPORTED = "use_oscillation"
CONF_BLE_ADV_FORCED_ID = "forced_id"
CONF_BLE_ADV_SHOW_CONFIG = "show_config"
CONF_BLE_ADV_SECONDARY = "secondary"
CONF_BLE_ADV_MAX_DURATION = "max_duration"
CONF_BLE_ADV_SEQ_DURATION = "seq_duration"
CONF_BRIGHTNESS_AFTER_COLOR_CHANGE = "send_brightness_after_color_temperature_change"