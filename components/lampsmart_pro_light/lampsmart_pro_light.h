#pragma once

#include "esphome.h"
#include "esphome/components/api/custom_api_device.h"
#include "esphome/components/light/light_output.h"

namespace esphome {
namespace lampsmartpro {

class LampSmartProLight : public light::LightOutput, public Component, public api::CustomAPIDevice {
 public:
  void setup() override;
  void dump_config() override;

  void set_cold_white_temperature(float cold_white_temperature) { cold_white_temperature_ = cold_white_temperature; }
  void set_warm_white_temperature(float warm_white_temperature) { warm_white_temperature_ = warm_white_temperature; }
  void set_constant_brightness(bool constant_brightness) { constant_brightness_ = constant_brightness; }
  void set_reversed(bool reversed) { reversed_ = reversed; }
  void set_min_brightness(uint8_t min_brightness) { min_brightness_ = min_brightness; }
  void set_tx_duration(uint32_t tx_duration) { tx_duration_ = tx_duration; }
  void write_state(light::LightState *state) override;
  light::LightTraits get_traits() override;

 protected:
  void on_pair();
  void send_packet(uint16_t cmd, uint8_t cold, uint8_t warm);

  float cold_white_temperature_{167};
  float warm_white_temperature_{333};
  bool constant_brightness_;
  bool reversed_;
  uint8_t min_brightness_;
  bool _is_off;
  uint8_t tx_count_;
  uint32_t tx_duration_;
};

} //namespace lampsmartpro
} //namespace esphome
