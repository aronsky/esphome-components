import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import fan

from esphome.const import CONF_ID

from .. import (
    bleadvcontroller_ns,
    ENTITY_BASE_CONFIG_SCHEMA,
    entity_base_code_gen,
    BleAdvEntity,
)

from ..const import (
    CONF_BLE_ADV_SPEED_COUNT,
    CONF_BLE_ADV_DIRECTION_SUPPORTED,
    CONF_BLE_ADV_OSCILLATION_SUPPORTED,
    CONF_BLE_ADV_FORCED_REFRESH_ON_START,
)

BleAdvFan = bleadvcontroller_ns.class_('BleAdvFan', fan.Fan, BleAdvEntity)

CONFIG_SCHEMA = cv.All(
    # override default value for restore mode, to always restore as it was if possible
    fan.fan_schema(BleAdvFan, default_restore_mode="RESTORE_DEFAULT_OFF").extend(
        {
            cv.Optional(CONF_BLE_ADV_SPEED_COUNT, default=6): cv.one_of(0,3,6),
            cv.Optional(CONF_BLE_ADV_DIRECTION_SUPPORTED, default=True): cv.boolean,
            cv.Optional(CONF_BLE_ADV_OSCILLATION_SUPPORTED, default=False): cv.boolean,
            cv.Optional(CONF_BLE_ADV_FORCED_REFRESH_ON_START, default=True): cv.boolean,
        }
    ).extend(ENTITY_BASE_CONFIG_SCHEMA),
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await entity_base_code_gen(var, config, "fan")
    await fan.register_fan(var, config)
    cg.add(var.set_speed_count(config[CONF_BLE_ADV_SPEED_COUNT]))
    cg.add(var.set_direction_supported(config[CONF_BLE_ADV_DIRECTION_SUPPORTED]))
    cg.add(var.set_oscillation_supported(config[CONF_BLE_ADV_OSCILLATION_SUPPORTED]))
    cg.add(var.set_forced_refresh_on_start(config[CONF_BLE_ADV_FORCED_REFRESH_ON_START]))
