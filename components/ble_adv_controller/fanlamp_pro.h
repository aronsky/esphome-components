#pragma once

#include "ble_adv_controller.h"

namespace esphome {
namespace bleadvcontroller {

class FanLampEncoder: public BleAdvEncoder
{
public:
  FanLampEncoder(const std::string & encoding, const std::string & variant, const std::vector<uint8_t> & prefix):
         BleAdvEncoder(encoding, variant), prefix_(prefix) {}

protected:
  virtual std::vector< Command > translate(const Command & cmd, const ControllerParam_t & cont) override;

  uint16_t get_seed(uint16_t forced_seed = 0);
  uint16_t crc16(uint8_t* buf, size_t len, uint16_t seed);

  std::vector<uint8_t> prefix_;
};

class FanLampEncoderV1: public FanLampEncoder
{
public:
  FanLampEncoderV1(const std::string & encoding, const std::string & variant,
                    uint8_t pair_arg3, bool pair_arg_only_on_pair = true, bool xor1 = false, uint8_t supp_prefix = 0x00);

protected:
  struct data_map_t {
    uint8_t command;
    uint16_t group_index;
    uint8_t args[3];
    uint8_t tx_count;
    uint8_t outs;
    uint8_t src;
    uint8_t r2;
    uint16_t seed;
    uint16_t crc16;
  }__attribute__((packed, aligned(1)));

  virtual std::vector< Command > translate(const Command & cmd, const ControllerParam_t & cont) override;
  virtual bool decode(uint8_t* buf, Command &cmd, ControllerParam_t & cont) override;
  virtual void encode(uint8_t* buf, Command &cmd, ControllerParam_t & cont) override;

  uint8_t pair_arg3_;
  bool pair_arg_only_on_pair_;
  bool with_crc2_;
  bool xor1_;
};

class FanLampEncoderV2: public FanLampEncoder
{
public:
  FanLampEncoderV2(const std::string & encoding, const std::string & variant, const std::vector<uint8_t> && prefix, uint16_t device_type, bool with_sign);

protected:
  struct data_map_t {
    uint8_t tx_count;
    uint16_t type;
    uint32_t identifier;
    uint8_t group_index;
    uint16_t command;
    uint8_t args[4];
    uint16_t sign;
    uint8_t spare;
    uint16_t seed;
    uint16_t crc16;
  }__attribute__((packed, aligned(1)));

  virtual std::vector< Command > translate(const Command & cmd, const ControllerParam_t & cont) override;
  virtual bool decode(uint8_t* buf, Command &cmd, ControllerParam_t & cont) override;
  virtual void encode(uint8_t* buf, Command &cmd, ControllerParam_t & cont) override;

  uint16_t sign(uint8_t* buf, uint8_t tx_count, uint16_t seed);
  void whiten(uint8_t *buf, uint8_t size, uint8_t seed, uint8_t salt = 0);

  uint16_t device_type_;
  bool with_sign_;
};

} //namespace bleadvcontroller
} //namespace esphome
