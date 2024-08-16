#pragma once

#include "esphome/core/defines.h"
#include "esphome/core/component.h"
#include "esphome/core/helpers.h"
#ifdef USE_API
#include "esphome/components/api/custom_api_device.h"
#endif

#include <esp_gap_ble_api.h>
#include <vector>
#include <list>

namespace esphome {

#ifdef USE_ESP32_BLE_CLIENT
namespace esp32_ble_tracker {
  class ESPBTDevice;
}
#endif

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
  Command(CommandType cmd = CommandType::NOCMD): main_cmd_(cmd) {}

  CommandType main_cmd_;
  uint8_t cmd_{0};
  uint8_t args_[4]{0};
};

/**
  Controller Parameters
 */
struct ControllerParam_t {
  uint32_t id_ = 0;
  uint8_t tx_count_ = 0;
  uint8_t index_ = 0;
  uint16_t seed_ = 0;
};

static constexpr size_t MAX_PACKET_LEN = 31;

class BleAdvParam
{
public:
  BleAdvParam() {};
  BleAdvParam(BleAdvParam&&) = default;
  BleAdvParam& operator=(BleAdvParam&&) = default;

  void from_raw(const uint8_t * buf, size_t len);
  void from_hex_string(std::string & raw);
  void init_with_ble_param(uint8_t ad_flag, uint8_t data_type);

  bool has_ad_flag() const { return this->ad_flag_index_ != MAX_PACKET_LEN; }
  uint8_t get_ad_flag() const { return this->buf_[this->ad_flag_index_ + 2]; }

  bool has_data() const { return this->data_index_ != MAX_PACKET_LEN; }
  void set_data_len(size_t len);
  uint8_t get_data_len() const { return this->buf_[this->data_index_] - 1; }
  uint8_t get_data_type() const { return this->buf_[this->data_index_ + 1]; }
  uint8_t * get_data_buf() {  return this->buf_ + this->data_index_ + 2; }
  const uint8_t * get_const_data_buf() const { return this->buf_ + this->data_index_ + 2; }

  uint8_t * get_full_buf() { return this->buf_; }
  uint8_t get_full_len() { return this->len_; }

  bool operator==(const BleAdvParam & comp) { return std::equal(comp.buf_, comp.buf_ + MAX_PACKET_LEN, this->buf_); }

  uint32_t duration_{100};

protected:
  uint8_t buf_[MAX_PACKET_LEN]{0};
  size_t len_{0};
  size_t ad_flag_index_{MAX_PACKET_LEN};
  size_t data_index_{MAX_PACKET_LEN};
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
  BleAdvEncoder(const std::string & encoding, const std::string & variant): 
      id_(encoding + " - " + variant), encoding_(encoding), variant_(variant) {}

  const std::string & get_id() const { return this->id_; }
  const std::string & get_encoding() const { return this->encoding_; }
  const std::string & get_variant() const { return this->variant_; }
  bool is_id(const std::string & ref_id) const { return ref_id == this->id_; }
  bool is_id(const std::string & encoding, const std::string & variant) const { return (encoding == this->encoding_) && (variant == this->variant_); }
  bool is_encoding(const std::string & encoding) const { return (encoding == this->encoding_); }

  void set_ble_param(uint8_t ad_flag, uint8_t adv_data_type){ this->ad_flag_ = ad_flag; this->adv_data_type_ = adv_data_type; }
  bool is_ble_param(uint8_t ad_flag, uint8_t adv_data_type) { return this->ad_flag_ == ad_flag && this->adv_data_type_ == adv_data_type; }
  void set_header(const std::vector< uint8_t > && header) { this->header_ = header; }

  virtual std::vector< Command > translate(const Command & cmd, const ControllerParam_t & cont) = 0;
  virtual void encode(std::vector< BleAdvParam > & params, Command &cmd, ControllerParam_t & cont);
  virtual bool is_supported(const Command &cmd) ;
  virtual bool decode(const BleAdvParam & packet, Command &cmd, ControllerParam_t & cont);

protected:
  virtual bool decode(uint8_t* buf, Command &cmd, ControllerParam_t & cont) { return false; };
  virtual void encode(uint8_t* buf, Command &cmd, ControllerParam_t & cont) { };

  // utils for encoding
  void reverse_all(uint8_t* buf, uint8_t len);
  void whiten(uint8_t *buf, size_t len, uint8_t seed);

  // encoder identifiers
  std::string id_;
  std::string encoding_;
  std::string variant_;

  // BLE parameters
  uint8_t ad_flag_{0x00};
  uint8_t adv_data_type_{ESP_BLE_AD_MANUFACTURER_SPECIFIC_TYPE};

  // Common parameters
  std::vector< uint8_t > header_;
  size_t len_{0};
};

#define ENSURE_EQ(param1, param2, ...) if ((param1) != (param2)) { ESP_LOGD(this->id_.c_str(), __VA_ARGS__); return false; }

/**
  BleAdvMultiEncoder:
    Encode several messages at the same time with different encoders
 */
class BleAdvMultiEncoder: public BleAdvEncoder
{
public:
  BleAdvMultiEncoder(const std::string encoding): BleAdvEncoder(encoding, "All") {}
  virtual void encode(std::vector< BleAdvParam > & params, Command &cmd, ControllerParam_t & cont) override;
  virtual bool is_supported(const Command &cmd) override;
  void add_encoder(BleAdvEncoder * encoder) { this->encoders_.push_back(encoder); }

  // Not used
  virtual std::vector< Command > translate(const Command & cmd, const ControllerParam_t & cont) { return std::vector< Command >(); };
  virtual bool decode(const BleAdvParam & packet, Command &cmd, ControllerParam_t & cont) override { return false; }

protected:
  std::vector< BleAdvEncoder * > encoders_;
};

/**
  BleAdvHandler: Central class instanciated only ONCE
  It owns the list of registered encoders and their simplified access, to be used by Controllers.
  It owns the centralized Advertiser allowing to advertise multiple messages at the same time 
    with handling of prioritization and parallel send when possible
 */
class BleAdvHandler: public Component
#ifdef USE_API
  , public api::CustomAPIDevice
#endif
{
public:
  // component handling
  void setup() override;
  void loop() override;

  // Encoder registration and access
  void add_encoder(BleAdvEncoder * encoder);
  BleAdvEncoder * get_encoder(const std::string & id);
  BleAdvEncoder * get_encoder(const std::string & encoding, const std::string & variant);
  std::vector<std::string> get_ids(const std::string & encoding);

  // Advertiser
  uint16_t add_to_advertiser(std::vector< BleAdvParam > & params);
  void remove_from_advertiser(uint16_t msg_id);

  // identify which encoder is relevant for the param, decode and log Action and Controller parameters
  bool identify_param(const BleAdvParam & param, bool ignore_ble_param);

  // Listener
#ifdef USE_ESP32_BLE_CLIENT
  void capture(const esp32_ble_tracker::ESPBTDevice & device, bool ignore_ble_param = true, uint16_t rem_time = 60);
#endif

#ifdef USE_API
  // HA service to decode
  void on_raw_decode(std::string raw);
#endif

protected:
  // ref to registered encoders
  std::vector< BleAdvEncoder * > encoders_;

  // packets being advertised
  std::list< BleAdvProcess > packets_;
  uint16_t id_count = 1;
  uint32_t adv_stop_time_ = 0;

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

  // Packets already captured once
  std::list< BleAdvParam > listen_packets_;
};

} //namespace bleadvcontroller
} //namespace esphome
