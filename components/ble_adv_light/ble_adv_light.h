#pragma once

#include "esphome.h"
#include "esphome/core/log.h"
#ifdef USE_API
#include "esphome/components/api/custom_api_device.h"
#endif
#include "esphome/components/light/light_output.h"

namespace esphome {
namespace bleadvlight {

class BleAdvLight : public light::LightOutput, public Component, public EntityBase
#ifdef USE_API
  , public api::CustomAPIDevice
#endif
{
 public:
  void setup() override;
  void dump_config() override;

  void set_cold_white_temperature(float cold_white_temperature) { cold_white_temperature_ = cold_white_temperature; }
  void set_warm_white_temperature(float warm_white_temperature) { warm_white_temperature_ = warm_white_temperature; }
  void set_constant_brightness(bool constant_brightness) { constant_brightness_ = constant_brightness; }
  void set_reversed(bool reversed) { reversed_ = reversed; }
  void set_min_brightness(uint8_t min_brightness) { min_brightness_ = min_brightness; }
  void set_tx_duration(uint32_t tx_duration) { tx_duration_ = tx_duration; }
  void setup_state(light::LightState *state) override { this->light_state_ = state; }
  void write_state(light::LightState *state) override;
  light::LightTraits get_traits() override;
  void on_pair();
  void on_unpair();

 protected:
  virtual void update_channels(uint8_t cold, uint8_t warm) = 0;
  virtual void send_packet(uint8_t cmd, uint8_t *args = NULL) = 0;

  virtual uint8_t CMD_PAIR() = 0;
  virtual uint8_t CMD_UNPAIR() = 0;
  virtual uint8_t CMD_TURN_ON() = 0;
  virtual uint8_t CMD_TURN_OFF() = 0;
  virtual uint8_t CMD_DIM() = 0;
  virtual uint8_t CMD_CCT() = 0;

  float cold_white_temperature_{167};
  float warm_white_temperature_{333};
  bool constant_brightness_;
  bool reversed_;
  uint8_t min_brightness_;
  bool _is_off;
  uint8_t tx_count_;
  uint32_t tx_duration_;
  light::LightState *light_state_;
};

class ZhiJiaLight : public BleAdvLight
{
 protected:
  void send_packet(uint8_t cmd, uint8_t *args) override;

  uint8_t CMD_PAIR() override { return 0xA2; };
  uint8_t CMD_UNPAIR() override { return 0x45; };
  uint8_t CMD_TURN_ON() override { return 0xA5; };
  uint8_t CMD_TURN_OFF() override { return 0xA6; };
  uint8_t CMD_DIM() override { return 0xAD; };
  uint8_t CMD_CCT() override { return 0xAE; };

  void update_channels(uint8_t cold, uint8_t warm) override {
    ESP_LOGD("zhijia", "ZhiJiaLight::update_channels called (cold: %d, warm: %d)!", cold, warm);
    ESP_LOGD("zhijia", "Calling send_packet(0x%02x, %d)", CMD_CCT(), (uint8_t) (255 * ((float) warm / (cold + warm))));
    send_packet(CMD_CCT(), (uint8_t) (255 * ((float) warm / (cold + warm))));
    send_packet(CMD_DIM(), (uint8_t) (cold + warm > 255 ? 255 : cold + warm));
  };

 private:
  void send_packet(uint8_t cmd, uint8_t val = 0) { send_packet(cmd, {val}); };
};

class LampSmartProLight : public BleAdvLight
{
 protected:
  void send_packet(uint8_t cmd, uint8_t *args) override;

  uint8_t CMD_PAIR() override { return 0x28; };
  uint8_t CMD_UNPAIR() override { return 0x45; };
  uint8_t CMD_TURN_ON() override { return 0x10; };
  uint8_t CMD_TURN_OFF() override { return 0x11; };
  uint8_t CMD_DIM() override { return 0x21; };
  uint8_t CMD_CCT() override { return 0; };

  void update_channels(uint8_t cold, uint8_t warm) override {
    send_packet(CMD_DIM(), cold, warm);
  };

 private:
  void send_packet(uint8_t cmd, uint8_t cold, uint8_t warm) {
    uint8_t args[2] = {cold, warm};
    send_packet(cmd, args);
  };
};

template<typename... Ts> class PairAction : public Action<Ts...> {
 public:
  explicit PairAction(esphome::light::LightState *state) : state_(state) {}

  void play(Ts... x) override {
    ((BleAdvLight *)this->state_->get_output())->on_pair();
  }

 protected:
  esphome::light::LightState *state_;
};

template<typename... Ts> class UnpairAction : public Action<Ts...> {
 public:
  explicit UnpairAction(esphome::light::LightState *state) : state_(state) {}

  void play(Ts... x) override {
    ((BleAdvLight *)this->state_->get_output())->on_unpair();
  }

 protected:
  esphome::light::LightState *state_;
};

} //namespace bleadvlight
} //namespace esphome
