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

/**
On button ON / OFF pressed: only State ON or OFF received
On Speed change: State and Speed received
On ON with Speed: State and Speed received
On Direction Change: only direction received
*/
void BleAdvFan::control(const fan::FanCall &call) {
  if (call.get_state().has_value()) {
    // State ON/OFF or SPEED changed
    this->state = *call.get_state();
    if (call.get_speed().has_value()) {
      this->speed = *call.get_speed();
    }
    if (this->state) {
      // Switch ON, always setting with SPEED
      ESP_LOGD(TAG, "BleAdvFan::control - Setting ON with speed %d", this->speed);
      if (this->get_parent()->is_supported(CommandType::FAN_ONOFF_SPEED)) {
        this->command(CommandType::FAN_ONOFF_SPEED, this->speed, this->speed_count_);
      } else {
        this->command(CommandType::FAN_ON);
        this->command(CommandType::FAN_SPEED, this->speed, this->speed_count_);
      }
    } else {
      // Switch OFF
      ESP_LOGD(TAG, "BleAdvFan::control - Setting OFF");
      if (this->get_parent()->is_supported(CommandType::FAN_ONOFF_SPEED)) {
        this->command(CommandType::FAN_ONOFF_SPEED, 0, this->speed_count_);
      } else {
        this->command(CommandType::FAN_OFF);
      }
    }
  }

  if (call.get_direction().has_value()) {
    // Change of direction
    this->direction = *call.get_direction();
    bool isFwd = this->direction == fan::FanDirection::FORWARD;
    ESP_LOGD(TAG, "BleAdvFan::control - Setting direction %s", (isFwd ? "fwd":"rev"));
    this->command(CommandType::FAN_DIR, isFwd);
  }

  this->publish_state();
}

} // namespace bleadvcontroller
} // namespace esphome
