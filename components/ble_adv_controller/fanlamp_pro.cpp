#include "fanlamp_pro.h"
#include "esphome/core/log.h"
#include <arpa/inet.h>

#define MBEDTLS_AES_ALT
#include <aes_alt.h>

namespace esphome {
namespace bleadvcontroller {

std::vector< Command > FanLampEncoder::translate(const Command & cmd, const ControllerParam_t & cont) {
  Command cmd_real(cmd.main_cmd_);
  switch(cmd.main_cmd_)
  {
    case CommandType::PAIR:
      cmd_real.cmd_ = 0x28;
      break;
    case CommandType::UNPAIR:
      cmd_real.cmd_ = 0x45;
      break;
    case CommandType::LIGHT_ON:
      cmd_real.cmd_ = 0x10;
      break;
    case CommandType::LIGHT_OFF:
      cmd_real.cmd_ = 0x11;
      break;
    case CommandType::LIGHT_WCOLOR:
      cmd_real.cmd_ = 0x21;
      break;
    case CommandType::LIGHT_SEC_ON:
      cmd_real.cmd_ = 0x12;
      break;
    case CommandType::LIGHT_SEC_OFF:
      cmd_real.cmd_ = 0x13;
      break;
    case CommandType::FAN_ONOFF_SPEED:
      cmd_real.cmd_ = 0x31;
      break;
    case CommandType::FAN_DIR:
      cmd_real.cmd_ = 0x15;
      break;
    case CommandType::FAN_OSC:
      cmd_real.cmd_ = 0x16;
      break;
    case CommandType::NOCMD:
    default:
      break;
  }
  std::vector< Command > cmds;
  if(cmd_real.cmd_ != 0x00) {
    cmds.emplace_back(cmd_real);
  }
  return cmds;
}

uint16_t FanLampEncoder::get_seed(uint16_t forced_seed) {
  return (forced_seed == 0) ? (uint16_t) rand() % 0xFFF5 : forced_seed;
}

uint16_t FanLampEncoder::crc16(uint8_t* buf, size_t len, uint16_t seed) {
  return esphome::crc16be(buf, len, seed);
}

FanLampEncoderV1::FanLampEncoderV1(const std::string & encoding, const std::string & variant, uint8_t pair_arg3,
                                    bool pair_arg_only_on_pair, bool xor1, uint8_t supp_prefix):
          FanLampEncoder(encoding, variant, {0xAA, 0x98, 0x43, 0xAF, 0x0B, 0x46, 0x46, 0x46}), pair_arg3_(pair_arg3), pair_arg_only_on_pair_(pair_arg_only_on_pair), 
              with_crc2_(supp_prefix == 0x00), xor1_(xor1) {
  if (supp_prefix != 0x00) this->prefix_.insert(this->prefix_.begin(), supp_prefix);
  this->len_ = this->prefix_.size() + sizeof(data_map_t) + (this->with_crc2_ ? 2 : 1);
}

std::vector< Command > FanLampEncoderV1::translate(const Command & cmd, const ControllerParam_t & cont) {
  auto cmds = FanLampEncoder::translate(cmd, cont);
  for (auto & cmd_real: cmds) {
    switch(cmd_real.main_cmd_)
    {
      case CommandType::PAIR:
        cmd_real.args_[0] = cont.id_ & 0xFF;
        cmd_real.args_[1] = (cont.id_ >> 8) & 0xF0;
        cmd_real.args_[2] = this->pair_arg3_;
        break;
      case CommandType::LIGHT_WCOLOR:
        cmd_real.args_[0] = cmd.args_[0];
        cmd_real.args_[1] = cmd.args_[1];
        break;
      case CommandType::FAN_ONOFF_SPEED:
        cmd_real.cmd_ = (cmd.args_[1] == 6) ? 0x32 : 0x31; // use Fan Gear or Fan Level
        cmd_real.args_[0] = cmd.args_[0];
        cmd_real.args_[1] = (cmd.args_[1] == 6) ? cmd.args_[1] : 0;
        break;
      case CommandType::FAN_DIR:
        cmd_real.args_[0] = !cmd.args_[0];
        break;
      case CommandType::FAN_OSC:
        cmd_real.args_[0] = cmd.args_[0];
        break;
      case CommandType::NOCMD:
      default:
        break;
    }
  }
  return cmds;
}

bool FanLampEncoderV1::decode(uint8_t* buf, Command &cmd, ControllerParam_t & cont){
  this->whiten(buf, this->len_, 0x6F);
  this->reverse_all(buf, this->len_);
  ENSURE_EQ(std::equal(this->prefix_.begin(), this->prefix_.end(), buf), true, "prefix KO");

  std::string decoded = esphome::format_hex_pretty(buf, this->len_);

  uint8_t data_start = this->prefix_.size();
  data_map_t * data = (data_map_t *) (buf + data_start);

  // distinguish between lampsmart pro and fanlamp pro
  if (data->command == 0x28 && this->pair_arg3_ != data->args[2]) return false;
  if (data->command != 0x28 && !this->pair_arg_only_on_pair_ && data->args[2] != this->pair_arg3_) return false;
  if (data->command != 0x28 && this->pair_arg_only_on_pair_ && data->args[2] != 0) return false;

  uint16_t seed = htons(data->seed);
  uint8_t seed8 = static_cast<uint8_t>(seed & 0xFF);
  ENSURE_EQ(data->r2, this->xor1_ ? seed8 ^ 1 : seed8, "Decoded KO (r2) - %s", decoded.c_str());

  uint16_t crc16 = htons(this->crc16((uint8_t*)(data), sizeof(data_map_t) - 2, ~seed));
  ENSURE_EQ(crc16, data->crc16, "Decoded KO (crc16) - %s", decoded.c_str());

  if (data->args[2] != 0) {
    ENSURE_EQ(data->args[2], this->pair_arg3_, "Decoded KO (arg3) - %s", decoded.c_str());
  }

  if (this->with_crc2_) {
    uint16_t crc16_mac = this->crc16(buf + 1, 5, 0xffff);
    uint16_t crc16_2 = htons(this->crc16(buf + data_start, sizeof(data_map_t), crc16_mac));
    uint16_t crc16_data_2 = *(uint16_t*) &buf[this->len_ - 2];
    ENSURE_EQ(crc16_data_2, crc16_2, "Decoded KO (crc16_2) - %s", decoded.c_str());
  }

  uint8_t rem_id = data->src ^ seed8;

  cmd.cmd_ = (CommandType) (data->command);
  std::copy(data->args, data->args + sizeof(data->args), cmd.args_);
  cont.tx_count_ = data->tx_count;
  cont.index_ = (data->group_index & 0x0F00) >> 8;
  cont.id_ = data->group_index + 256*256*rem_id;
  cont.seed_ = seed;
  return true;
}

/*    Code taken from FanLamp App
        public static final byte[] PREAMBLE = {113, 15, 85};                 // 0x71, 0x0F, 0x55
        public static final byte[] DEVICE_ADDRESS = {-104, 67, -81, 11, 70}; // 0x68, 0x43, 0xAF, 0x0B, 0x46

        byte[] bArr2 = new byte[22];
        bArr2[0] = (LampConfig.DEVICE_ADDRESS[0] & 128) == 128 ? (byte) -86 : (byte) 85;  // 0xAA : 0x55
        int i7 = 0;
        while (i7 < Math.min(LampConfig.DEVICE_ADDRESS.length, 5)) {
            int i8 = i7 + 1;
            bArr2[i8] = LampConfig.DEVICE_ADDRESS[i7];
            i7 = i8;
        }
        bArr2[6] = bArr2[5];
        bArr2[7] = bArr2[5];
        ...
        byte[] bArr3 = new byte[25];
        System.arraycopy(LampConfig.PREAMBLE, 0, bArr3, 0, LampConfig.PREAMBLE.length);
        System.arraycopy(bArr2, 0, bArr3, 3, bArr2.length);
*/

void FanLampEncoderV1::encode(uint8_t* buf, Command &cmd_real, ControllerParam_t & cont) {
  std::copy(this->prefix_.begin(), this->prefix_.end(), buf);
  data_map_t *data = (data_map_t*)(buf + this->prefix_.size());
  
  uint16_t seed = this->get_seed(cont.seed_);
  uint8_t seed8 = static_cast<uint8_t>(seed & 0xFF);
  uint16_t cmd_id_trunc = static_cast<uint16_t>(cont.id_ & 0xF0FF);

  data->command = cmd_real.cmd_;
  data->group_index = cmd_id_trunc + (((uint16_t)(cont.index_ & 0x0F)) << 8);
  data->tx_count = cont.tx_count_;
  data->outs = 0;
  data->src = this->xor1_ ? seed8 ^ 1 : seed8 ^ ((cont.id_ >> 16) & 0xFF);
  data->r2 = this->xor1_ ? seed8 ^ 1 : seed8;
  data->seed = htons(seed);
  data->args[0] = cmd_real.args_[0];
  data->args[1] = cmd_real.args_[1];
  data->args[2] = this->pair_arg_only_on_pair_ ? cmd_real.args_[2] : this->pair_arg3_;
  data->crc16 = htons(this->crc16((uint8_t*)(data), sizeof(data_map_t) - 2, ~seed));
  
  if (this->with_crc2_) {
    uint16_t* crc16_2 = (uint16_t*) &buf[this->len_ - 2];
    uint16_t crc_mac = this->crc16(buf + 1, 5, 0xffff);
    *crc16_2 = htons(this->crc16((uint8_t*)(data), sizeof(data_map_t), crc_mac));
  } else {
    buf[this->len_ - 1] = 0xAA;
  }

  this->reverse_all(buf, this->len_);
  this->whiten(buf, this->len_, 0x6F);
}

FanLampEncoderV2::FanLampEncoderV2(const std::string & encoding, const std::string & variant, const std::vector<uint8_t> && prefix, uint16_t device_type, bool with_sign):
  FanLampEncoder(encoding, variant, prefix), device_type_(device_type), with_sign_(with_sign) {
  this->len_ = this->prefix_.size() + sizeof(data_map_t);
}

uint16_t FanLampEncoderV2::sign(uint8_t* buf, uint8_t tx_count, uint16_t seed) {
  uint8_t sigkey[16] = {0, 0, 0, 0x0D, 0xBF, 0xE6, 0x42, 0x68, 0x41, 0x99, 0x2D, 0x0F, 0xB0, 0x54, 0xBB, 0x16};
  
  sigkey[0] = seed & 0xff;
  sigkey[1] = (seed >> 8) & 0xff;
  sigkey[2] = tx_count;
  mbedtls_aes_context aes_ctx;
  mbedtls_aes_init(&aes_ctx);
  mbedtls_aes_setkey_enc(&aes_ctx, sigkey, sizeof(sigkey)*8);
  uint8_t aes_in[16], aes_out[16];
  memcpy(aes_in, buf, 16);
  mbedtls_aes_crypt_ecb(&aes_ctx, ESP_AES_ENCRYPT, aes_in, aes_out);
  mbedtls_aes_free(&aes_ctx);
  uint16_t sign = ((uint16_t*) aes_out)[0]; 
  return sign == 0 ? 0xffff : sign;
}

void FanLampEncoderV2::whiten(uint8_t *buf, uint8_t size, uint8_t seed, uint8_t salt) {
  static constexpr uint8_t XBOXES[128] = {
    0xB7, 0xFD, 0x93, 0x26, 0x36, 0x3F, 0xF7, 0xCC,
    0x34, 0xA5, 0xE5, 0xF1, 0x71, 0xD8, 0x31, 0x15,
    0x04, 0xC7, 0x23, 0xC3, 0x18, 0x96, 0x05, 0x9A,
    0x07, 0x12, 0x80, 0xE2, 0xEB, 0x27, 0xB2, 0x75,
    0xD0, 0xEF, 0xAA, 0xFB, 0x43, 0x4D, 0x33, 0x85,
    0x45, 0xF9, 0x02, 0x7F, 0x50, 0x3C, 0x9F, 0xA8,
    0x51, 0xA3, 0x40, 0x8F, 0x92, 0x9D, 0x38, 0xF5,
    0xBC, 0xB6, 0xDA, 0x21, 0x10, 0xFF, 0xF3, 0xD2,
    0xE0, 0x32, 0x3A, 0x0A, 0x49, 0x06, 0x24, 0x5C,
    0xC2, 0xD3, 0xAC, 0x62, 0x91, 0x95, 0xE4, 0x79,
    0xE7, 0xC8, 0x37, 0x6D, 0x8D, 0xD5, 0x4E, 0xA9,
    0x6C, 0x56, 0xF4, 0xEA, 0x65, 0x7A, 0xAE, 0x08,
    0xE1, 0xF8, 0x98, 0x11, 0x69, 0xD9, 0x8E, 0x94,
    0x9B, 0x1E, 0x87, 0xE9, 0xCE, 0x55, 0x28, 0xDF,
    0x8C, 0xA1, 0x89, 0x0D, 0xBF, 0xE6, 0x42, 0x68,
    0x41, 0x99, 0x2D, 0x0F, 0xB0, 0x54, 0xBB, 0x16
  };

  for (uint8_t i = 0; i < size; ++i) {
    buf[i] ^= XBOXES[((seed + i + 9) & 0x1f) + (salt & 0x3) * 0x20];
    buf[i] ^= seed;
  }
}

std::vector< Command > FanLampEncoderV2::translate(const Command & cmd, const ControllerParam_t & cont) {
  auto cmds = FanLampEncoder::translate(cmd, cont);
  for (auto & cmd_real: cmds) {
    switch(cmd_real.main_cmd_)
    {
      case CommandType::LIGHT_WCOLOR:
        cmd_real.args_[2] = cmd.args_[0];
        cmd_real.args_[3] = cmd.args_[1];
        break;
      case CommandType::FAN_ONOFF_SPEED:
        cmd_real.args_[1] = (cmd.args_[1] == 6) ? 0x20 : 0; // specific flag for 6 level
        cmd_real.args_[2] = cmd.args_[0];
        break;
      case CommandType::FAN_DIR:
        cmd_real.args_[1] = !cmd.args_[0];
        break;
      case CommandType::FAN_OSC:
        cmd_real.args_[1] = cmd.args_[0];
        break;
      case CommandType::NOCMD:
      default:
        break;
    }
  }
  return cmds;
}

bool FanLampEncoderV2::decode(uint8_t* buf, Command &cmd, ControllerParam_t & cont){
  data_map_t * data = (data_map_t *) (buf + this->prefix_.size());
  uint16_t crc16 = this->crc16(buf , this->len_ - 2, ~(data->seed));

  this->whiten(buf + 2, this->len_ - 6, (uint8_t)(data->seed), 0);
  if (!std::equal(this->prefix_.begin(), this->prefix_.end(), buf)) return false;
  if (data->type != this->device_type_) return false;
  if (this->with_sign_ && data->sign == 0x0000) return false;
  if (!this->with_sign_ && data->sign != 0x0000) return false;

  std::string decoded = esphome::format_hex_pretty(buf, this->len_);
  ENSURE_EQ(crc16, data->crc16, "Decoded KO (crc16) - %s", decoded.c_str());

  if (this->with_sign_) {
    ENSURE_EQ(this->sign(buf + 1, data->tx_count, data->seed), data->sign, "Decoded KO (sign) - %s", decoded.c_str());
  }

  cmd.cmd_ = (CommandType) (data->command);
  std::copy(data->args, data->args + sizeof(data->args), cmd.args_);
  cont.tx_count_ = data->tx_count;
  cont.index_ = data->group_index;
  cont.id_ = data->identifier;
  cont.seed_ = data->seed;

  return true;
}

void FanLampEncoderV2::encode(uint8_t* buf, Command &cmd_real, ControllerParam_t & cont) {
  std::copy(this->prefix_.begin(), this->prefix_.end(), buf);
  data_map_t * data = (data_map_t *) (buf + this->prefix_.size());

  uint16_t seed = this->get_seed(cont.seed_);

  data->tx_count = cont.tx_count_;
  data->type = this->device_type_;
  data->identifier = cont.id_;
  data->command = cmd_real.cmd_;
  std::copy(cmd_real.args_, cmd_real.args_ + sizeof(cmd_real.args_), data->args);
  data->group_index = cont.index_;
  data->seed = seed;

  if (this->with_sign_) {
    data->sign = this->sign(buf + 1, data->tx_count, seed);
  }

  this->whiten(buf + 2, this->len_ - 6, (uint8_t) seed);
  data->crc16 = this->crc16(buf, this->len_ - 2, ~seed);
}

} // namespace bleadvcontroller
} // namespace esphome
