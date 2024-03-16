#include "msc26a.h"

void stage1(unsigned char *x0, unsigned char w20, unsigned char w21,
            unsigned char w22, unsigned char *x23, const unsigned char *x24,
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
          (uint8_t)(uVar4 >> 7) & 1 |
          (uint8_t)((uVar4 >> 6 & 1 |
                     (uVar4 >> 5 & 1 |
                      ((uVar4 & 0xff) >> 4 & 1 |
                       (uVar4 >> 3 & 1 |
                        (uVar4 & 2 | (uVar4 & 1) << 2 | (uVar4 & 0xff) >> 2 & 1)
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
      iVar4 += (uVar2 ^ bVar1 >> (uVar3 & 0xff) & 1) << (uVar3 & 0xff);
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

void get_adv_data(uint8_t *mfg_data, uint8_t *args, uint8_t cmd, uint8_t sn,
                  const uint8_t *uuid) {
  unsigned char MAC[] = {0x19, 0x01, 0x10};
  unsigned char GROUP = 127;

  stage1(args, cmd, sn, GROUP, MAC, uuid, mfg_data);
  stage2(mfg_data, 0x18, 0x2);
  stage3(mfg_data, 0x1a, mfg_data);
}