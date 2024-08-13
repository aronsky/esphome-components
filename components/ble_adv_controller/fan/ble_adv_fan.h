#pragma once

#include "esphome/components/fan/fan.h"
#include "../ble_adv_controller.h"

namespace esphome {
namespace bleadvcontroller {

class BleAdvFan : public fan::Fan, public BleAdvEntity
{
 public:
  void dump_config() override;
  fan::FanTraits get_traits() override { return this->traits_; }
  void setup() override;
  void control(const fan::FanCall &call) override;

  void set_speed_count(uint8_t speed_count) { this->traits_.set_supported_speed_count(speed_count); this->traits_.set_speed(speed_count > 0);}
  void set_direction_supported(bool use_direction) { this->traits_.set_direction(use_direction); }
  void set_oscillation_supported(bool use_oscillation) { this->traits_.set_oscillation(use_oscillation); }

protected:
  fan::FanTraits traits_;
};

} //namespace bleadvcontroller
} //namespace esphome
