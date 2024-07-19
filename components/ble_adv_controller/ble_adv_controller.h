#pragma once

#include "esphome/core/component.h"
#include "esphome/core/entity_base.h"
#include "esphome/core/helpers.h"
#ifdef USE_API
#include "esphome/components/api/custom_api_device.h"
#endif
#include <esp_gap_ble_api.h>
#include <vector>
#include <queue>

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
  FAN_ON = 30,
  FAN_OFF = 31,
  FAN_SPEED = 32,
  FAN_DIR = 33,
};

class Command
{
public:
  Command(CommandType cmd): cmd_(cmd) {}

  CommandType cmd_;
  std::vector<uint8_t> args_{0,0,0,0,0};

  // Attributes from entity
  uint8_t index_ = 0;
  uint16_t type_ = 0x0100;

  // Attributes from controller
  uint32_t id_ = 0;
  uint8_t tx_count_ = 0;
};

class BleAdvController : public Component, public EntityBase
#ifdef USE_API
  , public api::CustomAPIDevice
#endif
{
 public:
  void setup() override;
  void loop() override;
  virtual void dump_config() override;
  
  void set_tx_duration(uint32_t tx_duration) { this->tx_duration_ = tx_duration; }
  void set_forced_id(uint32_t forced_id) { this->forced_id_ = forced_id; }
  void set_variant(uint8_t variant) { this->variant_ = variant; }

#ifdef USE_API
  // Services
  void on_pair();
  void on_unpair();
  void on_cmd(int type, int index, int cmd, int arg0, int arg1, int arg2, int arg3);
#endif

  bool enqueue(Command &cmd);
  
  // supported commands
  virtual bool is_supported(const Command &cmd) = 0;

  // encoding data by dedicated encoder
  virtual void get_adv_data(uint8_t * buf, Command &cmd) = 0;

 protected:

  uint32_t tx_duration_;
  uint32_t forced_id_ = 0;
  uint8_t variant_;

  std::queue<uint8_t *> commands_;

  // Being advertised data properties
  uint8_t tx_count_ = 0;
  uint32_t adv_start_time_ = 0;

  // non static to allow customization by child if needed
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

  // non static to allow customization by child if needed
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

class BleAdvEntity: public Component, public Parented < BleAdvController >
{
  public:
    virtual void dump_config() override = 0;
    void set_index(uint8_t index) { this->index_ = index; }
    void set_type(uint16_t type) { this->type_ = type; }

  protected:
    void dump_config_base(const char * tag);
    void command(CommandType cmd, const std::vector<uint8_t> &args);
    void command(CommandType cmd, uint8_t value1 = 0, uint8_t value2 = 0);

    uint8_t index_ = 0;
    uint16_t type_ = 0x0100;
};

} //namespace bleadvcontroller
} //namespace esphome
