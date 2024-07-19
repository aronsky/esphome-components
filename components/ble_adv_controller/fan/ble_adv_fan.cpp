#include "ble_adv_fan.h"
#include "esphome/core/log.h"
#include "esphome/components/ble_adv_controller/ble_adv_controller.h"

namespace esphome {
namespace bleadvcontroller {

static const char *TAG = "ble_adv_fan";

fan::FanTraits BleAdvFan::get_traits() {
  auto traits = fan::FanTraits();

  traits.set_direction(this->use_direction_);
  traits.set_speed(this->speed_count_ > 0);
  traits.set_supported_speed_count(this->speed_count_);
  traits.set_oscillation(false);

  return traits;
}

void BleAdvFan::dump_config() {
  ESP_LOGCONFIG(TAG, "BleAdvFan '%s'", this->get_name().c_str());
  BleAdvEntity::dump_config_base(TAG);
}

void BleAdvFan::control(const fan::FanCall &call) {
  ESP_LOGD(TAG, "BleAdvFan::control called");
  bool stopped = false;
  if (call.get_speed().has_value() && (this->speed != *call.get_speed())) {
    // Speed change OR Start at speed
    ESP_LOGD(TAG, "BleAdvFan::control - setup speed %d", *call.get_speed());
    this->command(CommandType::FAN_SPEED, (uint8_t)*call.get_speed(), this->speed_count_);
    this->speed = *call.get_speed();
    this->state = true;
  }
  else if (call.get_state().has_value() && (this->state != *call.get_state())) {
    // No Speed change but state change
    ESP_LOGD(TAG, "BleAdvFan::control - toggle");
    this->command(CommandType::FAN_OFF);
    this->state = *call.get_state();
    // reset direction on stop, as this is the fan behaviour
    if(!this->state) {
      this->direction = fan::FanDirection::FORWARD;
    }
  }
  if (call.get_direction().has_value() && (this->direction != *call.get_direction())) {
    // Change of direction
    bool isFwd = *call.get_direction() == fan::FanDirection::FORWARD;
    ESP_LOGD(TAG, "BleAdvFan::control - setup direction %s", (isFwd ? "fwd":"rev"));
    this->command(CommandType::FAN_DIR, isFwd);
    this->direction = *call.get_direction();
  }

  this->publish_state();
}

} // namespace bleadvcontroller
} // namespace esphome
