#include "ble_adv_button.h"
#include "esphome/core/log.h"
#include "esphome/components/ble_adv_controller/ble_adv_controller.h"

namespace esphome {
namespace bleadvcontroller {

static const char *TAG = "ble_adv_button";

void BleAdvButton::dump_config() {
  LOG_BUTTON("", "BleAdvButton", this);
  BleAdvEntity::dump_config_base(TAG);
}

void BleAdvButton::press_action() {
  ESP_LOGD(TAG, "BleAdvButton::press_action called");
  this->command((CommandType)this->cmd_, this->args_);
}

} // namespace bleadvcontroller
} // namespace esphome
