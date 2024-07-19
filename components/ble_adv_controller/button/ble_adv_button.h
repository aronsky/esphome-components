#pragma once

#include "esphome/components/button/button.h"
#include "../ble_adv_controller.h"

namespace esphome {
namespace bleadvcontroller {

class BleAdvButton : public button::Button, public BleAdvEntity
{
 public:
  void dump_config() override;
  void press_action() override;
  void set_cmd(uint8_t cmd) { this->cmd_ = cmd; }
  void set_args(const std::vector<uint8_t> &args) { this->args_ = args; }

 protected:
  uint8_t cmd_;
  std::vector<uint8_t> args_;
};

} //namespace bleadvcontroller
} //namespace esphome
