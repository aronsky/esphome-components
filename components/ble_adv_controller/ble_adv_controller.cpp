#include "ble_adv_controller.h"
#include "esphome/core/log.h"
#include "esphome/core/hal.h"
#include "esphome/core/application.h"

namespace esphome {
namespace bleadvcontroller {

static const char *TAG = "ble_adv_controller";

void BleAdvSelect::control(const std::string &value) {
  this->publish_state(value);
  uint32_t hash_value = fnv1_hash(value);
  this->rtc_.save(&hash_value);
}

void BleAdvSelect::sub_init() { 
  App.register_select(this);
  this->rtc_ = global_preferences->make_preference< uint32_t >(this->get_object_id_hash());
  uint32_t restored;
  if (this->rtc_.load(&restored)) {
    for (auto & opt: this->traits.get_options()) {
      if(fnv1_hash(opt) == restored) {
        this->state = opt;
        return;
      }
    }
  }
}

void BleAdvNumber::control(float value) {
  this->publish_state(value);
  this->rtc_.save(&value);
}

void BleAdvNumber::sub_init() {
  App.register_number(this);
  this->rtc_ = global_preferences->make_preference< float >(this->get_object_id_hash());
  float restored;
  if (this->rtc_.load(&restored)) {
    this->state = restored;
  }
}

void BleAdvController::set_encoding_and_variant(const std::string & encoding, const std::string & variant) {
  this->select_encoding_.traits.set_options(this->handler_->get_ids(encoding));
  this->cur_encoder_ = this->handler_->get_encoder(encoding, variant);
  this->select_encoding_.state = this->cur_encoder_->get_id();
  this->select_encoding_.add_on_state_callback(std::bind(&BleAdvController::refresh_encoder, this, std::placeholders::_1, std::placeholders::_2));
}

void BleAdvController::refresh_encoder(std::string id, size_t index) {
  this->cur_encoder_ = this->handler_->get_encoder(id);
}

void BleAdvController::set_min_tx_duration(int tx_duration, int min, int max, int step) {
  this->number_duration_.traits.set_min_value(min);
  this->number_duration_.traits.set_max_value(max);
  this->number_duration_.traits.set_step(step);
  this->number_duration_.state = tx_duration;
}

void BleAdvController::setup() {
#ifdef USE_API
  register_service(&BleAdvController::on_pair, "pair_" + this->get_object_id());
  register_service(&BleAdvController::on_unpair, "unpair_" + this->get_object_id());
  register_service(&BleAdvController::on_cmd, "cmd_" + this->get_object_id(), {"cmd", "arg0", "arg1", "arg2", "arg3"});
  register_service(&BleAdvController::on_raw_inject, "inject_raw_" + this->get_object_id(), {"raw"});
#endif
  if (this->is_show_config()) {
    this->select_encoding_.init("Encoding", this->get_name());
    this->number_duration_.init("Duration", this->get_name());
  }
}

void BleAdvController::dump_config() {
  ESP_LOGCONFIG(TAG, "BleAdvController '%s'", this->get_object_id().c_str());
  ESP_LOGCONFIG(TAG, "  Hash ID '%lX'", this->params_.id_);
  ESP_LOGCONFIG(TAG, "  Index '%d'", this->params_.index_);
  ESP_LOGCONFIG(TAG, "  Transmission Min Duration: %ld ms", this->get_min_tx_duration());
  ESP_LOGCONFIG(TAG, "  Transmission Max Duration: %ld ms", this->max_tx_duration_);
  ESP_LOGCONFIG(TAG, "  Transmission Sequencing Duration: %ld ms", this->seq_duration_);
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
  cmd.cmd_ = (uint8_t)cmd_type;
  cmd.args_[0] = (uint8_t)arg0;
  cmd.args_[1] = (uint8_t)arg1;
  cmd.args_[2] = (uint8_t)arg2;
  cmd.args_[3] = (uint8_t)arg3;
  this->enqueue(cmd);
}

void BleAdvController::on_raw_inject(std::string raw) {
  this->commands_.emplace_back(CommandType::CUSTOM);
  this->commands_.back().params_.emplace_back();
  this->commands_.back().params_.back().from_hex_string(raw);
}
#endif

bool BleAdvController::enqueue(Command &cmd) {
  if (!this->cur_encoder_->is_supported(cmd)) {
    ESP_LOGW(TAG, "Unsupported command received: %d. Aborted.", cmd.main_cmd_);
    return false;
  }

  // Reset tx count if near the limit
  if (this->params_.tx_count_ > 120) {
    this->params_.tx_count_ = 0;
  }

  // Remove any previous command of the same type in the queue, if not used for several purposes
  if (cmd.main_cmd_ != CommandType::CUSTOM) {
    uint8_t nb_rm = std::count_if(this->commands_.begin(), this->commands_.end(), [&](QueueItem& q){ return q.cmd_type_ == cmd.cmd_; });
    if (nb_rm) {
      ESP_LOGD(TAG, "Removing %d previous pending commands", nb_rm);
      this->commands_.remove_if( [&](QueueItem& q){ return q.cmd_type_ == cmd.cmd_; } );
    }
  }

  // enqueue the new command and encode the buffer(s)
  this->commands_.emplace_back(cmd.main_cmd_);
  this->cur_encoder_->encode(this->commands_.back().params_, cmd, this->params_);
  
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
      this->handler_->remove_from_advertiser(this->adv_id_);
    }
  }
}

void BleAdvEntity::dump_config_base(const char * tag) {
  ESP_LOGCONFIG(tag, "  Controller '%s'", this->get_parent()->get_name().c_str());
}

void BleAdvEntity::command(CommandType cmd_type, const std::vector<uint8_t> &args) {
  Command cmd(cmd_type);
  std::copy(args.begin(), args.end(), cmd.args_);
  this->get_parent()->enqueue(cmd);
}

void BleAdvEntity::command(CommandType cmd, uint8_t value1, uint8_t value2) {
  this->command(cmd, {value1, value2});
}

} // namespace bleadvcontroller
} // namespace esphome
