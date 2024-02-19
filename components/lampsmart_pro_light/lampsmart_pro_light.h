#pragma once

#include "esphome.h"
#ifdef USE_API
#include "esphome/components/api/custom_api_device.h"
#endif
#include "esphome/components/light/light_output.h"

#define CMD_PAIR (0x28)
#define CMD_UNPAIR (0x45)
#define CMD_TURN_ON (0x10)
#define CMD_TURN_OFF (0x11)
#define CMD_DIM (0x21)

namespace esphome {
namespace lampsmartpro {

class LampSmartProLight : public light::LightOutput, public Component, public EntityBase
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
  void send_packet(uint16_t cmd, uint8_t cold, uint8_t warm);

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

template<typename... Ts> class PairAction : public Action<Ts...> {
 public:
  explicit PairAction(esphome::light::LightState *state) : state_(state) {}

  void play(Ts... x) override {
    ((LampSmartProLight *)this->state_->get_output())->on_pair();
  }

 protected:
  esphome::light::LightState *state_;
};

template<typename... Ts> class UnpairAction : public Action<Ts...> {
 public:
  explicit UnpairAction(esphome::light::LightState *state) : state_(state) {}

  void play(Ts... x) override {
    ((LampSmartProLight *)this->state_->get_output())->on_unpair();
  }

 protected:
  esphome::light::LightState *state_;
};

} //namespace lampsmartpro
} //namespace esphome
