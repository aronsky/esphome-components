#include "fanlamp_pro.h"
#include "esphome/core/log.h"
#include <mbedtls/aes.h>
#include <arpa/inet.h>


namespace esphome {
namespace bleadvcontroller {

static const char *TAG = "fanlamp_pro";

#pragma pack(push, 1)
typedef struct {
  uint8_t prefix[5];
  uint8_t packet_number;
  uint16_t type;
  uint32_t identifier;
  uint8_t group_index;
  uint16_t command;
  uint8_t args[4];
  uint16_t sign;
  uint8_t spare;
  uint16_t rand;
  uint16_t crc16;
} man_data_v2_t;

typedef struct { /* Advertising Data for version 1*/
  uint8_t prefix[8];
  uint8_t command;
  uint16_t group_idx;
  uint8_t channel1;
  uint8_t channel2;
  uint8_t channel3;
  uint8_t tx_count;
  uint8_t outs;
  uint8_t src;
  uint8_t r2;
  uint16_t seed;
  uint16_t crc16;
} man_data_v1_t;
#pragma pack(pop)

static uint8_t HEADERv1a[2] = {0x77, 0xF8};
static uint8_t HEADERv1b[3] = {0xF9, 0x08, 0x49};
static uint8_t PREFIXv1[8] = {0xAA, 0x98, 0x43, 0xAF, 0x0B, 0x46, 0x46, 0x46};
static uint8_t PREFIXv2[5] = {0xF0, 0x08, 0x10, 0x80, 0x00};

static uint8_t XBOXES[128] = {
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

uint16_t sign(uint8_t* buf, uint8_t tx_count, uint16_t seed) {
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

void v2_whiten(uint8_t *buf, uint8_t size, uint8_t seed, uint8_t salt) {
  for (uint8_t i = 0; i < size; ++i) {
    buf[i] ^= XBOXES[(seed + i + 9) & 0x1f + (salt & 0x3) * 0x20];
    buf[i] ^= seed;
  }
}

uint16_t v2_crc16_ccitt(uint8_t *src, uint8_t size, uint16_t crc16_result) {
  for (uint8_t i = 0; i < size; ++i) {
    crc16_result = crc16_result ^ (*(uint16_t*) &src[i]) << 8;
    for (uint8_t j = 8; j != 0; --j) {
      if ((crc16_result & 0x8000) == 0) {
        crc16_result <<= 1;
      } else {
        crc16_result = crc16_result << 1 ^ 0x1021;
      }
    }
  }

  return crc16_result;
}

void v1_whiten(uint8_t *buf, size_t start, size_t len, uint8_t seed) {
  uint8_t r = seed;
  for (size_t i=0; i < start+len; i++) {
    uint8_t b = 0;
    for (size_t j=0; j < 8; j++) {
      r <<= 1;
      if (r & 0x80) {
        r ^= 0x11;
        b |= 1 << j;
      }
      r &= 0x7F;
    }
    if (i >= start) {
      buf[i - start] ^= b;
    }
    //ESP_LOGD(TAG, "%0x", b);
  }
}

uint8_t reverse_byte(uint8_t x) {

  x = ((x & 0x55) << 1) | ((x & 0xAA) >> 1);
  x = ((x & 0x33) << 2) | ((x & 0xCC) >> 2);
  x = ((x & 0x0F) << 4) | ((x & 0xF0) >> 4);
  return x;
}

bool FanLampEncoder::is_supported(const Command &cmd) {
  FanLampArgs cmd_real = translate_cmd(cmd);
  return (cmd_real.cmd_ != 0);
}

FanLampArgs FanLampEncoder::translate_cmd(const Command &cmd) {
  FanLampArgs cmd_real;
  bool isV2 = ((this->variant_ == VARIANT_2) || (this->variant_ == VARIANT_3));
  switch(cmd.cmd_)
  {
    case CommandType::PAIR:
      cmd_real.cmd_ = 0x28;
      break;
    case CommandType::UNPAIR:
      cmd_real.cmd_ = 0x45;
      break;
    case CommandType::CUSTOM:
      cmd_real.cmd_ = cmd.args_[0];
      cmd_real.args_[0] = cmd.args_[1];
      cmd_real.args_[1] = cmd.args_[2];
      cmd_real.args_[2] = cmd.args_[3];
      cmd_real.args_[3] = cmd.args_[4];
      break;
    case CommandType::LIGHT_ON:
      cmd_real.cmd_ = 0x10;
      break;
    case CommandType::LIGHT_OFF:
      cmd_real.cmd_ = 0x11;
      break;
    case CommandType::LIGHT_WCOLOR:
      cmd_real.cmd_ = 0x21;
      {
        // Correct minimum brightness in any case for FanLamp as it could switch off if too low
        // and this would create different state in between the light and this component
        uint8_t br_corrected = std::max(cmd.args_[1], (uint8_t)7);
        uint8_t cold = (br_corrected * (255 - cmd.args_[0])) / 255;
        uint8_t warm = (br_corrected * (cmd.args_[0])) / 255;
        if (isV2) {
          cmd_real.args_[2] = cold;
          cmd_real.args_[3] = warm;
        } else {
          cmd_real.args_[0] = cold;
          cmd_real.args_[1] = warm;
        }
      }
      break;
    case CommandType::LIGHT_SEC_ON:
      cmd_real.cmd_ = 0x12;
      break;
    case CommandType::LIGHT_SEC_OFF:
      cmd_real.cmd_ = 0x13;
      break;
    case CommandType::FAN_ONOFF_SPEED:
      if(isV2) {
        cmd_real.cmd_ = 0x31;
        cmd_real.args_[1] = (cmd.args_[1] == 6) ? 0x20 : 0; // specific flag for 6 level
        cmd_real.args_[2] = cmd.args_[0];
      } else {
        cmd_real.cmd_ = (cmd.args_[1] == 6) ? 0x32 : 0x31; // use Fan Gear or Fan Level
        cmd_real.args_[0] = cmd.args_[0];
        cmd_real.args_[1] = (cmd.args_[1] == 6) ? cmd.args_[1] : 0;
      }
      break;
    case CommandType::FAN_DIR:
      cmd_real.cmd_ = 0x15;
      if(isV2) {
        cmd_real.args_[1] = !cmd.args_[0];
      } else {
        cmd_real.args_[0] = !cmd.args_[0];
      }
      break;
    case CommandType::FAN_OSC:
      if(isV2) {
        cmd_real.cmd_ = 0x16;
        cmd_real.args_[1] = cmd.args_[0];
      }
      break;
    case CommandType::NOCMD:
    default:
      break;
  }
  return cmd_real;
}

void FanLampEncoder::build_packet_v1(uint8_t* buf, Command &cmd) {
  uint16_t seed = this->get_seed();
  man_data_v1_t *packet = (man_data_v1_t*)buf;
  std::copy(PREFIXv1, PREFIXv1 + sizeof(PREFIXv1), packet->prefix);
  FanLampArgs cmd_real = this->translate_cmd(cmd);
  packet->command = cmd_real.cmd_;
  packet->group_idx = static_cast<uint16_t>(cmd.id_ & 0xF0FF);
  packet->tx_count = cmd.tx_count_;
  packet->outs = 0;
  packet->src = static_cast<uint8_t>(seed ^ 1);
  packet->r2 = static_cast<uint8_t>(seed ^ 1);
  packet->seed = htons(seed);
  if (cmd.cmd_ == CommandType::PAIR) {
    packet->channel1 = static_cast<uint8_t>(packet->group_idx & 0xFF);
    packet->channel2 = static_cast<uint8_t>(packet->group_idx >> 8);
    packet->channel3 = 0x81;
  } else {
    packet->channel1 = cmd_real.args_[0];
    packet->channel2 = cmd_real.args_[1];
    packet->channel3 = cmd_real.args_[2];
  }
  packet->crc16 = htons(v2_crc16_ccitt(buf + 8, 12, ~seed));
  
  ESP_LOGD(TAG, "%s - ID: '0x%08X', tx: %d, Command: '0x%02X', Args: [%d,%d,%d]", this->id_.c_str(), cmd.id_, 
          packet->tx_count, packet->command, packet->channel1, packet->channel2, packet->channel3);
}

void FanLampEncoder::build_packet_v1a(uint8_t* buf, Command &cmd) {

  const size_t base = sizeof(HEADERv1a);
  const size_t size = 26;
  std::copy(HEADERv1a, HEADERv1a + base, buf);
  
  build_packet_v1(buf + base, cmd);
  
  uint16_t* crc16_2 = (uint16_t*) &buf[24];
  *crc16_2 = htons(v2_crc16_ccitt(buf + base + 8, 14, v2_crc16_ccitt(buf + base + 1, 5, 0xffff)));

  for (size_t i=base; i < size; i++) {
    buf[i] = reverse_byte(buf[i]);
  }
  // something strange here... the 'start' at 8+base is a nonsense as we are working on buffer AFTER base....
  //v1_whiten(buf + base, 8+base, size-base, 83);
  // kept the "logic" by hardcoding the value of base that was 7 in previous software, then 8+7=15
  v1_whiten(buf + base, 15, size-base, 83);
}

void FanLampEncoder::build_packet_v1b(uint8_t* buf, Command &cmd) {
  const size_t base = sizeof(HEADERv1b);
  const size_t size = 26;
  std::copy(HEADERv1b, HEADERv1b + base, buf);

  build_packet_v1(buf + base, cmd);
  
  buf[25] = 0xaa;
  for (size_t i=base; i < size; i++) {
    buf[i] = reverse_byte(buf[i]);
  }
  // something strange here... the 'start' at 8+base is a nonsense as we are working on buffer AFTER base....
  //v1_whiten(buf + base, 8+base, size-base, 83);
  // kept the "logic" by hardcoding the value of base that was 8 in previous software, then 8+8=16
  v1_whiten(buf + base, 16, size-base, 83);
}

void FanLampEncoder::build_packet_v2(uint8_t * buf, Command &cmd, bool with_sign) {
  uint16_t seed = this->get_seed();
  man_data_v2_t * packet = (man_data_v2_t *) buf;
  std::copy(PREFIXv2, PREFIXv2 + sizeof(PREFIXv2), packet->prefix);
  FanLampArgs cmd_real = this->translate_cmd(cmd);
  packet->packet_number = cmd.tx_count_;
  packet->type = 0x0100;
  packet->identifier = cmd.id_;
  packet->command = cmd_real.cmd_;
  std::copy(cmd_real.args_, cmd_real.args_ + sizeof(cmd_real.args_), packet->args);
  packet->group_index = 0;
  packet->rand = seed;

  ESP_LOGD(TAG, "%s - ID: '0x%08X', tx: %d, Command: '0x%02X', Args: [%d,%d,%d,%d]", this->id_.c_str(), cmd.id_, 
          packet->packet_number, packet->command, packet->args[0], packet->args[1], packet->args[2], packet->args[3]);

  if (with_sign) {
    packet->sign = sign(buf + 3, packet->packet_number, seed);
  }

  v2_whiten(buf + 4, 18, (uint8_t) seed, 0);
  packet->crc16 = v2_crc16_ccitt(buf + 2, 22, ~seed);
}

uint16_t FanLampEncoder::get_seed() {
  uint16_t seed = (uint16_t) rand() % 0xFFF5;
  return seed;
}

uint8_t FanLampEncoder::get_adv_data(std::vector< BleAdvParam > & params, Command &cmd) {
  params.emplace_back();
  BleAdvParam & param = params.back();
  switch (this->variant_) {
    case VARIANT_3:
      this->build_packet_v2(param.buf_, cmd, true);
      break;
    case VARIANT_2:
      this->build_packet_v2(param.buf_, cmd, false);
      break;
    case VARIANT_1A:
      this->build_packet_v1a(param.buf_, cmd);
      break;
    case VARIANT_1B:
      this->build_packet_v1b(param.buf_, cmd);
      break;
    default:
      ESP_LOGW(TAG, "get_adv_data called with invalid variant %d", this->variant_);
      break;
  }
  return 1;
}

void FanLampEncoder::register_encoders(BleAdvHandler * handler, const std::string & encoding) {
  BleAdvMultiEncoder * fanlamp_all = new BleAdvMultiEncoder("FanLamp - All", encoding);
  handler->add_encoder(fanlamp_all);
  BleAdvEncoder * fanlamp_1a = new FanLampEncoder("FanLamp - v1a", encoding, FanLampVariant::VARIANT_1A);
  handler->add_encoder(fanlamp_1a);
  fanlamp_all->add_encoder(fanlamp_1a);
  BleAdvEncoder * fanlamp_1b = new FanLampEncoder("FanLamp - v1b", encoding, FanLampVariant::VARIANT_1B);
  handler->add_encoder(fanlamp_1b);
  fanlamp_all->add_encoder(fanlamp_1b);
  BleAdvEncoder * fanlamp_v2 = new FanLampEncoder("FanLamp - v2", encoding, FanLampVariant::VARIANT_2);
  handler->add_encoder(fanlamp_v2);
  fanlamp_all->add_encoder(fanlamp_v2);
  BleAdvEncoder * fanlamp_v3 = new FanLampEncoder("FanLamp - v3", encoding, FanLampVariant::VARIANT_3);
  handler->add_encoder(fanlamp_v3);
  fanlamp_all->add_encoder(fanlamp_v3);
}

} // namespace bleadvcontroller
} // namespace esphome
