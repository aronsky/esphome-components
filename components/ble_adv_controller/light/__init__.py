import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import light, output
from esphome.const import (
    CONF_CONSTANT_BRIGHTNESS,
    CONF_COLD_WHITE_COLOR_TEMPERATURE,
    CONF_WARM_WHITE_COLOR_TEMPERATURE,
    CONF_MIN_BRIGHTNESS,
    CONF_OUTPUT_ID,
    CONF_DEFAULT_TRANSITION_LENGTH,
    CONF_RESTORE_MODE,
)

from .. import (
    bleadvcontroller_ns,
    ENTITY_BASE_CONFIG_SCHEMA,
    entity_base_code_gen,
    BleAdvEntity,
)

from ..const import (
    CONF_BLE_ADV_SECONDARY,
    CONF_BLE_ADV_SPLIT_DIM_CCT,
)

BleAdvLight = bleadvcontroller_ns.class_('BleAdvLight', light.LightOutput, BleAdvEntity)
BleAdvSecLight = bleadvcontroller_ns.class_('BleAdvSecLight', light.LightOutput, BleAdvEntity)

CONFIG_SCHEMA = cv.All(
    cv.Any(
        light.RGB_LIGHT_SCHEMA.extend(
            {
                cv.GenerateID(CONF_OUTPUT_ID): cv.declare_id(BleAdvLight),
                cv.Optional(CONF_COLD_WHITE_COLOR_TEMPERATURE, default="167 mireds"): cv.color_temperature,
                cv.Optional(CONF_WARM_WHITE_COLOR_TEMPERATURE, default="333 mireds"): cv.color_temperature,
                cv.Optional(CONF_CONSTANT_BRIGHTNESS, default=False): cv.boolean,
                cv.Optional(CONF_MIN_BRIGHTNESS, default="1%"): cv.percentage,
                cv.Optional(CONF_BLE_ADV_SPLIT_DIM_CCT, default=False): cv.boolean,
                # override default value of default_transition_length to 0s as mostly not supported by those lights
                cv.Optional(CONF_DEFAULT_TRANSITION_LENGTH, default="0s"): cv.positive_time_period_milliseconds,
                # override default value for restore mode, to always restore as it was if possible
                cv.Optional(CONF_RESTORE_MODE, default="RESTORE_DEFAULT_OFF"): cv.enum(light.RESTORE_MODES, upper=True, space="_"),
            }
        ).extend(ENTITY_BASE_CONFIG_SCHEMA),
        light.RGB_LIGHT_SCHEMA.extend(
            {
                cv.GenerateID(CONF_OUTPUT_ID): cv.declare_id(BleAdvSecLight),
                cv.Required(CONF_BLE_ADV_SECONDARY): cv.one_of(True),
            }
        ).extend(ENTITY_BASE_CONFIG_SCHEMA),
    ),    
    cv.has_none_or_all_keys(
        [CONF_COLD_WHITE_COLOR_TEMPERATURE, CONF_WARM_WHITE_COLOR_TEMPERATURE]
    ),
    light.validate_color_temperature_channels,
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_OUTPUT_ID])
    await entity_base_code_gen(var, config, "light")
    await light.register_light(var, config)
    if CONF_BLE_ADV_SECONDARY in config:
        cg.add(var.set_traits())
    else:
        cg.add(var.set_traits(config[CONF_COLD_WHITE_COLOR_TEMPERATURE], config[CONF_WARM_WHITE_COLOR_TEMPERATURE]))
        cg.add(var.set_constant_brightness(config[CONF_CONSTANT_BRIGHTNESS]))
        cg.add(var.set_split_dim_cct(config[CONF_BLE_ADV_SPLIT_DIM_CCT]))
        cg.add(var.set_min_brightness(config[CONF_MIN_BRIGHTNESS] * 100, 0, 100, 1))
