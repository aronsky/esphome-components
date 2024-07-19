#pragma once

#include "ble_adv_controller.h"

namespace esphome {
namespace bleadvcontroller {

enum FanLampVariant : int {VARIANT_3, VARIANT_2, VARIANT_1A, VARIANT_1B};

typedef struct {
  uint8_t cmd_;
  uint8_t args_[4];
} FanLampArgs;

class FanLampController: public BleAdvController
{
public:
  void set_reversed(bool reversed) { this->reversed_ = reversed; }
  virtual bool is_supported(const Command &cmd) override;
  virtual void get_adv_data(uint8_t * buf, Command &cmd) override;

protected:
  virtual FanLampArgs translate_cmd(const Command &cmd);
  uint16_t get_seed();

  void build_packet_v1a(uint8_t* buf, Command &cmd);
  void build_packet_v1b(uint8_t* buf, Command &cmd);
  void build_packet_v1(uint8_t* buf, Command &cmd);
  void build_packet_v2(uint8_t* buf, Command &cmd, bool with_sign);

  bool reversed_;
};

} //namespace bleadvcontroller
} //namespace esphome
