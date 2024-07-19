#pragma once

#include "ble_adv_controller.h"

namespace esphome {
namespace bleadvcontroller {

enum ZhijiaVariant : int {VARIANT_V0, VARIANT_V1, VARIANT_V2};

typedef struct {
  uint8_t cmd_{0};
  uint8_t args_[3]{0};
} ZhijiaArgs_t;

class ZhijiaController: public BleAdvController
{
public:
  virtual bool is_supported(const Command &cmd) override;
  virtual void get_adv_data(uint8_t * buf, Command &cmd) override;

protected:
  virtual ZhijiaArgs_t translate_cmd(const Command &cmd);
};

} //namespace bleadvcontroller
} //namespace esphome
