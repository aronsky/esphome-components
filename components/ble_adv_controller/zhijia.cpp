#include "zhijia.h"
#include "esphome/core/log.h"

namespace esphome {
namespace bleadvcontroller {

static constexpr size_t MAC_LEN = 4;
static uint8_t MAC[MAC_LEN] = {0x19, 0x01, 0x10, 0xAA};
static constexpr size_t UID_LEN = 3;
static uint8_t UID[UID_LEN] = {0x19, 0x01, 0x10};

uint16_t ZhijiaEncoder::crc16(uint8_t* buf, size_t len, uint16_t seed) {
  return esphome::crc16(buf, len, seed, 0x8408, true, true);
}

// {0xAB, 0xCD, 0xEF} => 0xABCDEF
uint32_t ZhijiaEncoder::uuid_to_id(uint8_t * uuid, size_t len) {
  uint32_t id = 0;
  for (size_t i = 0; i < len; ++i) {
    id |= uuid[len - i - 1] << (8*i);
  }
  return id;
}

// 0xABCDEF => {0xAB, 0xCD, 0xEF}
void ZhijiaEncoder::id_to_uuid(uint8_t * uuid, uint32_t id, size_t len) {
  for (size_t i = 0; i < len; ++i) {
    uuid[len - i - 1] = (id >> (8*i)) & 0xFF;
  }
}

std::vector< Command > ZhijiaEncoderV0::translate(const Command & cmd, const ControllerParam_t & cont) {
  Command cmd_real(cmd.main_cmd_);
  switch(cmd.main_cmd_)
  {
    case CommandType::PAIR:
      cmd_real.cmd_ = 0xB4;  // -76
      break;
    case CommandType::UNPAIR:
      cmd_real.cmd_ = 0xB0;  // -80
      break;
    case CommandType::LIGHT_ON:
      cmd_real.cmd_ = 0xB3;  // -77
      break;
    case CommandType::LIGHT_OFF:
      cmd_real.cmd_ = 0xB2;  // -78 
      break;
    case CommandType::LIGHT_DIM:
      {
        cmd_real.cmd_ = 0xB5; // -75
        // app software: int i in between 0 -> 1000
        // (byte) ((0xFF0000 & i) >> 16), (byte) ((0x00FF00 & i) >> 8), (byte) (i & 0x0000FF)
        uint16_t argBy4 = (1000 * (float)cmd.args_[0]) / 255; // from 0..255 -> 0..1000
        cmd_real.args_[1] = ((argBy4 & 0xFF00) >> 8);
        cmd_real.args_[2] = (argBy4 & 0x00FF);
      }
      break;
    case CommandType::LIGHT_CCT:
      {
        cmd_real.cmd_ = 0xB7; // -73
        // app software: int i in between 0 -> 1000
        // (byte) ((0xFF0000 & i) >> 16), (byte) ((0x00FF00 & i) >> 8), (byte) (i & 0x0000FF)
        uint16_t argBy4 = (1000 * (float)cmd.args_[0]) / 255; 
        cmd_real.args_[1] = ((argBy4 & 0xFF00) >> 8);
        cmd_real.args_[2] = (argBy4 & 0x00FF);
      }
      break;
    case CommandType::LIGHT_SEC_ON:
      cmd_real.cmd_ = 0xA6; // -90
      cmd_real.args_[0] = 1;
      break;
    case CommandType::LIGHT_SEC_OFF:
      cmd_real.cmd_ = 0xA6; // -90
      cmd_real.args_[0] = 2;
      break;
    default:
      break;
  }
  std::vector< Command > cmds;
  if(cmd_real.cmd_ != 0x00) {
    cmds.emplace_back(cmd_real);
  }
  return cmds;
}

bool ZhijiaEncoderV0::decode(uint8_t* buf, Command &cmd, ControllerParam_t & cont) {
  this->whiten(buf, this->len_, 0x37);
  this->whiten(buf, this->len_, 0x7F);

  data_map_t * data = (data_map_t *) buf;
  uint16_t crc16 = this->crc16(buf, ADDR_LEN + TXDATA_LEN);
  ENSURE_EQ(crc16, data->crc16, "Decoded KO (CRC)");

  uint8_t addr[ADDR_LEN];
  this->reverse_all(buf, ADDR_LEN);
  std::reverse_copy(data->addr, data->addr + ADDR_LEN, addr);
  ENSURE_EQ(std::equal(addr, addr + ADDR_LEN, MAC), true, "Decoded KO (MAC)");

  cont.tx_count_ = data->txdata[0] ^ data->txdata[6];
  cmd.args_[0] = cont.tx_count_ ^ data->txdata[7];
  uint8_t pivot = data->txdata[1] ^ cmd.args_[0];
  uint8_t uuid[UUID_LEN];
  uuid[0] = pivot ^ data->txdata[0];
  uuid[1] = pivot ^ data->txdata[5];
  cont.id_ = this->uuid_to_id(uuid, UUID_LEN);
  cont.index_ = pivot ^ data->txdata[2];
  cmd.cmd_ = pivot ^ data->txdata[4];
  cmd.args_[1] = pivot ^ data->txdata[3];
  cmd.args_[2] = uuid[0] ^ data->txdata[6];

  return true;
}

void ZhijiaEncoderV0::encode(uint8_t* buf, Command &cmd_real, ControllerParam_t & cont) {
  unsigned char uuid[UUID_LEN] = {0};
  this->id_to_uuid(uuid, cont.id_, UUID_LEN);

  data_map_t * data = (data_map_t *) buf;
  std::reverse_copy(MAC, MAC + ADDR_LEN, data->addr);
  this->reverse_all(data->addr, ADDR_LEN);

  uint8_t pivot = cmd_real.args_[2] ^ cont.tx_count_;
  data->txdata[0] = pivot ^ uuid[0];
  data->txdata[1] = pivot ^ cmd_real.args_[0];
  data->txdata[2] = pivot ^ cont.index_;
  data->txdata[3] = pivot ^ cmd_real.args_[1];
  data->txdata[4] = pivot ^ cmd_real.cmd_;
  data->txdata[5] = pivot ^ uuid[1];
  data->txdata[6] = cmd_real.args_[2] ^ uuid[0];
  data->txdata[7] = cmd_real.args_[0] ^ cont.tx_count_;

  data->crc16 = this->crc16(buf, ADDR_LEN + TXDATA_LEN);
  this->whiten(buf, this->len_, 0x7F);
  this->whiten(buf, this->len_, 0x37);
}

std::vector< Command > ZhijiaEncoderV1::translate(const Command & cmd, const ControllerParam_t & cont) {
  Command cmd_real(cmd.main_cmd_);
  switch(cmd.main_cmd_)
  {
    case CommandType::PAIR:
      cmd_real.cmd_ = 0xA2;  // -94
      break;
    case CommandType::UNPAIR:
      cmd_real.cmd_ = 0xA3;  // -93
      break;
    case CommandType::LIGHT_ON:
      cmd_real.cmd_ = 0xA5;  // -91
      break;
    case CommandType::LIGHT_OFF:
      cmd_real.cmd_ = 0xA6;  // -90
      break;
    case CommandType::LIGHT_WCOLOR:
      cmd_real.cmd_ = 0xA8; // -88
      cmd_real.args_[0] = (250 * (float)cmd.args_[1]) / 255;
      cmd_real.args_[1] = (250 * (float)cmd.args_[0]) / 255;
      break;
    case CommandType::LIGHT_DIM:
      cmd_real.cmd_ = 0xAD; // -83
      // app software: value in between 0 -> 250
      cmd_real.args_[0] = (250 * (float)cmd.args_[0]) / 255;
      break;
    case CommandType::LIGHT_CCT:
      cmd_real.cmd_ = 0xAE; // -82
      // app software: value in between 0 -> 250
      cmd_real.args_[0] = (250 * (float)cmd.args_[0]) / 255;
      break;
    case CommandType::LIGHT_SEC_ON:
      cmd_real.cmd_ = 0xAF; // -81
      break;
    case CommandType::LIGHT_SEC_OFF:
      cmd_real.cmd_ = 0xB0; // -80
      break;
    default:
      break;
  }
  std::vector< Command > cmds;
  if(cmd_real.cmd_ != 0x00) {
    cmds.emplace_back(cmd_real);
  }
  return cmds;
}

bool ZhijiaEncoderV1::decode(uint8_t* buf, Command &cmd, ControllerParam_t & cont) {
  this->whiten(buf, this->len_, 0x37);

  data_map_t * data = (data_map_t *) buf;
  uint16_t crc16 = this->crc16(buf, ADDR_LEN + TXDATA_LEN);
  ENSURE_EQ(crc16, data->crc16, "Decoded KO (CRC)");

  uint8_t addr[ADDR_LEN];
  this->reverse_all(data->addr, ADDR_LEN);
  std::reverse_copy(data->addr, data->addr + ADDR_LEN, addr);
  ENSURE_EQ(std::equal(addr, addr + ADDR_LEN, addr), true, "Decoded KO (MAC)");

  ENSURE_EQ(data->txdata[7], data->txdata[14], "Decoded KO (Dupe 7/14)");
  ENSURE_EQ(data->txdata[8], data->txdata[11], "Decoded KO (Dupe 8/11)");
  ENSURE_EQ(data->txdata[11], data->txdata[16], "Decoded KO (Dupe 11/16)");

  uint8_t pivot = data->txdata[16];
  uint8_t uid[UID_LEN];
  uid[0] = data->txdata[7] ^ pivot;
  uid[1] = data->txdata[10] ^ pivot;
  uid[2] = data->txdata[4] ^ data->txdata[13];
  ENSURE_EQ(std::equal(uid, uid + UID_LEN, UID), true, "Decoded KO (UID)");

  cmd.cmd_ = (CommandType) (data->txdata[9] ^ pivot);
  cmd.args_[0] = data->txdata[0] ^ pivot;
  cmd.args_[1] = data->txdata[3] ^ pivot;
  cmd.args_[2] = data->txdata[5] ^ pivot;
  cont.tx_count_ = data->txdata[4] ^ pivot;
  cont.index_ = data->txdata[6] ^ pivot;
  uint8_t uuid[UUID_LEN];
  uuid[0] = data->txdata[2] ^ pivot;
  uuid[1] = data->txdata[2] ^ data->txdata[12];
  uuid[2] = data->txdata[9] ^ data->txdata[15];
  cont.id_ = this->uuid_to_id(uuid, UUID_LEN);

  uint8_t key = pivot ^ cmd.args_[0] ^ cmd.args_[1] ^ cmd.args_[2] ^ uid[0] ^ uid[1] ^ uid[2];
  key ^= uuid[0] ^ uuid[1] ^ uuid[2] ^ cont.tx_count_ ^ cont.index_ ^ cmd.cmd_;
  ENSURE_EQ(key, data->txdata[1], "Decoded KO (Key)");
  
  uint8_t re_pivot = uuid[1] ^ uuid[2] ^ uid[2];
  re_pivot ^= ((re_pivot & 1) - 1);
  ENSURE_EQ(pivot, re_pivot, "Decoded KO (Pivot)");

  return true;
}

void ZhijiaEncoderV1::encode(uint8_t* buf, Command &cmd_real, ControllerParam_t & cont) {
  unsigned char uuid[UUID_LEN] = {0};
  this->id_to_uuid(uuid, cont.id_, UUID_LEN);

  data_map_t * data = (data_map_t *) buf;
  std::reverse_copy(MAC, MAC + ADDR_LEN, data->addr);
  this->reverse_all(data->addr, ADDR_LEN);

  uint8_t pivot = uuid[1] ^ uuid[2] ^ UID[2];
  pivot ^= (pivot & 1) - 1;

  uint8_t key = cmd_real.args_[0] ^ cmd_real.args_[1] ^ cmd_real.args_[2];
  key ^=  uuid[0] ^ uuid[1] ^ uuid[2] ^ cont.tx_count_ ^ cont.index_ ^ cmd_real.cmd_ ^ UID[0] ^ UID[1] ^ UID[2];

  data->txdata[0] = pivot ^ cmd_real.args_[0];
  data->txdata[1] = pivot ^ key;
  data->txdata[2] = pivot ^ uuid[0];
  data->txdata[3] = pivot ^ cmd_real.args_[1];
  data->txdata[4] = pivot ^ cont.tx_count_;
  data->txdata[5] = pivot ^ cmd_real.args_[2];
  data->txdata[6] = pivot ^ cont.index_;
  data->txdata[7] = pivot ^ UID[0];
  data->txdata[8] = pivot;
  data->txdata[9] = pivot ^ cmd_real.cmd_;
  data->txdata[10] = pivot ^ UID[1];
  data->txdata[11] = pivot;
  data->txdata[12] = uuid[1] ^ data->txdata[2];
  data->txdata[13] = UID[2] ^ data->txdata[4];
  data->txdata[14] = data->txdata[7];
  data->txdata[15] = uuid[2] ^ data->txdata[9];
  data->txdata[16] = pivot;

  data->crc16 = this->crc16(buf, ADDR_LEN + TXDATA_LEN);
  this->whiten(buf, this->len_, 0x37);
}

std::vector< Command > ZhijiaEncoderV2::translate(const Command & cmd, const ControllerParam_t & cont) {
  auto cmds = ZhijiaEncoderV1::translate(cmd, cont);
  if (!cmds.empty()) return cmds;

  Command cmd_real(cmd.main_cmd_);
  switch(cmd.main_cmd_)
  {
    case CommandType::FAN_ON:
      cmd_real.cmd_ = 0xD2; // -47
      break;
    case CommandType::FAN_OFF:
      cmd_real.cmd_ = 0xD3; // -46
      break;
    case CommandType::FAN_SPEED:
       // -37 + speed(1..6) => -36 -> -31
      cmd_real.cmd_ = 0xDB + ((cmd.args_[1] == 3) ? 2 * cmd.args_[0] : cmd.args_[0]);
      break;
    default:
      break;
  }
  if(cmd_real.cmd_ != 0x00) {
    cmds.emplace_back(cmd_real);
  }
  return cmds;
}

bool ZhijiaEncoderV2::decode(uint8_t* buf, Command &cmd, ControllerParam_t & cont) {
  this->whiten(buf, this->len_, 0x6F);
  this->whiten(buf, this->len_ - 2, 0xD3);

  data_map_t * data = (data_map_t *) buf;
  for (size_t i = 0; i < TXDATA_LEN; ++i) {
    data->txdata[i] ^= data->pivot;
  }

  cont.tx_count_ = data->txdata[4];
  cont.index_ = data->txdata[6];
  cmd.cmd_ = (CommandType) (data->txdata[9]);
  uint8_t addr[ADDR_LEN];
  addr[0] = data->txdata[7];
  addr[1] = data->txdata[10];
  addr[2] = data->txdata[13] ^ cont.tx_count_;
  cmd.args_[0] = data->txdata[0];
  cmd.args_[1] = data->txdata[3];
  cmd.args_[2] = data->txdata[5];
  uint8_t uuid[UUID_LEN]{0};
  uuid[0] = data->txdata[2];
  uuid[1] = data->txdata[12] ^ uuid[0];
  uuid[2] = data->txdata[15] ^ cmd.cmd_;
  cont.id_ = this->uuid_to_id(uuid, UUID_LEN);

  ENSURE_EQ(std::equal(addr, addr + ADDR_LEN, MAC), true, "Decoded KO (MAC)");

  uint8_t key = addr[0] ^ addr[1] ^ addr[2] ^ cont.index_ ^ cont.tx_count_ ^ cmd.args_[0] ^ cmd.args_[1] ^ cmd.args_[2] ^ uuid[0] ^ uuid[1] ^ uuid[2];
  ENSURE_EQ(key, data->txdata[1], "Decoded KO (Key)");
  
  uint8_t re_pivot = uuid[0] ^ uuid[1] ^ uuid[2] ^ cont.tx_count_ ^ cmd.args_[1] ^ addr[0] ^ addr[2] ^ cmd.cmd_;
  re_pivot = ((re_pivot & 1) - 1) ^ re_pivot;
  ENSURE_EQ(data->pivot, re_pivot, "Decoded KO (Pivot)");

  ENSURE_EQ(data->txdata[8], uuid[0] ^ cont.tx_count_ ^ cmd.args_[1] ^ addr[0], "Decoded KO (txdata 8)");
  ENSURE_EQ(data->txdata[11], 0x00, "Decoded KO (txdata 11)");
  ENSURE_EQ(data->txdata[14], uuid[0] ^ cont.tx_count_ ^ cmd.args_[1] ^ cmd.cmd_, "Decoded KO (txdata 14)");

  return true;
}

void ZhijiaEncoderV2::encode(uint8_t* buf, Command &cmd_real, ControllerParam_t & cont) {
  unsigned char uuid[UUID_LEN] = {0};
  this->id_to_uuid(uuid, cont.id_, UUID_LEN);

  data_map_t * data = (data_map_t *) buf;
  uint8_t key = MAC[0] ^ MAC[1] ^ MAC[2] ^ cont.index_ ^ cont.tx_count_;
  key ^= cmd_real.args_[0] ^ cmd_real.args_[1] ^ cmd_real.args_[2] ^ uuid[0] ^ uuid[1] ^ uuid[2];

  data->pivot = uuid[0] ^ uuid[1] ^ uuid[2] ^ cont.tx_count_ ^ cmd_real.args_[1] ^ MAC[0] ^ MAC[2] ^ cmd_real.cmd_;
  data->pivot = ((data->pivot & 1) - 1) ^ data->pivot;

  data->txdata[0] = cmd_real.args_[0];
  data->txdata[1] = key;
  data->txdata[2] = uuid[0];
  data->txdata[3] = cmd_real.args_[1];
  data->txdata[4] = cont.tx_count_;
  data->txdata[5] = cmd_real.args_[2];
  data->txdata[6] = cont.index_;
  data->txdata[7] = MAC[0];
  data->txdata[8] = uuid[0] ^ cont.tx_count_ ^ cmd_real.args_[1] ^ MAC[0];
  data->txdata[9] = cmd_real.cmd_;
  data->txdata[10] = MAC[1];
  data->txdata[11] = 0x00;
  data->txdata[12] = uuid[1] ^ uuid[0];
  data->txdata[13] = MAC[2] ^ cont.tx_count_;
  data->txdata[14] = uuid[0] ^ cont.tx_count_ ^ cmd_real.args_[1] ^ cmd_real.cmd_;
  data->txdata[15] = uuid[2] ^ cmd_real.cmd_;

  for (size_t i = 0; i < TXDATA_LEN; ++i) {
    data->txdata[i] ^= data->pivot;
  }
  
  this->whiten(buf, this->len_ - 2, 0xD3);
  this->whiten(buf, this->len_, 0x6F);
}

} // namespace bleadvcontroller
} // namespace esphome
