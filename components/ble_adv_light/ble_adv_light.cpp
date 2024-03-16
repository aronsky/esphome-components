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

void BleAdvLight::on_pair() {
  ESP_LOGD(TAG, "BleAdvLight::on_pair called!");
  send_packet(CMD_PAIR);
}

void BleAdvLight::on_unpair() {
  ESP_LOGD(TAG, "BleAdvLight::on_unpair called!");
  send_packet(CMD_UNPAIR);
}

} // namespace bleadvlight
} // namespace esphome

#endif
