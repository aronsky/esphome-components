#include "ble_adv_light.h"
#include "msc26a.h"
#include "esphome/core/log.h"

#ifdef USE_ESP32

#include <esp_gap_ble_api.h>
#include <esp_gatts_api.h>

namespace esphome {
namespace bleadvlight {

static const char *TAG = "ble_adv_light";

void BleAdvLight::setup() {
#ifdef USE_API
  register_service(&BleAdvLight::on_pair, light_state_ ? "pair_" + light_state_->get_object_id() : "pair");
  register_service(&BleAdvLight::on_unpair, light_state_ ? "unpair_" + light_state_->get_object_id() : "unpair");
#endif
}

light::LightTraits BleAdvLight::get_traits() {
  auto traits = light::LightTraits();

  traits.set_supported_color_modes({light::ColorMode::COLD_WARM_WHITE});
  traits.set_min_mireds(this->cold_white_temperature_);
  traits.set_max_mireds(this->warm_white_temperature_);

  return traits;
}

void BleAdvLight::dump_config() {
  ESP_LOGCONFIG(TAG, "BleAdvLight '%s'", light_state_ ? light_state_->get_name().c_str() : "");
  ESP_LOGCONFIG(TAG, "  Cold White Temperature: %f mireds", cold_white_temperature_);
  ESP_LOGCONFIG(TAG, "  Warm White Temperature: %f mireds", warm_white_temperature_);
  ESP_LOGCONFIG(TAG, "  Constant Brightness: %s", constant_brightness_ ? "true" : "false");
  ESP_LOGCONFIG(TAG, "  Minimum Brightness: %d", min_brightness_);
  ESP_LOGCONFIG(TAG, "  Transmission Duratoin: %d millis", tx_duration_);
}

void BleAdvLight::write_state(light::LightState *state) {
  float cwf, wwf;
  state->current_values_as_cwww(&cwf, &wwf, this->constant_brightness_);

  if (!cwf && !wwf) {
    send_packet(CMD_TURN_OFF(), 0, 0);
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

  ESP_LOGD(TAG, "LampSmartProLight::write_state called! Requested cw: %d, ww: %d", cwi, wwi);

  if (_is_off) {
    send_packet(CMD_TURN_ON(), 0, 0);
    _is_off = false;
  }

  update_channels(cwi, wwi);
}

void BleAdvLight::on_pair() {
  ESP_LOGD(TAG, "BleAdvLight::on_pair called!");
  send_packet(CMD_PAIR());
}

void BleAdvLight::on_unpair() {
  ESP_LOGD(TAG, "BleAdvLight::on_unpair called!");
  send_packet(CMD_UNPAIR());
}

} // namespace bleadvlight
} // namespace esphome

#endif
