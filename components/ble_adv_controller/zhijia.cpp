#include "zhijia.h"
#include "esphome/core/log.h"

namespace esphome {
namespace bleadvcontroller {

static const char *TAG = "zhijia";
static unsigned char MAC[] = {0x19, 0x01, 0x10, 0xAA};

/*********************
START OF MSC16_MSC26 encoder
**********************/
namespace msc16_msc26 {

// Reverse byte
uint8_t reverse_8(uint8_t d)
{
    uint8_t result = 0;
    for (int k = 0; k < 8; k++)
    {
        result |= ((d >> k) & 1) << (7 - k);
    }
    return result;
}

// Reverse 16-bit number
uint16_t reverse_16(uint16_t d)
{
    uint16_t result = 0;
    for (int k = 0; k < 16; k++)
    {
        result |= ((d >> k) & 1) << (15 - k);
    }
    return result;
}

// CRC-16 calculation
uint16_t crc16(uint8_t *addr, size_t addr_length, uint8_t *data, size_t data_length)
{
    uint16_t crc = 0xFFFF;

    for (int i = addr_length - 1; i >= 0; i--)
    {
        crc ^= addr[i] << 8;
        for (int j = 0; j < 8; j++)
        {
            uint16_t tmp = crc << 1;
            if (crc & 0x8000)
                tmp ^= 0x1021;
            crc = tmp;
        }
    }

    for (size_t i = 0; i < data_length; i++)
    {
        crc ^= reverse_8(data[i]) << 8;
        for (int j = 0; j < 8; j++)
        {
            uint16_t tmp = crc << 1;
            if (crc & 0x8000)
                tmp ^= 0x1021;
            crc = tmp;
        }
    }

    return (~reverse_16(crc)) & 0xFFFF;
}

// Whitening initialization
void whitening_init(uint8_t val, int *ctx, size_t ctx_length)
{
    ctx[0] = 1;
    ctx[1] = (val >> 5) & 1;
    ctx[2] = (val >> 4) & 1;
    ctx[3] = (val >> 3) & 1;
    ctx[4] = (val >> 2) & 1;
    ctx[5] = (val >> 1) & 1;
    ctx[6] = val & 1;
}

// Whitening encoding
void whitening_encode(uint8_t *data, size_t length, int *ctx)
{
    for (size_t i = 0; i < length; i++)
    {
        uint8_t varC = ctx[3];
        uint8_t var14 = ctx[5];
        uint8_t var18 = ctx[6];
        uint8_t var10 = ctx[4];
        uint8_t var8 = var14 ^ ctx[2];
        uint8_t var4 = var10 ^ ctx[1];
        uint8_t _var = var18 ^ varC;
        uint8_t var0 = _var ^ ctx[0];

        uint8_t c = data[i];
        data[i] = ((c & 0x80) ^ ((var8 ^ var18) << 7)) |
                  ((c & 0x40) ^ (var0 << 6)) |
                  ((c & 0x20) ^ (var4 << 5)) |
                  ((c & 0x10) ^ (var8 << 4)) |
                  ((c & 0x08) ^ (_var << 3)) |
                  ((c & 0x04) ^ (var10 << 2)) |
                  ((c & 0x02) ^ (var14 << 1)) |
                  ((c & 0x01) ^ (var18));

        // Update context
        ctx[2] = var4;
        ctx[3] = var8;
        ctx[4] = var8 ^ varC;
        ctx[5] = var0 ^ var10;
        ctx[6] = var4 ^ var14;
        ctx[0] = var8 ^ var18;
        ctx[1] = var0;
    }
}

// Adjusted to include length parameters for addr and data arrays
void get_rf_payload16(uint8_t *addr, size_t addr_length, uint8_t *data, size_t data_length, uint8_t *output)
{
    const size_t data_offset = 0x12;
    const size_t inverse_offset = 0x0F;
    const size_t result_data_size = data_offset + addr_length + data_length;
    uint8_t *resultbuf = (uint8_t *)calloc(result_data_size + 2, sizeof(uint8_t));

    // Hardcoded values
    resultbuf[0x0F] = 0x71;
    resultbuf[0x10] = 0x0F;
    resultbuf[0x11] = 0x55;

    // Reverse copy the address
    for (size_t i = 0; i < addr_length; i++)
    {
        resultbuf[data_offset + addr_length - i - 1] = addr[i];
    }

    // Copy data
    memcpy(&resultbuf[data_offset + addr_length], data, data_length);

    // Reverse certain bytes
    for (size_t i = inverse_offset; i < inverse_offset + addr_length + 3; i++)
    {
        resultbuf[i] = reverse_8(resultbuf[i]);
    }

    // CRC
    uint16_t crc = crc16(addr, addr_length, data, data_length);
    resultbuf[result_data_size] = crc & 0xFF;
    resultbuf[result_data_size + 1] = (crc >> 8) & 0xFF;

    // Whitening
    int whiteningBLE[7];
    whitening_init(0x3F, whiteningBLE, 7);
    whitening_encode(&resultbuf[data_offset], result_data_size + 2 - data_offset, whiteningBLE);
    int whiteningContext[7];
    whitening_init(0x25, whiteningContext, 7);
    whitening_encode(&resultbuf[2], result_data_size + 2 - 2, whiteningContext);

    // Returning the last 16 bytes of the payload
    memcpy(output, &resultbuf[result_data_size - 16 + 2], 16);

    free(resultbuf);
}
void get_rf_payload26(uint8_t *addr, size_t addr_length, uint8_t *data, size_t data_length, uint8_t *output)
{
    const size_t data_offset = 0x12;
    const size_t inverse_offset = 0x0F;
    const size_t result_data_size = data_offset + addr_length + data_length;
    uint8_t *resultbuf = (uint8_t *)calloc(result_data_size + 2, sizeof(uint8_t));

    // Hardcoded values
    resultbuf[0x0F] = 0x71;
    resultbuf[0x10] = 0x0F;
    resultbuf[0x11] = 0x55;

    // Reverse copy the address
    for (size_t i = 0; i < addr_length; i++)
    {
        resultbuf[data_offset + addr_length - i - 1] = addr[i];
    }

    // Copy data
    memcpy(&resultbuf[data_offset + addr_length], data, data_length);

    // Reverse certain bytes
    for (size_t i = inverse_offset; i < inverse_offset + addr_length + 3; i++)
    {
        resultbuf[i] = reverse_8(resultbuf[i]);
    }

    // CRC
    uint16_t crc = crc16(addr, addr_length, data, data_length);
    resultbuf[result_data_size] = crc & 0xFF;
    resultbuf[result_data_size + 1] = (crc >> 8) & 0xFF;

    // Whitening
    int whiteningContext[7];
    whitening_init(0x25, whiteningContext, 7);

    whitening_encode(&resultbuf[2], result_data_size + 2 - 2, whiteningContext);

    // Returning the last 26 bytes of the payload
    // uint8_t* final_payload = (uint8_t*)malloc(26 * sizeof(uint8_t));
    memcpy(output, &resultbuf[result_data_size - 24], 26);

    free(resultbuf);
}

void aes16Encrypt16(unsigned char *mac, unsigned char *uuid, unsigned char groupId, unsigned char cmd, unsigned char sn, unsigned char *data,
                    unsigned char *outputData)
{
    unsigned char key;
    unsigned char txData[8];

    txData[5] = data[2] ^ sn;
    txData[6] = data[2] ^ *uuid;
    txData[7] = *data ^ sn;
    txData[0] = txData[5] ^ *uuid;
    txData[1] = txData[5] ^ *data;
    txData[2] = txData[5] ^ groupId;
    txData[3] = txData[5] ^ data[1];
    txData[4] = txData[5] ^ cmd;
    txData[5] = (txData[5] ^ uuid[1]) - 1;
    get_rf_payload16(mac, 3, txData, 8, outputData);
}

void aes26Encrypt(uint8_t *mac, uint8_t *uuid, uint8_t *uid, uint8_t groupId, uint8_t cmd, uint8_t sn, uint8_t *data,
                  uint8_t *outputData)

{
    uint8_t bVar1;
    uint8_t bVar2;
    uint8_t bVar3;
    uint8_t key3;
    uint8_t key4;
    uint8_t txData[17];

    bVar1 = uuid[1];
    bVar2 = uid[2];
    bVar3 = uuid[2];
    txData[8] = bVar2 ^ bVar1 ^ bVar3;
    txData[8] = ((txData[8] & 1) - 1) ^ txData[8];
    txData[0] = txData[8] ^ *data;
    txData[2] = txData[8] ^ *uuid;
    txData[3] = txData[8] ^ data[1];
    txData[4] = txData[8] ^ sn;
    txData[12] = bVar1 ^ txData[2];
    txData[13] = bVar2 ^ txData[4];
    txData[5] = txData[8] ^ data[2];
    txData[6] = txData[8] ^ groupId;
    txData[9] = txData[8] ^ cmd;
    txData[7] = txData[8] ^ *uid;
    txData[10] = txData[8] ^ uid[1];
    txData[15] = bVar3 ^ txData[9];
    txData[1] = txData[8] ^
                *data ^ *uuid ^ data[1] ^ sn ^ data[2] ^ groupId ^ *uid ^ cmd ^ uid[1] ^ bVar1 ^ bVar2 ^ bVar3;
    txData[11] = txData[8];
    txData[14] = txData[7];
    txData[16] = txData[8];
    get_rf_payload26(mac, 4, &txData[0], 0x11, outputData);
}

}
/*********************
END OF MSC16_MSC26 encoder
**********************/

/**********************
START OF MSC26A encoder
**********************/
namespace msc26a {

void stage1(const unsigned char *x0, const unsigned char w20, const unsigned char w21,
            const unsigned char w22, const unsigned char *x23, const unsigned char *x24,
            unsigned char *buffer) {
  unsigned short w8 = 0xa6aa;
  size_t xzr = 0;
  unsigned char sp[0x100] = {0};
  unsigned short wzr = 0;
  size_t x8 = 0xa6aa;
  unsigned char w11, w13, w14, w15, w16, w17, w1, w12, w9, w10, w0, w3, w2, w4,
      w6, w5;

  w8 = 0xa6aa;
  // sp = sp - 0x30;
  *(size_t *)(sp + 0x40) = wzr;
  *(size_t *)(sp + 0x28) = x8;
  w11 = *(unsigned char *)(x24);
  w13 = *(unsigned char *)(x0 + 1);
  w14 = *(unsigned char *)(x0 + 2);
  w15 = *(unsigned char *)(x23);
  w16 = w11 ^ w21;
  w17 = w11 ^ w13;
  w8 = *(unsigned char *)(x24 + 1);
  w1 = w16 ^ w13;
  w16 = w17 ^ w21;
  w12 = *(unsigned char *)(x23 + 1);
  w9 = *(unsigned char *)(x23 + 2);
  w2 = w16 ^ w14;
  w10 = *(unsigned char *)(x24 + 2);
  w17 = w1 ^ w15;
  w2 = w2 ^ w22;
  w16 = w17 ^ w20;
  w1 = w2 ^ w1;
  w2 = w8 ^ w16;
  w1 = w1 ^ w20;
  w2 = w2 ^ w9;
  w1 = w1 ^ w12;
  w2 = w2 ^ w10;
  w0 = *(unsigned char *)(x0);
  w1 = w1 ^ w8;
  w3 = w2 & 1;
  w1 = w1 ^ w9;
  w3 = w3 - 1;
  w1 = w1 ^ w16;
  w2 = w3 ^ w2;
  w1 = w1 ^ w10;
  w3 = *(unsigned char *)(sp + 0x35);
  w1 = w0 ^ w1;
  w0 = w2 ^ w0;
  *(unsigned char *)(sp + 0x2a) = w0;
  w0 = *(unsigned char *)(sp + 0x3a);
  w11 = w2 ^ w11;
  w13 = w2 ^ w13;
  w4 = w2 ^ w21;
  w15 = w2 ^ w15;
  w6 = w2 ^ w20;
  w12 = w2 ^ w12;
  w3 = w2 ^ w3;
  w14 = w2 ^ w14;
  w5 = w2 ^ w22;
  w17 = w2 ^ w17;
  *(unsigned char *)(sp + 0x2c) = w11;
  *(unsigned char *)(sp + 0x2d) = w13;
  *(unsigned char *)(sp + 0x34) = w12;
  *(unsigned char *)(sp + 0x35) = w3;
  w8 = w8 ^ w11;
  w9 = w9 ^ w4;
  w11 = w16 ^ w15;
  w10 = w10 ^ w6;
  w12 = w0 ^ w3;
  w13 = w2 ^ w1;
  x0 = sp + 0x28;
  *(size_t *)(sp + 0x10) = xzr;
  *(size_t *)(sp + 8) = xzr;
  *(unsigned char *)(sp + 0x2e) = w4;
  *(unsigned char *)(sp + 0x2f) = w14;
  *(unsigned char *)(sp + 0x30) = w5;
  *(unsigned char *)(sp + 0x31) = w15;
  *(unsigned char *)(sp + 0x32) = w17;
  *(unsigned char *)(sp + 0x33) = w6;
  *(unsigned char *)(sp + 0x36) = w8;
  *(unsigned char *)(sp + 0x37) = w9;
  *(unsigned char *)(sp + 0x38) = w11;
  *(unsigned char *)(sp + 0x39) = w10;
  *(unsigned char *)(sp + 0x3a) = w12;
  *(unsigned char *)(sp + 0x2b) = w13;
  *(unsigned short *)(sp + 0x20) = wzr;

  memcpy(buffer, (sp + 0x28), 0x18);
}

unsigned char *stage2(unsigned char *d, unsigned int n, unsigned int md) {
  int8_t cVar1;
  uint8_t *pbVar2;
  unsigned int in_w3 = 0xa6;
  long lVar3;
  unsigned int uVar4;
  unsigned int uVar5;

  if (((unsigned int)md & 0xff) < (unsigned int)(uint8_t)n) {
    lVar3 = (n & 0xff) - ((size_t)md & 0xff);
    cVar1 = (int8_t)in_w3;
    pbVar2 = d + ((size_t)md & 0xff);
    while (1) {
      if (cVar1 < '\0') {
        in_w3 ^= 0x11;
        *pbVar2 = *pbVar2 ^ 1;
      }
      uVar5 = (unsigned int)(int8_t)(in_w3 << 1);
      if ((int)uVar5 < 0) {
        uVar5 ^= 0x11;
        *pbVar2 = *pbVar2 ^ 2;
      }
      uVar5 = (unsigned int)(int8_t)(uVar5 << 1);
      if ((int)uVar5 < 0) {
        uVar5 ^= 0x11;
        *pbVar2 = *pbVar2 ^ 4;
      }
      uVar5 = (unsigned int)(int8_t)(uVar5 << 1);
      if ((int)uVar5 < 0) {
        uVar5 ^= 0x11;
        *pbVar2 = *pbVar2 ^ 8;
      }
      uVar5 = (unsigned int)(int8_t)(uVar5 << 1);
      if ((int)uVar5 < 0) {
        uVar5 ^= 0x11;
        *pbVar2 = *pbVar2 ^ 0x10;
      }
      uVar5 = (unsigned int)(int8_t)(uVar5 << 1);
      if ((int)uVar5 < 0) {
        uVar5 ^= 0x11;
        *pbVar2 = *pbVar2 ^ 0x20;
      }
      uVar5 = (unsigned int)(int8_t)(uVar5 << 1);
      if ((int)uVar5 < 0) {
        uVar5 ^= 0x11;
        *pbVar2 = *pbVar2 ^ 0x40;
      }
      uVar5 = (unsigned int)(int8_t)(uVar5 << 1);
      if ((int)uVar5 < 0) {
        uVar5 ^= 0x11;
        uVar4 = *pbVar2 ^ 0xffffff80;
        *pbVar2 = (uint8_t)uVar4;
      } else {
        uVar4 = (unsigned int)*pbVar2;
      }
      in_w3 = uVar5 << 1;
      lVar3 += -1;
      *pbVar2 =
          (uint8_t)((uVar4 >> 7) & 1) |
          (uint8_t)(((uVar4 >> 6 & 1) |
                     ((uVar4 >> 5 & 1) |
                      (((uVar4 & 0xff) >> 4 & 1) |
                       ((uVar4 >> 3 & 1) |
                        ((uVar4 & 2) | (uVar4 & 1) << 2 | ((uVar4 & 0xff) >> 2 & 1))
                            << 1)
                           << 1)
                          << 1)
                         << 1)
                    << 1);
      if (lVar3 == 0)
        break;
      cVar1 = (int8_t)in_w3;
      pbVar2 = pbVar2 + 1;
    }
  }
  return d;
}

void whitening_init(unsigned char value, unsigned int *seed) {
  unsigned int *puVar1;
  unsigned int uVar2;

  *seed = 1;
  puVar1 = seed + 1;
  for (uVar2 = 5; uVar2 != 0xffffffff; uVar2 -= 1) {
    *puVar1 = value >> (uVar2 & 0xff) & 1;
    puVar1++;
  }
  return;
}

unsigned char invert_8(unsigned char param_1) {
  unsigned int uVar1;
  unsigned int uVar2;
  unsigned int uVar3;

  uVar2 = 7;
  uVar3 = 0;
  for (uVar1 = 0; uVar1 != 8; uVar1 += 1) {
    if ((1 << (uVar1 & 0xff) & param_1) != 0) {
      uVar3 |= 1 << (uVar2 & 0xff);
    }
    uVar2 -= 1;
  }
  return uVar3 & 0xff;
}

unsigned int whitening_output(unsigned int *param_1) {
  unsigned int uVar1;
  unsigned int uVar2;
  unsigned int uVar3;

  uVar1 = param_1[1];
  uVar2 = param_1[2];
  uVar3 = param_1[3];
  param_1[1] = *param_1;
  param_1[2] = uVar1;
  param_1[3] = uVar2;
  uVar1 = param_1[6];
  *param_1 = uVar1;
  uVar2 = param_1[5];
  param_1[5] = param_1[4];
  param_1[6] = uVar2;
  param_1[4] = uVar1 ^ uVar3;
  return uVar1;
}

void whitening_encode(unsigned char *buffer, int length, unsigned int *seed) {
  unsigned char bVar1;
  unsigned int uVar2;
  unsigned int uVar3;
  int iVar4;
  int iVar5;

  for (iVar5 = 0; iVar5 < length; iVar5 += 1) {
    bVar1 = *(unsigned char *)(buffer + iVar5);
    iVar4 = 0;
    for (uVar3 = 0; uVar3 != 8; uVar3 += 1) {
      uVar2 = whitening_output(seed);
      iVar4 += ((uVar2 ^ bVar1 >> (uVar3 & 0xff)) & 1) << (uVar3 & 0xff);
    }
    *(char *)(buffer + iVar5) = (char)iVar4;
  }
  return;
}

void stage3(unsigned char *buffer, size_t length, unsigned char *dest) {
  unsigned char whitened[39];
  unsigned char seed[28] = {0};

  whitening_init(0x25, (unsigned int *)seed);

  for (int i = 0; i < length; ++i) {
    whitened[13 + i] = invert_8(buffer[i]);
  }

  whitening_encode(whitened, length + 0xd, (unsigned int *)seed);

  for (int i = 0; i < length; ++i) {
    dest[i] = whitened[13 + i];
  }
}

}
/*********************
END OF MSC26A encoder
**********************/

bool ZhijiaEncoder::is_supported(const Command &cmd) {
  ZhijiaArgs_t cmd_real = translate_cmd(cmd);
  return (cmd_real.cmd_ != 0);
}

ZhijiaArgs_t ZhijiaEncoder::translate_cmd(const Command &cmd) {
  ZhijiaArgs_t cmd_real;
  bool isV0 = (this->variant_ == VARIANT_V0);
  bool isV2 = (this->variant_ == VARIANT_V2);
  switch(cmd.cmd_)
  {
    case CommandType::PAIR:
      cmd_real.cmd_ = isV0 ? 0xB4 : 0xA2;  // -76 : -94
      break;
    case CommandType::UNPAIR:
      cmd_real.cmd_ = isV0 ? 0xB0 : 0xA3;  // -80 : -93
      break;
    case CommandType::CUSTOM:
      cmd_real.cmd_ = cmd.args_[0];
      cmd_real.args_[0] = cmd.args_[1];
      cmd_real.args_[1] = cmd.args_[2];
      cmd_real.args_[2] = cmd.args_[3];
      break;
    case CommandType::LIGHT_ON:
      cmd_real.cmd_ = isV0 ? 0xB3 : 0xA5; // -77 : -91
      break;
    case CommandType::LIGHT_OFF:
      cmd_real.cmd_ = isV0 ? 0xB2 : 0xA6;  // -78 : -90
      break;
    case CommandType::LIGHT_DIM:
      if(isV0) {
        cmd_real.cmd_ = 0xB5; // -75
        // app software: int i in between 0 -> 999
        // (byte) ((0xFF0000 & i) >> 16), (byte) ((0x00FF00 & i) >> 8), (byte) (i & 0x0000FF)
        uint16_t argBy4 = (999 * (float)cmd.args_[0]) / 255; // from 0..255 -> 0..999
        cmd_real.args_[1] = ((argBy4 & 0xFF00) >> 8);
        cmd_real.args_[2] = (argBy4 & 0x00FF);
      } else {
        cmd_real.cmd_ = 0xAD; // -83
        // app software: value in between 0 -> 249
        cmd_real.args_[0] = (249 * (float)cmd.args_[0]) / 255;
      }
      break;
    case CommandType::LIGHT_CCT:
      if(isV0) {
        cmd_real.cmd_ = 0xB7; // -73
        // app software: int i in between 0 -> 999
        // (byte) ((0xFF0000 & i) >> 16), (byte) ((0x00FF00 & i) >> 8), (byte) (i & 0x0000FF)
        uint16_t argBy4 = (999 * (float)cmd.args_[0]) / 255; 
        cmd_real.args_[1] = ((argBy4 & 0xFF00) >> 8);
        cmd_real.args_[2] = (argBy4 & 0x00FF);
      } else {
        cmd_real.cmd_ = 0xAE; // -82
        // app software: value in between 0 -> 249
        cmd_real.args_[0] = (249 * (float)cmd.args_[0]) / 255;
      }
      break;
    case CommandType::LIGHT_SEC_ON:
      if(isV0) {
        cmd_real.cmd_ = 0xA6; // -90
        cmd_real.args_[0] = 1;
      } else {
        cmd_real.cmd_ = 0xAF; // -81
      }
      break;
    case CommandType::LIGHT_SEC_OFF:
      if(isV0) {
        cmd_real.cmd_ = 0xA6; // -90
        cmd_real.args_[0] = 2;
      } else {
        cmd_real.cmd_ = 0xB0; // -80
      }
      break;
    case CommandType::FAN_ON:
      cmd_real.cmd_ = isV2 ? 0xD2 : 0; // -47
      break;
    case CommandType::FAN_OFF:
      cmd_real.cmd_ = isV2 ? 0xD3 : 0; // -46
      break;
    case CommandType::FAN_SPEED:
      {
        uint8_t level = (cmd.args_[1] == 3) ? 2 * cmd.args_[0] : cmd.args_[0];
        cmd_real.cmd_ = isV2 ? 0xDB + level : 0; // -37 + speed(1..6) => -36 -> -31
      }
      break;
    default:
      break;
  }
  return cmd_real;
}

/*
    private byte[] mMAC = {0x19, 0x01, 0x10};
    private byte[] mMAC1 = {0x19, 0x01, 0x10, 0xAA};
    private byte[] mUUID = {App.getAndroidId()[0], App.getAndroidId()[1]};
    private byte[] mUUID1 = App.getAndroidId();
    private byte[] mUID = {0x19, 0x01, 0x10};

    private byte[] getAdvData(byte b, byte[] bArr) {
        byte[] msc16 = MSC16.msc16(this.mMAC, this.mUUID, (byte) getGroupId(), b, (byte) getSN(), bArr);
        Log.d(TAG, "Began: " + StringUtils.bytes2HexString(msc16));
        return msc16;
    }

    private byte[] getAdvData1(byte b, byte[] bArr) {
        byte[] msc26 = MSC26.msc26(this.mMAC1, this.mUUID1, this.mUID, (byte) getGroupId1(), b, (byte) getSN1(), bArr);
        Log.d(TAG, "Began: " + StringUtils.bytes2HexString(msc26));
        return msc26;
    }

    private byte[] getAdvData2(byte b, byte[] bArr) {
        byte[] msc26 = MSC26A.msc26(this.mMAC1, this.mUUID1, this.mUID, (byte) getGroupId1(), b, (byte) getSN1(), bArr, this.mMAC1, this.mUUID1, this.mUID);
        Log.d(TAG, "Began: " + StringUtils.bytes2HexString(msc26));
        return msc26;
    }
*/

uint8_t ZhijiaEncoder::get_adv_data(std::vector< BleAdvParam > & params, Command &cmd) {
  params.emplace_back();
  BleAdvParam & param = params.back();

  ZhijiaArgs_t cmd_real = this->translate_cmd(cmd);

  unsigned char uuid[3] = {0};
  uuid[0] = (cmd.id_ & 0xFF0000) >> 16;
  uuid[1] = (cmd.id_ & 0x00FF00) >> 8;
  uuid[2] = (cmd.id_ & 0x0000FF);
  ESP_LOGD(TAG, "%s - UUID: '0x%02X%02X%02X', tx: %d, Command: '0x%02X', Args: [%d,%d,%d]", this->id_.c_str(), 
      uuid[0], uuid[1], uuid[2], cmd.tx_count_, cmd_real.cmd_, cmd_real.args_[0], cmd_real.args_[1], cmd_real.args_[2]);
  switch(this->variant_)
  {
    case VARIANT_V0:
      msc16_msc26::aes16Encrypt16(MAC, uuid, 0xFF, cmd_real.cmd_, cmd.tx_count_, cmd_real.args_, param.buf_);
      param.len_ = 16;
      break;
    case VARIANT_V1:
      msc16_msc26::aes26Encrypt(MAC, uuid, MAC, 0x7F, cmd_real.cmd_, cmd.tx_count_, cmd_real.args_, param.buf_);
      param.len_ = 26;
      break;
    case VARIANT_V2:
      msc26a::stage1(cmd_real.args_, cmd_real.cmd_, cmd.tx_count_, 0x7F, MAC, uuid, param.buf_);
      msc26a::stage2(param.buf_, 0x18, 0x2);
      msc26a::stage3(param.buf_, 0x1a, param.buf_);
      param.len_ = 26;
      break;
    default:
      ESP_LOGW(TAG, "get_adv_data called with invalid variant %d", this->variant_);
      break;
  }
  return 1;
}

void ZhijiaEncoder::register_encoders(BleAdvHandler * handler, const std::string & encoding) {
  BleAdvMultiEncoder * zhijia_all = new BleAdvMultiEncoder("Zhi Jia - All", encoding);
  handler->add_encoder(zhijia_all);
  BleAdvEncoder * zhijia_v0 = new ZhijiaEncoder("Zhi Jia - v0", encoding, ZhijiaVariant::VARIANT_V0);
  handler->add_encoder(zhijia_v0);
  zhijia_all->add_encoder(zhijia_v0);
  BleAdvEncoder * zhijia_v1 = new ZhijiaEncoder("Zhi Jia - v1", encoding, ZhijiaVariant::VARIANT_V1);
  handler->add_encoder(zhijia_v1);
  zhijia_all->add_encoder(zhijia_v1);
  BleAdvEncoder * zhijia_v2 = new ZhijiaEncoder("Zhi Jia - v2", encoding, ZhijiaVariant::VARIANT_V2);
  handler->add_encoder(zhijia_v2);
  zhijia_all->add_encoder(zhijia_v2);
}

} // namespace bleadvcontroller
} // namespace esphome
