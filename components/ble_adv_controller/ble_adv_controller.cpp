#include "ble_adv_controller.h"
#include "esphome/core/log.h"
#include "esphome/core/hal.h"

namespace esphome {
namespace bleadvcontroller {

static const char *TAG = "ble_adv_controller";

void BleAdvSelect::set_id(const char * name, const StringRef & parent_name) {
  // Due to the use of sh... StringRef, we are forced to keep a ref on the built string...
  this->ref_name_ = std::string(parent_name) + " - " + std::string(name);
  this->set_object_id(this->ref_name_.c_str());
  this->set_name(this->ref_name_.c_str());
}

void BleAdvNumber::set_id(const char * name, const StringRef & parent_name) {
  // Due to the use of sh... StringRef, we are forced to keep a ref on the built string...
  this->ref_name_ = std::string(parent_name) + " - " + std::string(name);
  this->set_object_id(this->ref_name_.c_str());
  this->set_name(this->ref_name_.c_str());
}

void BleAdvController::set_encoding_and_variant(const std::string & encoding, uint8_t variant) {
  this->select_encoding_.traits.set_options(this->handler_->get_ids(encoding));
  this->select_encoding_.state = this->handler_->get_encoder(encoding, variant).get_id();
}

void BleAdvController::setup() {
#ifdef USE_API
  register_service(&BleAdvController::on_pair, "pair_" + this->get_object_id());
  register_service(&BleAdvController::on_unpair, "unpair_" + this->get_object_id());
  register_service(&BleAdvController::on_cmd, "cmd_" + this->get_object_id(), {"cmd", "arg0", "arg1", "arg2", "arg3"});
#endif
  if (this->show_config_) {
    // init select for encoding
    this->select_encoding_.set_id("Encoding", this->get_name());
    this->select_encoding_.set_entity_category(EntityCategory::ENTITY_CATEGORY_CONFIG);
    this->select_encoding_.publish_state(this->select_encoding_.state);

    // init number for duration
    this->number_duration_.set_id("Duration", this->get_name());
    this->number_duration_.set_entity_category(EntityCategory::ENTITY_CATEGORY_CONFIG);
    this->number_duration_.traits.set_min_value(100);
    this->number_duration_.traits.set_max_value(500);
    this->number_duration_.traits.set_step(10);
    this->number_duration_.publish_state(this->number_duration_.state);
  }
}

void BleAdvController::dump_config() {
  ESP_LOGCONFIG(TAG, "BleAdvController '%s'", this->get_object_id().c_str());
  ESP_LOGCONFIG(TAG, "  Hash ID '%X'", this->forced_id_);
  ESP_LOGCONFIG(TAG, "  Transmission Min Duration: %d ms", this->get_min_tx_duration());
  ESP_LOGCONFIG(TAG, "  Transmission Max Duration: %d ms", this->max_tx_duration_);
  ESP_LOGCONFIG(TAG, "  Transmission Sequencing Duration: %d ms", this->seq_duration_);
  ESP_LOGCONFIG(TAG, "  Configuration visible: %s", this->show_config_ ? "YES" : "NO");
}

#ifdef USE_API
void BleAdvController::on_pair() { 
  Command cmd(CommandType::PAIR);
  this->enqueue(cmd);
}

void BleAdvController::on_unpair() {
  Command cmd(CommandType::UNPAIR);
  this->enqueue(cmd);
}

void BleAdvController::on_cmd(float cmd_type, float arg0, float arg1, float arg2, float arg3) {
  Command cmd(CommandType::CUSTOM);
  cmd.args_[0] = (uint8_t)cmd_type;
  cmd.args_[1] = (uint8_t)arg0;
  cmd.args_[2] = (uint8_t)arg1;
  cmd.args_[3] = (uint8_t)arg2;
  cmd.args_[4] = (uint8_t)arg3;
  this->enqueue(cmd);
}
#endif

bool BleAdvController::enqueue(Command &cmd) {
  // get the encoder from the currently selected one
  BleAdvEncoder & encoder = this->get_encoder();

  if (!encoder.is_supported(cmd)) {
    ESP_LOGW(TAG, "Unsupported command received: %d. Aborted.", cmd.cmd_);
    return false;
  }

  cmd.tx_count_ = this->tx_count_;
  cmd.id_ = this->forced_id_;

  // Remove any previous command of the same type in the queue, if not used for several purposes
  if (cmd.cmd_ != CommandType::FAN_ONOFF_SPEED // Used for ON / OFF / SPEED
      && cmd.cmd_ != CommandType::CUSTOM) {
    uint8_t nb_rm = std::count_if(this->commands_.begin(), this->commands_.end(), [&](QueueItem& q){ return q.cmd_type_ == cmd.cmd_; });
    if (nb_rm) {
      ESP_LOGD(TAG, "Removing %d previous pending commands", nb_rm);
      this->commands_.remove_if( [&](QueueItem& q){ return q.cmd_type_ == cmd.cmd_; } );
    }
  }

  // enqueue the new command and encode the buffer(s)
  this->commands_.emplace_back(cmd.cmd_);
  this->tx_count_ += encoder.get_adv_data(this->commands_.back().params_, cmd);
  if (this->tx_count_ > 127) {
    this->tx_count_ = 1;
  }

  // setup seq duration for each packet
  bool use_seq_duration = (this->seq_duration_ > 0) && (this->seq_duration_ < this->get_min_tx_duration());
  for (auto & param : this->commands_.back().params_) {
    param.duration_ = use_seq_duration ? this->seq_duration_: this->get_min_tx_duration();
  }
  
  return true;
}

void BleAdvController::loop() {
  uint32_t now = millis();
  if(this->adv_start_time_ == 0) {
    // no on going command advertised by this controller, check if any to advertise
    if(!this->commands_.empty()) {
      QueueItem & item = this->commands_.front();
      for (auto & param : item.params_) {
        ESP_LOGD(TAG, "%s - request start advertising: %s", this->get_object_id().c_str(), 
                esphome::format_hex_pretty(param.buf_, param.len_).c_str());
      }
      this->adv_id_ = this->handler_->add_to_advertiser(item.params_);
      this->adv_start_time_ = now;
      this->commands_.pop_front();
    }
  }
  else {
    // command is being advertised by this controller, check if stop and clean-up needed
    uint32_t duration = this->commands_.empty() ? this->max_tx_duration_ : this->number_duration_.state;
    if (now > this->adv_start_time_ + duration) {
      this->adv_start_time_ = 0;
      ESP_LOGD(TAG, "%s - request stop advertising", this->get_object_id().c_str());
      this->handler_->remove_from_advertiser(this->adv_id_);
    }
  }
}

void BleAdvEntity::dump_config_base(const char * tag) {
  ESP_LOGCONFIG(tag, "  Controller '%s'", this->get_parent()->get_name().c_str());
}

void BleAdvEntity::command(CommandType cmd_type, const std::vector<uint8_t> &args) {
  Command cmd(cmd_type);
  std::copy(args.begin(), args.end(), cmd.args_.begin());
  this->get_parent()->enqueue(cmd);
}

void BleAdvEntity::command(CommandType cmd, uint8_t value1, uint8_t value2) {
  this->command(cmd, {value1, value2});
}

} // namespace bleadvcontroller
} // namespace esphome
