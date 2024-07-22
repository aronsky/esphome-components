#pragma once

#include "esphome/components/light/light_output.h"
#include "../ble_adv_controller.h"

namespace esphome {
namespace bleadvcontroller {

class BleAdvLight : public light::LightOutput, public BleAdvEntity, public EntityBase
{
 public:
  void dump_config() override;

  void set_cold_white_temperature(float cold_white_temperature) { this->cold_white_temperature_ = cold_white_temperature; }
  void set_warm_white_temperature(float warm_white_temperature) { this->warm_white_temperature_ = warm_white_temperature; }
  void set_constant_brightness(bool constant_brightness) { this->constant_brightness_ = constant_brightness; }
  void set_min_brightness(float min_brightness) { this->min_brightness_ = min_brightness; }

  void setup_state(light::LightState *state) override { this->state_ = state; };
  void write_state(light::LightState *state) override;
  light::LightTraits get_traits() override;

 protected:
  light::LightState * state_{nullptr};

  float cold_white_temperature_;
  float warm_white_temperature_;
  bool constant_brightness_;
  float min_brightness_;

  bool is_off_{true};
  float brightness_{0};
  float color_temperature_{0};
};

} //namespace bleadvcontroller
} //namespace esphome
