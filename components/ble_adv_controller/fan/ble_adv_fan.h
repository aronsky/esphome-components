#pragma once

#include "esphome/components/fan/fan.h"
#include "../ble_adv_controller.h"

namespace esphome {
namespace bleadvcontroller {

class BleAdvFan : public fan::Fan, public BleAdvEntity
{
 public:
  void dump_config() override;
  fan::FanTraits get_traits() override;
  void control(const fan::FanCall &call) override;

  void set_speed_count(uint8_t speed_count) { this->speed_count_ = speed_count; }
  void set_direction_supported(bool use_direction) { this->use_direction_ = use_direction; }

protected:
  uint8_t speed_count_;
  uint8_t use_direction_;
};

} //namespace bleadvcontroller
} //namespace esphome
