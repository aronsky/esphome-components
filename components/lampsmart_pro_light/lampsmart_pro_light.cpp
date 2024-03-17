#include "lampsmart_pro_light.h"
#include "esphome/core/log.h"

#ifdef USE_ESP32

#include <esp_gap_ble_api.h>
#include <esp_gatts_api.h>
#include <mbedtls/aes.h>

namespace esphome {
namespace lampsmartpro {

static const char *TAG = "lampsmartpro";

#pragma pack(1)
typedef union {
  struct { /* Advertising Data */
    uint8_t prefix[10];
    uint8_t packet_number;
    uint16_t type;
    uint32_t identifier;
    uint8_t var2;
    uint16_t command;
    uint16_t _20;
    uint8_t channel1;
    uint8_t channel2;
    uint16_t signature_v3;
    uint8_t _26;
    uint16_t rand;
    uint16_t crc16;
  };
  uint8_t raw[31];
} adv_data_t;

static esp_ble_adv_params_t ADVERTISING_PARAMS = {
  .adv_int_min = 0x20,
  .adv_int_max = 0x20,
  .adv_type = ADV_TYPE_NONCONN_IND,
  .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
  .peer_addr =
    {
      0x00,
    },
  .peer_addr_type = BLE_ADDR_TYPE_PUBLIC,
  .channel_map = ADV_CHNL_ALL,
  .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

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

void ble_whiten(uint8_t *buf, uint8_t size, uint8_t seed, uint8_t salt) {
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

void LampSmartProLight::setup() {
#ifdef USE_API
  register_service(&LampSmartProLight::on_pair, light_state_ ? "pair_" + light_state_->get_object_id() : "pair");
  register_service(&LampSmartProLight::on_unpair, light_state_ ? "unpair_" + light_state_->get_object_id() : "unpair");
#endif
}

light::LightTraits LampSmartProLight::get_traits() {
  auto traits = light::LightTraits();
  traits.set_supported_color_modes({light::ColorMode::COLD_WARM_WHITE});
  traits.set_min_mireds(this->cold_white_temperature_);
  traits.set_max_mireds(this->warm_white_temperature_);
  return traits;
}

void LampSmartProLight::write_state(light::LightState *state) {
  float cwf, wwf;
  state->current_values_as_cwww(&cwf, &wwf, this->constant_brightness_);

  if (!cwf && !wwf) {
    send_packet(CMD_TURN_OFF, 0, 0);
    _is_off = true;

    return;
  }

  uint8_t cwi = (uint8_t)(0xff * cwf);
  uint8_t wwi = (uint8_t)(0xff * wwf);

  if ((cwi < min_brightness_) && (wwi < min_brightness_)) {
    if (cwf > 0.000001) {
      cwi = min_brightness_;
    }
    
    if (wwf > 0.000001) {
      wwi = min_brightness_;
    }
  }

  ESP_LOGD(TAG, "LampSmartProLight::write_state called! Requested cw: %d, ww: %d", cwi, wwi);

  if (_is_off) {
    send_packet(CMD_TURN_ON, 0, 0);
    _is_off = false;
  }

  send_packet(CMD_DIM, cwi, wwi);
}

void LampSmartProLight::dump_config() {
  ESP_LOGCONFIG(TAG, "LampSmartProLight '%s'", light_state_ ? light_state_->get_name().c_str() : "");
  ESP_LOGCONFIG(TAG, "  Cold White Temperature: %f mireds", cold_white_temperature_);
  ESP_LOGCONFIG(TAG, "  Warm White Temperature: %f mireds", warm_white_temperature_);
  ESP_LOGCONFIG(TAG, "  Constant Brightness: %s", constant_brightness_ ? "true" : "false");
  ESP_LOGCONFIG(TAG, "  Minimum Brightness: %d", min_brightness_);
  ESP_LOGCONFIG(TAG, "  Transmission Duratoin: %d millis", tx_duration_);
}

void LampSmartProLight::on_pair() {
  ESP_LOGD(TAG, "LampSmartProLight::on_pair called!");
  send_packet(CMD_PAIR, 0, 0);
}

void LampSmartProLight::on_unpair() {
  ESP_LOGD(TAG, "LampSmartProLight::on_unpair called!");
  send_packet(CMD_UNPAIR, 0, 0);
}

void sign_packet_v3(adv_data_t* packet) {
  uint16_t seed = packet->rand;
  uint8_t sigkey[16] = {0, 0, 0, 0x0D, 0xBF, 0xE6, 0x42, 0x68, 0x41, 0x99, 0x2D, 0x0F, 0xB0, 0x54, 0xBB, 0x16};
  uint8_t tx_count = (uint8_t) packet->packet_number;

  sigkey[0] = seed & 0xff;
  sigkey[1] = (seed >> 8) & 0xff;
  sigkey[2] = tx_count;
  mbedtls_aes_context aes_ctx;
  mbedtls_aes_init(&aes_ctx);
  mbedtls_aes_setkey_enc(&aes_ctx, sigkey, sizeof(sigkey)*8);
  uint8_t aes_in[16], aes_out[16];
  memcpy(aes_in, &(packet->raw[8]), 16);
  mbedtls_aes_crypt_ecb(&aes_ctx, ESP_AES_ENCRYPT, aes_in, aes_out);
  mbedtls_aes_free(&aes_ctx);
  packet->signature_v3 = ((uint16_t*) aes_out)[0]; 
  if (packet->signature_v3 == 0) {
      packet->signature_v3 = 0xffff;
  }
}

void LampSmartProLight::send_packet(uint16_t cmd, uint8_t cold, uint8_t warm) {
  uint16_t seed = (uint16_t) rand();

  adv_data_t packet = {{
      .prefix = {0x02, 0x01, 0x02, 0x1B, 0x16, 0xF0, 0x08, 0x10, 0x80, 0x00},
      .packet_number = ++(this->tx_count_),
      .type = 0x100,
      .identifier = light_state_ ? light_state_->get_object_id_hash() : 0xcafebabe,
      .var2 = 0x0,
      .command = cmd,
      ._20 = 0,
      .channel1 = reversed_ ? warm : cold,
      .channel2 = reversed_ ? cold : warm,
      .signature_v3 = 0,
      ._26 = 0,
      .rand = seed,
  }};

  sign_packet_v3(&packet);
  ble_whiten(&packet.raw[9], 0x12, (uint8_t) seed, 0);
  packet.crc16 = v2_crc16_ccitt(&packet.raw[7], 0x16, ~seed);
  
  ESP_ERROR_CHECK_WITHOUT_ABORT(esp_ble_gap_config_adv_data_raw(packet.raw, sizeof(packet)));
  ESP_ERROR_CHECK_WITHOUT_ABORT(esp_ble_gap_start_advertising(&ADVERTISING_PARAMS));
  delay(tx_duration_);
  ESP_ERROR_CHECK_WITHOUT_ABORT(esp_ble_gap_stop_advertising());
}

} // namespace lampsmartpro
} // namespace esphome

#endif
