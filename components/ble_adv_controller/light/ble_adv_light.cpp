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
  float cwf, wwf;
  state->current_values_as_cwww(&cwf, &wwf, this->constant_brightness_);

  if (!cwf && !wwf) {
    this->command(CommandType::LIGHT_OFF);
    this->is_off_ = true;

    return;
  }

  uint8_t cwi = (uint8_t)(0xff * cwf);
  uint8_t wwi = (uint8_t)(0xff * wwf);

  if ((cwi < this->min_brightness_) && (wwi < this->min_brightness_)) {
    if (cwf > 0.000001) {
      cwi = this->min_brightness_;
    }
    
    if (wwf > 0.000001) {
      wwi = this->min_brightness_;
    }
  }

  ESP_LOGD(TAG, "BleAdvLight::write_state called! Requested cw: %d, ww: %d", cwi, wwi);

  if (this->is_off_) {
    this->command(CommandType::LIGHT_ON);
    this->is_off_ = false;
  }

  if(this->get_parent()->is_supported(CommandType::LIGHT_WCOLOR)) {
    this->command(CommandType::LIGHT_WCOLOR, cwi, wwi);
  } else {
    uint8_t cct_arg = (uint8_t) (255 * ((float) wwi / (cwi + wwi)));
    uint8_t dim_arg = (uint8_t) (cwi + wwi > 255 ? 255 : cwi + wwi);
    this->command(CommandType::LIGHT_CCT, cct_arg);
    this->command(CommandType::LIGHT_DIM, dim_arg);
  }
  
}

} // namespace bleadvcontroller
} // namespace esphome

