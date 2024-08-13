#include "ble_adv_light.h"
#include "esphome/core/log.h"

namespace esphome {
namespace bleadvcontroller {

static const char *TAG = "ble_adv_light";

float ensure_range(float f) {
  return (f > 1.0) ? 1.0 : ( (f < 0.0) ? 0.0 : f );
}

void BleAdvLight::setup() {
  // init number for Min Brightness
  this->number_min_brightness_.set_id("Min Brightness", this->get_name());
  this->number_min_brightness_.set_entity_category(EntityCategory::ENTITY_CATEGORY_CONFIG);
  this->number_min_brightness_.traits.set_min_value(0);
  this->number_min_brightness_.traits.set_max_value(100);
  this->number_min_brightness_.traits.set_step(1);
  this->number_min_brightness_.publish_state(this->number_min_brightness_.state);
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
  ESP_LOGCONFIG(TAG, "  Constant Brightness: %s", this->constant_brightness_ ? "true" : "false");
  ESP_LOGCONFIG(TAG, "  Minimum Brightness: 0x%.2X", this->get_min_brightness());
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

  // Compute Corrected Brigtness / Warm Color Temperature (potentially reversed) as float: 0 -> 1
  float updated_brf = ensure_range(this->get_min_brightness() + state->current_values.get_brightness() * (1.f - this->get_min_brightness()));
  float updated_ctf = ensure_range((state->current_values.get_color_temperature() - this->cold_white_temperature_) / (this->warm_white_temperature_ - this->cold_white_temperature_));
  updated_ctf = this->get_parent()->is_reversed() ? 1.0 - updated_ctf : updated_ctf;

  // During transition(current / remote states are not the same), do not process change 
  //    if Brigtness / Color Temperature was not modified enough
  float br_diff = abs(this->brightness_ - updated_brf) * 100;
  float ct_diff = abs(this->warm_color_ - updated_ctf) * 100;
  bool is_last = (state->current_values == state->remote_values);
  if (br_diff < 3 && ct_diff < 3 && !is_last) {
    return;
  }
  
  this->brightness_ = updated_brf;
  this->warm_color_ = updated_ctf;

  if(this->get_parent()->is_supported(CommandType::LIGHT_WCOLOR) && !this->split_dim_cct_) {
    light::LightColorValues eff_values = state->current_values;
    eff_values.set_brightness(updated_brf);
    float cwf, wwf;
    if (this->get_parent()->is_reversed()) {
      eff_values.as_cwww(&wwf, &cwf, 0, this->constant_brightness_);
    } else {
      eff_values.as_cwww(&cwf, &wwf, 0, this->constant_brightness_);
    }
    ESP_LOGD(TAG, "Updating Cold: %.0f%, Warm: %.0f%", cwf*100, wwf*100);
    this->command(CommandType::LIGHT_WCOLOR, (uint8_t) (cwf*255), (uint8_t) (wwf*255));
  } else {
    if (ct_diff != 0) {
      ESP_LOGD(TAG, "Updating warm color temperature: %.0f%", updated_ctf*100);
      this->command(CommandType::LIGHT_CCT, (uint8_t) (255*updated_ctf));
    }
    if (br_diff != 0) {
      ESP_LOGD(TAG, "Updating brightness: %.0f%", updated_brf*100);
      this->command(CommandType::LIGHT_DIM, (uint8_t) (255*updated_brf));
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

