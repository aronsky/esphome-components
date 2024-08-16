#include "ble_adv_handler.h"
#include "esphome/core/log.h"
#include "esphome/core/hal.h"

#ifdef USE_ESP32_BLE_CLIENT
#include "esphome/components/esp32_ble_tracker/esp32_ble_tracker.h"
#endif

namespace esphome {
namespace bleadvcontroller {

static const char *TAG = "ble_adv_handler";

void BleAdvParam::from_raw(const uint8_t * buf, size_t len) {
  // Copy the raw data as is, limiting to the max size of the buffer
  this->len_ = std::min(MAX_PACKET_LEN, len);
  std::copy(buf, buf + this->len_, this->buf_);

  // find the data / flag indexes in the buffer
  size_t cur_len = 0;
  while (cur_len < this->len_ - 2) {
    size_t sub_len = this->buf_[cur_len];
    uint8_t type = this->buf_[cur_len + 1];
    if (type == ESP_BLE_AD_TYPE_FLAG) {
      this->ad_flag_index_ = cur_len;
    }
    if ((type == ESP_BLE_AD_MANUFACTURER_SPECIFIC_TYPE) 
        || (type == ESP_BLE_AD_TYPE_16SRV_CMPL)
        || (type == ESP_BLE_AD_TYPE_SERVICE_DATA)){
      this->data_index_ = cur_len;
    }
    cur_len += (sub_len + 1);
  }  
}

void BleAdvParam::from_hex_string(std::string & raw) {
  // Clean-up input string
  raw = raw.substr(0, raw.find('('));
  raw.erase(std::remove_if(raw.begin(), raw.end(), [&](char & c) { return c == '.' || c == ' '; }), raw.end());
  if (raw.substr(0,2) == "0x") {
    raw = raw.substr(2);
  }

  // convert to integers
  uint8_t raw_int[MAX_PACKET_LEN]{0};
  uint8_t len = std::min(MAX_PACKET_LEN, raw.size()/2);
  for (uint8_t i; i < len; ++i) {
    raw_int[i] = stoi(raw.substr(2*i, 2), 0, 16);
  }
  this->from_raw(raw_int, len);
}

void BleAdvParam::init_with_ble_param(uint8_t ad_flag, uint8_t data_type) {
  if (ad_flag != 0x00) {
    this->ad_flag_index_ = 0;
    this->buf_[0] = 2;
    this->buf_[1] = ESP_BLE_AD_TYPE_FLAG;
    this->buf_[2] = ad_flag;
    this->data_index_ = 3;
    this->buf_[4] = data_type;
  } else {
    this->data_index_ = 0;
    this->buf_[1] = data_type;
  }
}

void BleAdvParam::set_data_len(size_t len) {
  this->buf_[this->data_index_] = len + 1;
  this->len_ = len + 2 + (this->has_ad_flag() ? 3 : 0);
}

bool BleAdvEncoder::is_supported(const Command &cmd) {
  ControllerParam_t cont;
  auto cmds = this->translate(cmd, cont);
  return !cmds.empty();
}

bool BleAdvEncoder::decode(const BleAdvParam & param, Command &cmd, ControllerParam_t & cont) {
  // Check global len and header to discard most of encoders
  size_t len = param.get_data_len() - this->header_.size();
  const uint8_t * cbuf = param.get_const_data_buf();
  if (len != this->len_) return false;
  if (!std::equal(this->header_.begin(), this->header_.end(), cbuf)) return false;

  // copy the data to be decoded, not to alter it for other decoders
  uint8_t buf[MAX_PACKET_LEN]{0};
  std::copy(cbuf, cbuf + param.get_data_len(), buf);
  return this->decode(buf + this->header_.size(), cmd, cont);
}

void BleAdvEncoder::encode(std::vector< BleAdvParam > & params, Command &cmd, ControllerParam_t & cont) {
  auto cmds = (cmd.main_cmd_ == CommandType::CUSTOM) ? std::vector< Command >({cmd}) : this->translate(cmd, cont);
  for (auto & acmd: cmds) {
    cont.tx_count_++;

    params.emplace_back();
    BleAdvParam & param = params.back();
    param.init_with_ble_param(this->ad_flag_, this->adv_data_type_);
    std::copy(this->header_.begin(), this->header_.end(), param.get_data_buf());
    uint8_t * buf = param.get_data_buf() + this->header_.size();

    ESP_LOGD(this->id_.c_str(), "UUID: '0x%X', index: %d, tx: %d, cmd: '0x%02X', args: [%d,%d,%d,%d]", 
        cont.id_, cont.index_, cont.tx_count_, acmd.cmd_, acmd.args_[0], acmd.args_[1], acmd.args_[2], acmd.args_[3]);

    this->encode(buf, acmd, cont);
    param.set_data_len(this->len_ + this->header_.size());    
  }
}

void BleAdvEncoder::whiten(uint8_t *buf, size_t len, uint8_t seed) {
  uint8_t r = seed;
  for (size_t i=0; i < len; i++) {
    uint8_t b = 0;
    for (size_t j=0; j < 8; j++) {
      r <<= 1;
      if (r & 0x80) {
        r ^= 0x11;
        b |= 1 << j;
      }
      r &= 0x7F;
    }
    buf[i] ^= b;
  }
}

void BleAdvEncoder::reverse_all(uint8_t* buf, uint8_t len) {
  for (size_t i = 0; i < len; ++i) {
    uint8_t & x = buf[i];
    x = ((x & 0x55) << 1) | ((x & 0xAA) >> 1);
    x = ((x & 0x33) << 2) | ((x & 0xCC) >> 2);
    x = ((x & 0x0F) << 4) | ((x & 0xF0) >> 4);
  }
}

void BleAdvMultiEncoder::encode(std::vector< BleAdvParam > & params, Command &cmd, ControllerParam_t & cont) {
  uint8_t count = 0;
  for(auto & encoder : this->encoders_) {
    ControllerParam_t c_cont = cont; // Copy to avoid increasing counts for the same command
    encoder->encode(params, cmd, c_cont);
    count = std::max(c_cont.tx_count_, count);
  }
  cont.tx_count_ = count;
}

bool BleAdvMultiEncoder::is_supported(const Command &cmd) {
  bool is_supported = false;
  for(auto & encoder : this->encoders_) {
    is_supported |= encoder->is_supported(cmd);
  }
  return is_supported;
}

void BleAdvHandler::setup() {
#ifdef USE_API
  register_service(&BleAdvHandler::on_raw_decode, "raw_decode", {"raw"});
#endif
}

void BleAdvHandler::add_encoder(BleAdvEncoder * encoder) { 
  BleAdvMultiEncoder * enc_all = nullptr;
  auto all_enc = std::find_if(this->encoders_.begin(), this->encoders_.end(), 
                    [&](BleAdvEncoder * p){ return p->is_id(encoder->get_encoding(), "All"); });
  if (all_enc == this->encoders_.end()) {
    enc_all = new BleAdvMultiEncoder(encoder->get_encoding());
    this->encoders_.push_back(enc_all);
  } else {
    enc_all = static_cast<BleAdvMultiEncoder*>(*all_enc);
  }
  this->encoders_.push_back(encoder);
  enc_all->add_encoder(encoder);
}

BleAdvEncoder * BleAdvHandler::get_encoder(const std::string & id) {
  for(auto & encoder : this->encoders_) {
    if (encoder->is_id(id)) {
      return encoder;
    }
  }
  ESP_LOGE(TAG, "No Encoder with id: %s", id.c_str());
  return nullptr;
}

BleAdvEncoder * BleAdvHandler::get_encoder(const std::string & encoding, const std::string & variant) {
  for(auto & encoder : this->encoders_) {
    if (encoder->is_id(encoding, variant)) {
      return encoder;
    }
  }
  ESP_LOGE(TAG, "No Encoder with encoding: %s and variant: %s", encoding.c_str(), variant.c_str());
  return nullptr;
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
    ESP_LOGD(TAG, "request start advertising - %d: %s", msg_id, 
                esphome::format_hex_pretty(param.get_full_buf(), param.get_full_len()).c_str());
  }
  params.clear(); // As we moved the content, just to be sure no caller will re use it
  return this->id_count;
}

void BleAdvHandler::remove_from_advertiser(uint16_t msg_id) {
  ESP_LOGD(TAG, "request stop advertising - %d", msg_id);
  for (auto & param : this->packets_) {
    if (param.id_ == msg_id) {
      param.to_be_removed_ = true;
    }
  }
}

// try to identify the relevant encoder
bool BleAdvHandler::identify_param(const BleAdvParam & param, bool ignore_ble_param) {
  for(auto & encoder : this->encoders_) {
    if (!ignore_ble_param && !encoder->is_ble_param(param.get_ad_flag(), param.get_data_type())) {
      continue;
    }
    ControllerParam_t cont;
    Command cmd(CommandType::CUSTOM);
    if(encoder->decode(param, cmd, cont)) {
      ESP_LOGI(encoder->get_id().c_str(), "Decoded OK - tx: %d, cmd: '0x%02X', Args: [%d,%d,%d,%d]",
               cont.tx_count_, cmd.cmd_, cmd.args_[0], cmd.args_[1], cmd.args_[2], cmd.args_[3]);

      std::string config_str = "config: \nble_adv_controller:";
      config_str += "\n  - id: my_controller_id";
      config_str += "\n    encoding: %s";
      config_str += "\n    variant: %s";
      config_str += "\n    forced_id: 0x%X";
      if (cont.index_ != 0) {
        config_str += "\n    index: %d";
      }
      ESP_LOGI(TAG, config_str.c_str(), encoder->get_encoding().c_str(), encoder->get_variant().c_str(), cont.id_, cont.index_);
      
      // Re encoding with the same parameters to check if it gives the same output
      std::vector< BleAdvParam > params;
      cont.tx_count_--; // as the encoder will increase it automatically
      if(cmd.cmd_ == 0x28) {
        // Force recomputation of Args by translate function for PAIR command, as part of encoding
        cmd.main_cmd_ = CommandType::PAIR;
      }
      encoder->encode(params, cmd, cont);
      BleAdvParam & fparam = params.back();
      ESP_LOGD(TAG, "enc - %s", esphome::format_hex_pretty(fparam.get_full_buf(), fparam.get_full_len()).c_str());
      bool nodiff = std::equal(param.get_const_data_buf(), param.get_const_data_buf() + param.get_data_len(), fparam.get_data_buf());
      nodiff ? ESP_LOGI(TAG, "Decoded / Re-encoded with NO DIFF") : ESP_LOGE(TAG, "DIFF after Decode / Re-encode");

      return true;
    } 
  }
  return false;
}

#ifdef USE_API
void BleAdvHandler::on_raw_decode(std::string raw) {
  BleAdvParam param;
  param.from_hex_string(raw);
  ESP_LOGD(TAG, "raw - %s", esphome::format_hex_pretty(param.get_full_buf(), param.get_full_len()).c_str());
  this->identify_param(param, true);
}
#endif

#ifdef USE_ESP32_BLE_CLIENT
/* Basic class inheriting esp32_ble_tracker::ESPBTDevice in order to access 
    protected attribute 'scan_result_' containing raw advertisement
*/
class HackESPBTDevice: public esp32_ble_tracker::ESPBTDevice {
public:
  void get_raw_packet(BleAdvParam & param) const {
    param.from_raw(this->scan_result_.ble_adv, this->scan_result_.adv_data_len);
  }
};

void BleAdvHandler::capture(const esp32_ble_tracker::ESPBTDevice & device, bool ignore_ble_param, uint16_t rem_time) {
  // Clean-up expired packets
  this->listen_packets_.remove_if( [&](BleAdvParam & p){ return p.duration_ < millis(); } );

  // Read raw advertised packets
  BleAdvParam param;
  const HackESPBTDevice * hack_device = reinterpret_cast< const HackESPBTDevice * >(&device);
  hack_device->get_raw_packet(param);
  if (!param.has_data()) return;

  // Check if not already received in the last 300s
  auto idx = std::find(this->listen_packets_.begin(), this->listen_packets_.end(), param);
  if (idx == this->listen_packets_.end()) {
    ESP_LOGD(TAG, "raw - %s", esphome::format_hex_pretty(param.get_full_buf(), param.get_full_len()).c_str());
    param.duration_ = millis() + (uint32_t)rem_time * 1000;
    this->identify_param(param, ignore_ble_param);
    this->listen_packets_.emplace_back(std::move(param));
  }
}
#endif

void BleAdvHandler::loop() {
  if (this->adv_stop_time_ == 0) {
    // No packet is being advertised, process with clean-up IF already processed once and requested for removal
    this->packets_.remove_if([&](BleAdvProcess & p){ return p.processed_once_ && p.to_be_removed_; } );
    // if packets to be advertised, advertise the front one
    if (!this->packets_.empty()) {
      BleAdvParam & packet = this->packets_.front().param_;
      ESP_ERROR_CHECK_WITHOUT_ABORT(esp_ble_gap_config_adv_data_raw(packet.get_full_buf(), packet.get_full_len()));
      ESP_ERROR_CHECK_WITHOUT_ABORT(esp_ble_gap_start_advertising(&(this->adv_params_)));
      this->adv_stop_time_ = millis() + this->packets_.front().param_.duration_;
      this->packets_.front().processed_once_ = true;
    }
  } else {
    // Packet is being advertised, check if time to switch to next one in case:
    // The advertise seq_duration expired AND
    // There is more than one packet to advertise OR the front packet was requested to be removed
    bool multi_packets = (this->packets_.size() > 1);
    bool front_to_be_removed = this->packets_.front().to_be_removed_;
    if ((millis() > this->adv_stop_time_) && (multi_packets || front_to_be_removed)) {
      ESP_ERROR_CHECK_WITHOUT_ABORT(esp_ble_gap_stop_advertising());
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
