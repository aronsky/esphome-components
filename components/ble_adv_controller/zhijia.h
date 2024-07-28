#pragma once

#include "ble_adv_controller.h"

namespace esphome {
namespace bleadvcontroller {

enum ZhijiaVariant : int {VARIANT_V0, VARIANT_V1, VARIANT_V2};

typedef struct {
  uint8_t cmd_{0};
  uint8_t args_[3]{0};
} ZhijiaArgs_t;

class ZhijiaEncoder: public BleAdvEncoder
{
public:
  ZhijiaEncoder(const std::string & name, const std::string & encoding, ZhijiaVariant variant): 
        BleAdvEncoder(name, encoding, variant) {}
  virtual bool is_supported(const Command &cmd) override;
  virtual uint8_t get_adv_data(std::vector< BleAdvParam > & params, Command &cmd) override;
  
  static void register_encoders(BleAdvHandler * handler, const std::string & encoding);

protected:
  virtual ZhijiaArgs_t translate_cmd(const Command &cmd);
};

} //namespace bleadvcontroller
} //namespace esphome
