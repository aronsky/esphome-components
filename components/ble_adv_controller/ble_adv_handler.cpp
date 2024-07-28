#include "ble_adv_handler.h"
#include "esphome/core/log.h"
#include "esphome/core/hal.h"

namespace esphome {
namespace bleadvcontroller {

static const char *TAG = "ble_adv_handler";

uint8_t BleAdvMultiEncoder::get_adv_data(std::vector< BleAdvParam > & params, Command &cmd) {
  uint8_t count = 0;
  for(auto & encoder : this->encoder_ids_) {
    count = std::max(encoder->get_adv_data(params, cmd), count);
  }
  return count;
}

bool BleAdvMultiEncoder::is_supported(const Command &cmd) {
  bool is_supported = false;
  for(auto & encoder : this->encoder_ids_) {
    is_supported |= encoder->is_supported(cmd);
  }
  return is_supported;
}

BleAdvEncoder & BleAdvHandler::get_encoder(const std::string & id) {
  for(auto & encoder : this->encoders_) {
    if (encoder->is_id(id)) {
      return *encoder;
    }
  }
  ESP_LOGE(TAG, "No Encoder with id: %s", id.c_str());
  return **(this->encoders_.begin());
}

BleAdvEncoder & BleAdvHandler::get_encoder(const std::string & encoding, int variant) {
  for(auto & encoder : this->encoders_) {
    if (encoder->is_id(encoding, variant)) {
      return *encoder;
    }
  }
  ESP_LOGE(TAG, "No Encoder with encoding: %s and variant: %d", encoding.c_str(), variant);
  return **(this->encoders_.begin());
}

std::vector<std::string> BleAdvHandler::get_ids(const std::string & encoding) {
  std::vector<std::string> ids;
  for(auto & encoder : this->encoders_) {
    if (encoder->is_encoding(encoding)) {
      ids.push_back(encoder->get_id());
    }
  }
  return ids;
}

uint16_t BleAdvHandler::add_to_advertiser(std::vector< BleAdvParam > & params) {
  uint32_t msg_id = ++this->id_count;
  for (auto & param : params) {
    this->packets_.emplace_back(BleAdvProcess(msg_id, std::move(param)));
  }
  params.clear(); // As we moved the content, just to be sure no caller will re use it
  return this->id_count;
}

void BleAdvHandler::remove_from_advertiser(uint16_t msg_id) {
  for (auto & param : this->packets_) {
    if (param.id_ == msg_id) {
      param.to_be_removed_ = true;
    }
  }
}

void BleAdvHandler::loop() {
  if (this->adv_stop_time_ == 0) {
    // No packet is being advertised, process with clean-up IF already processed once and requested for removal
    this->packets_.remove_if([&](BleAdvProcess & p){ return p.processed_once_ && p.to_be_removed_; } );
    // if packets to be advertised, advertise the front one
    if (!this->packets_.empty()) {
      BleAdvParam & packet = this->packets_.front().param_;
      this->adv_data_.p_manufacturer_data = packet.buf_;
      this->adv_data_.manufacturer_len = packet.len_;
      //ESP_LOGD(TAG, "Effective advertising start at %d: id %d - %s", millis(), this->packets_.front().id_,
      //        esphome::format_hex_pretty(packet.buf_, packet.len_).c_str());
      ESP_ERROR_CHECK_WITHOUT_ABORT(esp_ble_gap_config_adv_data(&(this->adv_data_)));
      ESP_ERROR_CHECK_WITHOUT_ABORT(esp_ble_gap_start_advertising(&(this->adv_params_)));
      this->adv_stop_time_ = millis() + this->packets_.front().param_.duration_;
      this->packets_.front().processed_once_ = true;
    }
  } else {
    // Packet is being advertised, check if time to switch to next one in case:
    // The advertise seq duration expired AND
    // There is more than one packet to advertise OR the front packet was requested to be removed
    bool multi_packets = (this->packets_.size() > 1);
    bool front_to_be_removed = this->packets_.front().to_be_removed_;
    if ((millis() > this->adv_stop_time_) && (multi_packets || front_to_be_removed)) {
      //ESP_LOGD(TAG, "Effective advertising stop at %d: id %d", millis(), this->packets_.front().id_);
      //ESP_LOGD(TAG, "Nb Packets: %d", this->packets_.size() );
      //ESP_LOGD(TAG, "Front to be removed: %s", front_to_be_removed ? "Y":"N" );
      ESP_ERROR_CHECK_WITHOUT_ABORT(esp_ble_gap_stop_advertising());
      delay(10); // Not sure it helps, but...
      this->adv_data_.p_manufacturer_data = nullptr;
      this->adv_data_.manufacturer_len = 0;
      this->adv_stop_time_ = 0;
      if (front_to_be_removed) {
        this->packets_.pop_front();
      } else if (multi_packets) {
        this->packets_.emplace_back(std::move(this->packets_.front()));
        this->packets_.pop_front();
      }
    }
  }
}

} // namespace bleadvcontroller
} // namespace esphome
