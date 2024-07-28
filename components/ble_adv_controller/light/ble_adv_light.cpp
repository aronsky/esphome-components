#include "ble_adv_light.h"
#include "esphome/core/log.h"

namespace esphome {
namespace bleadvcontroller {

static const char *TAG = "ble_adv_light";

float ensure_range(float f) {
  return (f > 1.0) ? 1.0 : ( (f < 0.0) ? 0.0 : f );
}

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

  // Compute Corrected Brigtness / Warm Color Temperature (potentially reversed) as uint8_t: 0 -> 255
  uint8_t updated_br = 255 * ensure_range(this->min_brightness_ + state->current_values.get_brightness() * (1.f - this->min_brightness_));
  uint8_t updated_ct = 255 * ensure_range((state->current_values.get_color_temperature() - this->cold_white_temperature_) / (this->warm_white_temperature_ - this->cold_white_temperature_));
  updated_ct = this->get_parent()->is_reversed() ? 255 - updated_ct : updated_ct;

  // During transition(current / remote states are the same), do not process change 
  //    if Brigtness / Color Temperature was not modified enough
  bool br_modified = abs(this->brightness_ - updated_br) > 5;
  bool ct_modified = abs(this->warm_color_ - updated_ct) > 5;
  if (!br_modified && !ct_modified && (state->current_values != state->remote_values)) {
    return;
  }
  
  this->brightness_ = updated_br;
  this->warm_color_ = updated_ct;
  ESP_LOGD(TAG, "Cold: %d, Warm: %d, Brightness: %d", 255 - updated_ct, updated_ct, updated_br);

  if(this->get_parent()->is_supported(CommandType::LIGHT_WCOLOR)) {
    this->command(CommandType::LIGHT_WCOLOR, updated_ct, updated_br);
  } else {
    if (ct_modified) {
      this->command(CommandType::LIGHT_CCT, updated_ct);
    }
    if (br_modified || (this->brightness_after_color_change_ && ct_modified)) {
      this->command(CommandType::LIGHT_DIM, updated_br);
    }
  }
}

/*********************
Secondary Light
**********************/

light::LightTraits BleAdvSecLight::get_traits() {
  auto traits = light::LightTraits();
  traits.set_supported_color_modes({light::ColorMode::ON_OFF});
  return traits;
}

void BleAdvSecLight::dump_config() {
  ESP_LOGCONFIG(TAG, "BleAdvSecLight");
  BleAdvEntity::dump_config_base(TAG);
  ESP_LOGCONFIG(TAG, "  Base Light '%s'", this->state_->get_name().c_str());
}

void BleAdvSecLight::write_state(light::LightState *state) {
  bool binary;
  state->current_values_as_binary(&binary);
  if (binary) {
    ESP_LOGD(TAG, "BleAdvSecLight::write_state - Switch ON");
    this->command(CommandType::LIGHT_SEC_ON);
  } else {
    ESP_LOGD(TAG, "BleAdvSecLight::write_state - Switch OFF");
    this->command(CommandType::LIGHT_SEC_OFF);
  }
}

} // namespace bleadvcontroller
} // namespace esphome

