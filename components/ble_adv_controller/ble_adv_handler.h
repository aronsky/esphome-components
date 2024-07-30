#pragma once

#include "esphome/core/component.h"
#include "esphome/core/helpers.h"
#include <esp_gap_ble_api.h>
#include <vector>
#include <list>

namespace esphome {
namespace bleadvcontroller {

enum CommandType {
  NOCMD = 0,
  PAIR = 1,
  UNPAIR = 2,
  CUSTOM = 3,
  LIGHT_ON = 13,
  LIGHT_OFF = 14,
  LIGHT_DIM = 15,
  LIGHT_CCT = 16,
  LIGHT_WCOLOR = 17,
  LIGHT_SEC_ON = 18,
  LIGHT_SEC_OFF = 19,
  FAN_ON = 30,
  FAN_OFF = 31,
  FAN_SPEED = 32,
  FAN_ONOFF_SPEED = 33,
  FAN_DIR = 34,
  FAN_OSC = 35,
};

/**
  Command: 
    structure to transport basic parameters for processing by encoders
 */
class Command
{
public:
  Command(CommandType cmd): cmd_(cmd) {}

  CommandType cmd_;
  std::vector<uint8_t> args_{0,0,0,0,0};

  // Attributes from controller
  uint32_t id_ = 0;
  uint8_t tx_count_ = 0;
};

const size_t MAX_PACKET_LEN = 26;

class BleAdvParam
{
public:
  BleAdvParam() {};
  uint8_t buf_[MAX_PACKET_LEN]{0};
  size_t len_{MAX_PACKET_LEN};
  uint32_t duration_{100};

  // Only move operators to avoid data copy
  BleAdvParam(BleAdvParam&&) = default;
  BleAdvParam& operator=(BleAdvParam&&) = default;
};

class BleAdvProcess
{
public:
  BleAdvProcess(uint32_t id, BleAdvParam && param): param_(std::move(param)), id_(id) {}
  BleAdvParam param_;
  uint32_t id_{0};
  bool processed_once_{false};
  bool to_be_removed_{false};

  // Only move operators to avoid data copy
  BleAdvProcess(BleAdvProcess&&) = default;
  BleAdvProcess& operator=(BleAdvProcess&&) = default;
} ;

/**
  BleAdvEncoder: 
    Base class for encoders, for registration in the BleAdvHandler
    and usage by BleAdvController
 */
class BleAdvEncoder {
public:
  BleAdvEncoder(const std::string & id, const std::string & encoding, int variant): 
      id_(id), encoding_(encoding), variant_(variant) {}

  const std::string & get_id() const { return this->id_; }
  bool is_id(const std::string & ref_id) const { return ref_id == this->id_; }
  bool is_id(const std::string & encoding, int variant) const { return (encoding == this->encoding_) && (variant == this->variant_); }
  bool is_encoding(const std::string & encoding) const { return (encoding == this->encoding_); }

  virtual uint8_t get_adv_data(std::vector< BleAdvParam > & params, Command &cmd) = 0;
  virtual bool is_supported(const Command &cmd) = 0;

protected:
  std::string id_;
  std::string encoding_;
  int variant_ = 0;
};

/**
  BleAdvMultiEncoder:
    Encode several messages at the same time with different encoders
 */
class BleAdvMultiEncoder: public BleAdvEncoder
{
public:
  BleAdvMultiEncoder(const std::string id, const std::string encoding): BleAdvEncoder(id, encoding, -1) {}
  virtual uint8_t get_adv_data(std::vector< BleAdvParam > & params, Command &cmd) override;
  virtual bool is_supported(const Command &cmd) override;
  void add_encoder(BleAdvEncoder * encoder_id) { this->encoder_ids_.push_back(encoder_id); }

protected:
  std::vector< BleAdvEncoder * > encoder_ids_;
};

/**
  BleAdvHandler: Central class instanciated only ONCE
  It owns the list of registered encoders and their simplified access, to be used by Controllers.
  It owns the centralized Advertiser allowing to advertise multiple messages at the same time 
    with handling of prioritization and parallel send when possible
 */
class BleAdvHandler: public Component
{
public:
  // Loop handling
  void loop() override;

  // Encoder registration and access
  void add_encoder(BleAdvEncoder * encoder) { this->encoders_.push_back(encoder); }
  BleAdvEncoder & get_encoder(const std::string & id);
  BleAdvEncoder & get_encoder(const std::string & encoding, int variant);
  std::vector<std::string> get_ids(const std::string & encoding);

  // Advertiser
  uint16_t add_to_advertiser(std::vector< BleAdvParam > & params);
  void remove_from_advertiser(uint16_t msg_id);

protected:
  // ref to registered encoders
  std::vector< BleAdvEncoder * > encoders_;

  // packets being advertised
  std::list< BleAdvProcess > packets_;
  uint16_t id_count = 1;
  uint32_t adv_stop_time_ = 0;

  esp_ble_adv_data_t adv_data_ = {
    .set_scan_rsp = false,
    .include_name = false,
    .include_txpower = false,
    .min_interval = 0x0001,
    .max_interval = 0x0004,
    .appearance = 0x00,
    .manufacturer_len = 0,
    .p_manufacturer_data = nullptr,
    .service_data_len = 0,
    .p_service_data = nullptr,
    .service_uuid_len = 0,
    .p_service_uuid = nullptr,
    .flag = (ESP_BLE_ADV_FLAG_LIMIT_DISC | ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_DMT_CONTROLLER_SPT),
  };

  esp_ble_adv_params_t adv_params_ = {
    .adv_int_min = 0x20,
    .adv_int_max = 0x20,
    .adv_type = ADV_TYPE_NONCONN_IND,
    .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
    .peer_addr = { 0x00 },
    .peer_addr_type = BLE_ADDR_TYPE_PUBLIC,
    .channel_map = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
  };

};

} //namespace bleadvcontroller
} //namespace esphome
