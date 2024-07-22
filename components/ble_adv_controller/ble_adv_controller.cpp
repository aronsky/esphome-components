#include "ble_adv_controller.h"
#include "esphome/core/log.h"
#include "esphome/core/hal.h"

namespace esphome {
namespace bleadvcontroller {

static const char *TAG = "ble_adv_controller";
const size_t MAX_PACKET_LEN = 26;

// global attribute to know if a controller is already advertising data
// as 2 controllers cannot advertise at the same time
static bool IsBleAdvAdvertisingUsed = false;

void BleAdvController::setup() {
#ifdef USE_API
  register_service(&BleAdvController::on_pair, "pair_" + this->get_object_id());
  register_service(&BleAdvController::on_unpair, "unpair_" + this->get_object_id());
  register_service(&BleAdvController::on_cmd, "cmd_" + this->get_object_id(), {"type", "index", "cmd", "arg0", "arg1", "arg2", "arg3"});
#endif
  // defaulting to built-in hash from id or name
  if (this->forced_id_ == 0) {
    this->forced_id_ = this->get_object_id_hash();
  }
}

void BleAdvController::dump_config() {
  ESP_LOGCONFIG(TAG, "BleAdvController '%s'", this->get_object_id().c_str());
  ESP_LOGCONFIG(TAG, "  Hash ID '%X'", this->forced_id_);
  ESP_LOGCONFIG(TAG, "  Transmission Duration: %d ms", tx_duration_);
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

void BleAdvController::on_cmd(int type, int index, int cmd_type, int arg0, int arg1, int arg2, int arg3) {
  Command cmd(CommandType::CUSTOM);
  cmd.type_ = type;
  cmd.index_ = index;
  cmd.args_[0] = cmd_type;
  cmd.args_[1] = arg0;
  cmd.args_[2] = arg1;
  cmd.args_[3] = arg2;
  cmd.args_[4] = arg3;
  this->enqueue(cmd);
}
#endif

bool BleAdvController::enqueue(Command &cmd) {
  if (!this->is_supported(cmd)) {
    ESP_LOGW(TAG, "Unsupported command received: %d. Aborted.", cmd.cmd_);
    return false;
  }
  uint8_t * buffer = new uint8_t[MAX_PACKET_LEN] {0};
  ++this->tx_count_;
  if (this->tx_count_ > 127) {
    this->tx_count_ = 1;
  }
  cmd.tx_count_ = this->tx_count_;
  cmd.id_ = this->forced_id_;
  this->get_adv_data(buffer, cmd);

  this->commands_.push(buffer);
  return true;
}

void BleAdvController::loop() {
  uint32_t now = millis();
  if(this->adv_start_time_ == 0) {
    // no on going command advertised by this controller
    // check if any to advertise and no other controller is already advertising data
    if(!this->commands_.empty() && !IsBleAdvAdvertisingUsed) {
      // Prevent other controllers to advertise
      IsBleAdvAdvertisingUsed = true;
      this->adv_start_time_ = now;
      this->adv_data_.p_manufacturer_data = this->commands_.front();
      this->commands_.pop();
      this->adv_data_.manufacturer_len = MAX_PACKET_LEN;
      ESP_LOGD(TAG, "%s - start advertising: %s", this->get_object_id().c_str(), 
              esphome::format_hex_pretty(this->adv_data_.p_manufacturer_data, MAX_PACKET_LEN).c_str());
      ESP_ERROR_CHECK_WITHOUT_ABORT(esp_ble_gap_config_adv_data(&(this->adv_data_)));
      ESP_ERROR_CHECK_WITHOUT_ABORT(esp_ble_gap_start_advertising(&(this->adv_params_)));
    }
  }
  else {
    // command is being advertised by this controller, check if stop and clean-up needed
    if (now > this->adv_start_time_ + this->tx_duration_) {
      ESP_LOGD(TAG, "%s - stop advertising", this->get_object_id().c_str());
      ESP_ERROR_CHECK_WITHOUT_ABORT(esp_ble_gap_stop_advertising());
      delete[] this->adv_data_.p_manufacturer_data;
      this->adv_data_.p_manufacturer_data = nullptr;
      this->adv_data_.manufacturer_len = 0;
      this->adv_start_time_ = 0;
      // Allow other controllers to advertise again
      IsBleAdvAdvertisingUsed = false;
    }
  }
}

void BleAdvEntity::dump_config_base(const char * tag) {
  ESP_LOGCONFIG(tag, "  Controller '%s'", this->get_parent()->get_name().c_str());
  ESP_LOGCONFIG(tag, "  Type '0x%.4X'", this->type_);
  ESP_LOGCONFIG(tag, "  Index '%d'", this->index_);
}

void BleAdvEntity::command(CommandType cmd_type, const std::vector<uint8_t> &args) {
  Command cmd(cmd_type);
  cmd.type_ = this->type_;
  cmd.index_ = this->index_;
  std::copy(args.begin(), args.end(), cmd.args_.begin());
  this->get_parent()->enqueue(cmd);
}

void BleAdvEntity::command(CommandType cmd, uint8_t value1, uint8_t value2) {
  this->command(cmd, {value1, value2});
}

} // namespace bleadvcontroller
} // namespace esphome
