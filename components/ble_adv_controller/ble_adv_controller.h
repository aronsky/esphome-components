#pragma once

#include "esphome/core/component.h"
#include "esphome/core/entity_base.h"
#include "esphome/core/helpers.h"
#ifdef USE_API
#include "esphome/components/api/custom_api_device.h"
#endif
#include "esphome/components/select/select.h"
#include "esphome/components/number/number.h"
#include "ble_adv_handler.h"
#include <vector>
#include <list>

namespace esphome {
namespace bleadvcontroller {

/**
  BleAdvSelect: basic implementation of 'Select' to handle configuration choice from HA directly
 */
class BleAdvSelect : public select::Select
{
public:
  void control(const std::string &value) override { this->publish_state(value); }
  void set_id(const char * name, const StringRef & parent_name);

protected:
  std::string ref_name_;
};

/**
  BleAdvNumber: basic implementation of 'Number' to handle duration(s) choice from HA directly
 */
class BleAdvNumber : public number::Number
{
public:
  void control(float value) override { this->publish_state(value); }
  void set_id(const char * name, const StringRef & parent_name);

protected:
  std::string ref_name_;
};

/**
  BleAdvController:
    One physical device controlled == One Controller.
    Referenced by Entities as their parent to perform commands.
    Chooses which encoder(s) to be used to issue a command
    Interacts with the BleAdvHandler for Queue processing
 */
class BleAdvController : public Component, public EntityBase
#ifdef USE_API
  , public api::CustomAPIDevice
#endif
{
public:
  void setup() override;
  void loop() override;
  virtual void dump_config() override;
  
  void set_min_tx_duration(uint32_t tx_duration) { this->number_duration_.state = tx_duration; }
  void set_max_tx_duration(uint32_t tx_duration) { this->max_tx_duration_ = tx_duration; }
  void set_seq_duration(uint32_t seq_duration) { this->seq_duration_ = seq_duration; }
  void set_forced_id(uint32_t forced_id) { this->forced_id_ = forced_id; }
  void set_forced_id(const std::string & str_id) { this->forced_id_ = fnv1_hash(str_id); }
  void set_encoding_and_variant(const std::string & encoding, uint8_t variant);
  select::Select * get_select_encoding() { return &(this->select_encoding_); }
  number::Number * get_number_duration() { return &(this->number_duration_); }
  void set_reversed(bool reversed) { this->reversed_ = reversed; }
  bool is_reversed() const { return this->reversed_; }
  bool is_supported(const Command &cmd) { return this->get_encoder().is_supported(cmd); }
  void set_show_config(bool show_config) { this->show_config_ = show_config; }

  void set_handler(BleAdvHandler * handler) { this->handler_ = handler; }
  BleAdvEncoder & get_encoder() { return this->handler_->get_encoder(this->select_encoding_.state); }

#ifdef USE_API
  // Services
  void on_pair();
  void on_unpair();
  void on_cmd(float cmd, float arg0, float arg1, float arg2, float arg3);
#endif

  bool enqueue(Command &cmd);

protected:

  uint32_t max_tx_duration_ = 3000;
  uint32_t seq_duration_ = 150;

  uint32_t forced_id_ = 0;
  bool reversed_;

  bool show_config_{false};
  BleAdvSelect select_encoding_;
  BleAdvNumber number_duration_;
  BleAdvHandler * handler_{nullptr};

  class QueueItem {
  public:
    QueueItem(CommandType cmd_type): cmd_type_(cmd_type) {}
    CommandType cmd_type_;
    std::vector< BleAdvParam > params_;
  
    // Only move operators to avoid data copy
    QueueItem(QueueItem&&) = default;
    QueueItem& operator=(QueueItem&&) = default;
  };
  std::list< QueueItem > commands_;

  // Being advertised data properties
  uint8_t tx_count_ = 1;
  uint32_t adv_start_time_ = 0;
  uint16_t adv_id_ = 0;
};

/**
  BleAdvEntity: 
    Base class for implementation of Entities, referencing the parent BleAdvController
 */
class BleAdvEntity: public Component, public Parented < BleAdvController >
{
  public:
    virtual void dump_config() override = 0;

  protected:
    void dump_config_base(const char * tag);
    void command(CommandType cmd, const std::vector<uint8_t> &args);
    void command(CommandType cmd, uint8_t value1 = 0, uint8_t value2 = 0);
};

} //namespace bleadvcontroller
} //namespace esphome
