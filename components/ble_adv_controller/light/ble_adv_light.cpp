#include "ble_adv_light.h"
#include "esphome/core/log.h"

namespace esphome {
namespace bleadvcontroller {

static const char *TAG = "ble_adv_light";

light::LightTraits BleAdvLight::get_traits() {
  auto traits = light::LightTraits();

  traits.set_supported_color_modes({light::ColorMode::COLD_WARM_WHITE});
  traits.set_min_mireds(this->cold_white_temperature_);
  traits.set_max_mireds(this->warm_white_temperature_);

  return traits;
}

void BleAdvLight::dump_config() {
  ESP_LOGCONFIG(TAG, "BleAdvLight");
  BleAdvEntity::dump_config_base(TAG);
  ESP_LOGCONFIG(TAG, "  Base Light '%s'", this->state_->get_name().c_str());
  ESP_LOGCONFIG(TAG, "  Cold White Temperature: %f mireds", this->cold_white_temperature_);
  ESP_LOGCONFIG(TAG, "  Warm White Temperature: %f mireds", this->warm_white_temperature_);
  ESP_LOGCONFIG(TAG, "  Constant Brightness: %s", this->constant_brightness_ ? "true" : "false");
  ESP_LOGCONFIG(TAG, "  Minimum Brightness: 0x%.2X", this->min_brightness_);
}

void BleAdvLight::write_state(light::LightState *state) {
  // If target state is off, switch off
  if (state->current_values.get_state() == 0) {
    ESP_LOGD(TAG, "BleAdvLight::write_state - Switch OFF");
    this->command(CommandType::LIGHT_OFF);
    this->is_off_ = true;
    return;
  }

  // If current state is off, switch on
  if (this->is_off_) {
    ESP_LOGD(TAG, "BleAdvLight::write_state - Switch ON");
    this->command(CommandType::LIGHT_ON);
    this->is_off_ = false;
  }

  // Correct Brightness to avoid going under min_brightness defined
  // from 0 -> 1 to min_brigthness -> 1
  float eff_bri = this->min_brightness_ + state->current_values.get_brightness() * (1.f - this->min_brightness_);
  ESP_LOGD(TAG, "Corrected brightness: %.0f%%", eff_bri*100.f);
  state->current_values.set_brightness(eff_bri);

  // Check if Brigtness / Color Temperature was modified
  bool br_modified = this->brightness_ != state->current_values.get_brightness();
  bool ct_modified = this->color_temperature_ != state->current_values.get_color_temperature();
  if (!br_modified && !ct_modified) {
    return;
  }
  
  this->brightness_ = state->current_values.get_brightness();
  this->color_temperature_ = state->current_values.get_color_temperature();

  if(this->get_parent()->is_supported(CommandType::LIGHT_WCOLOR)) {
    float cwf, wwf;
    state->current_values_as_cwww(&cwf, &wwf, this->constant_brightness_);
    // convert cold and warm from 0 -> 1 to 0 -> 255
    uint8_t cwi = (uint8_t)(0xff * cwf);
    uint8_t wwi = (uint8_t)(0xff * wwf);
    ESP_LOGD(TAG, "BleAdvLight::write_state - Requested cold: %d, warm: %d", cwi, wwi);
    this->command(CommandType::LIGHT_WCOLOR, cwi, wwi);
  } else {
    ESP_LOGD(TAG, "ct: %.3f, br: %.3f", this->color_temperature_, this->brightness_);
    // convert color temp from cold_color -> warm_color to 0 -> 255 and brigtness from 0 -> 1 to 0 -> 255
    uint8_t cti = (uint8_t)(0xff * (this->color_temperature_ - this->cold_white_temperature_) / (this->warm_white_temperature_ - this->cold_white_temperature_));
    uint8_t bri = (uint8_t)(0xff * this->brightness_);
    if (ct_modified) {
      ESP_LOGD(TAG, "BleAdvLight::write_state - Requested color temperature: %d", cti);
      this->command(CommandType::LIGHT_CCT, cti);
    }
    if (br_modified) {
      ESP_LOGD(TAG, "BleAdvLight::write_state - Requested brightness: %d", bri);
      this->command(CommandType::LIGHT_DIM, bri);
    }
  }

}

} // namespace bleadvcontroller
} // namespace esphome

