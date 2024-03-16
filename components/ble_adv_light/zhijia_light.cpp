#include "ble_adv_light.h"
#include "msc26a.h"

#ifdef USE_ESP32

#include <esp_gap_ble_api.h>
#include <esp_gatts_api.h>

namespace esphome {
namespace bleadvlight {

static const char *TAG = "zhijia_light";
static const unsigned char UUID1[] = {0xc6, 0x30, 0xb8};
static const unsigned char MAC[] = {0x19, 0x01, 0x10};
static const unsigned char GROUP = 127;

static esp_ble_adv_params_t ADVERTISING_PARAMS = {
  .adv_int_min = 0x20,
  .adv_int_max = 0x20,
  .adv_type = ADV_TYPE_NONCONN_IND,
  .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
  .peer_addr =
    {
      0x00,
    },
  .peer_addr_type = BLE_ADDR_TYPE_PUBLIC,
  .channel_map = ADV_CHNL_ALL,
  .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

void ZhiJiaLight::write_state(light::LightState *state) {
  float cwf, wwf;
  state->current_values_as_cwww(&cwf, &wwf, this->constant_brightness_);

  if (!cwf && !wwf) {
    send_packet(CMD_TURN_OFF);
    _is_off = true;

    return;
  }

  uint8_t cwi = (uint8_t)(0xff * cwf);
  uint8_t wwi = (uint8_t)(0xff * wwf);

  if ((cwi < min_brightness_) && (wwi < min_brightness_)) {
    if (cwf > 0.000001) {
      cwi = min_brightness_;
    }
    
    if (wwf > 0.000001) {
      wwi = min_brightness_;
    }
  }

  ESP_LOGD(TAG, "ZhiJiaLight::write_state called! Requested cw: %d, ww: %d", cwi, wwi);

  if (_is_off) {
    send_packet(CMD_TURN_ON);
    _is_off = false;
  }

  send_packet(CMD_CCT, 255 * ((float) wwi / (cwi + wwi)));
  send_packet(CMD_DIM, cwi + wwi > 255 ? 255 : cwi + wwi);
}

void ZhiJiaLight::send_packet(uint8_t cmd, uint8_t *val) {
  unsigned char args[] = {val[0], 0, 0};
  unsigned char mfg_data[0x1a] = {0};
  char mfg_data_dump[3 * sizeof(mfg_data) + 1] = {0};
  esp_ble_adv_data_t adv_data = {
    .set_scan_rsp = false,
    .include_name = false,
    .include_txpower = false,
    .min_interval = 0x0001,
    .max_interval = 0x0004,
    .appearance = 0x00,
    .manufacturer_len = sizeof(mfg_data),
    .p_manufacturer_data =  mfg_data,
    .flag = (ESP_BLE_ADV_FLAG_LIMIT_DISC | ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_DMT_CONTROLLER_SPT),
  };

  ESP_LOGD(TAG, "ZhiJiaLight::send_packet called!");
  ESP_LOGD(TAG, "  command: %02x", cmd);
  ESP_LOGD(TAG, "  args: [%02x, %02x, %02x]", args[0], args[1], args[2]);
  ESP_LOGD(TAG, "ZhiJiaLight::send_packet called! Sending mfg_data:");
  get_adv_data(mfg_data, args, cmd, ++(this->tx_count_), UUID1);
  for (int i = 0; i < sizeof(mfg_data); ++i) snprintf(mfg_data_dump + i * 3, 4, "%02x ", mfg_data[i]);
  ESP_LOGD(TAG, "  %s", mfg_data_dump);

  ESP_ERROR_CHECK_WITHOUT_ABORT(esp_ble_gap_config_adv_data(&adv_data));
  ESP_ERROR_CHECK_WITHOUT_ABORT(esp_ble_gap_start_advertising(&ADVERTISING_PARAMS));
  delay(tx_duration_);
  ESP_ERROR_CHECK_WITHOUT_ABORT(esp_ble_gap_stop_advertising());
}

} // namespace bleadvlight
} // namespace esphome

#endif
