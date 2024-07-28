#pragma once

#include "ble_adv_controller.h"

namespace esphome {
namespace bleadvcontroller {

enum FanLampVariant : int {VARIANT_3, VARIANT_2, VARIANT_1A, VARIANT_1B};

typedef struct {
  uint8_t cmd_{0};
  uint8_t args_[4]{0};
} FanLampArgs;

class FanLampEncoder: public BleAdvEncoder
{
public:
  FanLampEncoder(const std::string & name, const std::string & encoding, FanLampVariant variant):
         BleAdvEncoder(name, encoding, variant) {}

  virtual bool is_supported(const Command &cmd) override;
  virtual uint8_t get_adv_data(std::vector< BleAdvParam > & params, Command &cmd) override;
  static void register_encoders(BleAdvHandler * handler, const std::string & encoding);

protected:
  virtual FanLampArgs translate_cmd(const Command &cmd);
  uint16_t get_seed();

  void build_packet_v1a(uint8_t* buf, Command &cmd);
  void build_packet_v1b(uint8_t* buf, Command &cmd);
  void build_packet_v1(uint8_t* buf, Command &cmd);
  void build_packet_v2(uint8_t* buf, Command &cmd, bool with_sign);
};

} //namespace bleadvcontroller
} //namespace esphome
