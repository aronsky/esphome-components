#pragma once

#include "ble_adv_controller.h"

namespace esphome {
namespace bleadvcontroller {

class ZhijiaEncoder: public BleAdvEncoder
{
public:
  ZhijiaEncoder(const std::string & encoding, const std::string & variant): BleAdvEncoder(encoding, variant) {}
  
protected:
  uint16_t crc16(uint8_t* buf, size_t len, uint16_t seed = 0);
  uint32_t uuid_to_id(uint8_t * uuid, size_t len);
  void id_to_uuid(uint8_t * uuid, uint32_t id, size_t len);
};

class ZhijiaEncoderV0: public ZhijiaEncoder
{
public:
  ZhijiaEncoderV0(const std::string & encoding, const std::string & variant): 
          ZhijiaEncoder(encoding, variant) { { this->len_ = sizeof(data_map_t); }}
  
protected:
  static constexpr size_t UUID_LEN = 2;
  static constexpr size_t ADDR_LEN = 3;
  static constexpr size_t TXDATA_LEN = 8;
  struct data_map_t {
    uint8_t addr[ADDR_LEN];
    uint8_t txdata[TXDATA_LEN];
    uint16_t crc16;
  }__attribute__((packed, aligned(1)));

  virtual std::vector< Command > translate(const Command & cmd, const ControllerParam_t & cont) override;
  virtual bool decode(uint8_t* buf, Command &cmd, ControllerParam_t & cont) override;
  virtual void encode(uint8_t* buf, Command &cmd, ControllerParam_t & cont) override;
};

class ZhijiaEncoderV1: public ZhijiaEncoder
{
public:
  ZhijiaEncoderV1(const std::string & encoding, const std::string & variant): 
          ZhijiaEncoder(encoding, variant) { this->len_ = sizeof(data_map_t); }
  
protected:
  static constexpr size_t UUID_LEN = 3;
  static constexpr size_t ADDR_LEN = 4;
  static constexpr size_t TXDATA_LEN = 17;
  struct data_map_t {
    uint8_t addr[ADDR_LEN];
    uint8_t txdata[TXDATA_LEN];
    uint16_t crc16;
  }__attribute__((packed, aligned(1)));

  virtual std::vector< Command > translate(const Command & cmd, const ControllerParam_t & cont) override;
  virtual bool decode(uint8_t* buf, Command &cmd, ControllerParam_t & cont) override;
  virtual void encode(uint8_t* buf, Command &cmd, ControllerParam_t & cont) override;
};

class ZhijiaEncoderV2: public ZhijiaEncoderV1
{
public:
  ZhijiaEncoderV2(const std::string & encoding, const std::string & variant): 
          ZhijiaEncoderV1(encoding, variant) { this->len_ = sizeof(data_map_t); }
  
protected:
  static constexpr size_t UUID_LEN = 3;
  static constexpr size_t ADDR_LEN = 3;
  static constexpr size_t TXDATA_LEN = 16;
  static constexpr size_t SPARE_LEN = 7;
  struct data_map_t {
    uint8_t txdata[TXDATA_LEN];
    uint8_t pivot;
    uint8_t spare[SPARE_LEN];
  }__attribute__((packed, aligned(1)));

  virtual std::vector< Command > translate(const Command & cmd, const ControllerParam_t & cont) override;
  virtual bool decode(uint8_t* buf, Command &cmd, ControllerParam_t & cont) override;
  virtual void encode(uint8_t* buf, Command &cmd, ControllerParam_t & cont) override;
};


} //namespace bleadvcontroller
} //namespace esphome
