import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import light, output
from esphome import automation
from esphome.const import (
    CONF_DURATION,
    CONF_CONSTANT_BRIGHTNESS,
    CONF_OUTPUT_ID,
    CONF_COLD_WHITE_COLOR_TEMPERATURE,
    CONF_WARM_WHITE_COLOR_TEMPERATURE,
    CONF_REVERSED,
    CONF_MIN_BRIGHTNESS,
    CONF_TYPE,
    CONF_ID,
)

AUTO_LOAD = ["esp32_ble"]
DEPENDENCIES = ["esp32"]

bleadvlight_ns = cg.esphome_ns.namespace('bleadvlight')
BleAdvLight = bleadvlight_ns.class_('BleAdvLight', cg.Component, light.LightOutput)
ZhiJiaLight = bleadvlight_ns.class_('ZhiJiaLight', BleAdvLight)
LampSmartProLight = bleadvlight_ns.class_('LampSmartProLight', BleAdvLight)
PairAction = bleadvlight_ns.class_("PairAction", automation.Action)
UnpairAction = bleadvlight_ns.class_("UnpairAction", automation.Action)


ACTION_ON_PAIR_SCHEMA = cv.All(
    automation.maybe_simple_id(
        {
            cv.Required(CONF_ID): cv.use_id(light.LightState),
        }
    )
)

ACTION_ON_UNPAIR_SCHEMA = cv.All(
    automation.maybe_simple_id(
        {
            cv.Required(CONF_ID): cv.use_id(light.LightState),
        }
    )
)

CONFIG_SCHEMA = cv.All(
    light.RGB_LIGHT_SCHEMA.extend(
        {
            cv.GenerateID(CONF_OUTPUT_ID): cv.declare_id(BleAdvLight),
            cv.Required(CONF_TYPE): cv.one_of("zhijia", "lampsmart_pro", lower=True),
            cv.Optional(CONF_DURATION, default=100): cv.positive_int,
            cv.Optional(CONF_COLD_WHITE_COLOR_TEMPERATURE): cv.color_temperature,
            cv.Optional(CONF_WARM_WHITE_COLOR_TEMPERATURE): cv.color_temperature,
            cv.Optional(CONF_CONSTANT_BRIGHTNESS, default=False): cv.boolean,
            cv.Optional(CONF_REVERSED, default=False): cv.boolean,
            cv.Optional(CONF_MIN_BRIGHTNESS, default=0x7): cv.hex_uint8_t,
        }
    ),
    cv.has_none_or_all_keys(
        [CONF_COLD_WHITE_COLOR_TEMPERATURE, CONF_WARM_WHITE_COLOR_TEMPERATURE]
    ),
    light.validate_color_temperature_channels,
)


async def to_code(config):
    match config[CONF_TYPE]:
        case "zhijia":
            subclass = ZhiJiaLight.new
        case "lampsmart_pro":
            subclass = LampSmartProLight.new
    var = cg.Pvariable(config[CONF_OUTPUT_ID], subclass)
    await cg.register_component(var, config)
    await light.register_light(var, config)

    if CONF_COLD_WHITE_COLOR_TEMPERATURE in config:
        cg.add(
            var.set_cold_white_temperature(config[CONF_COLD_WHITE_COLOR_TEMPERATURE])
        )

    if CONF_WARM_WHITE_COLOR_TEMPERATURE in config:
        cg.add(
            var.set_warm_white_temperature(config[CONF_WARM_WHITE_COLOR_TEMPERATURE])
        )

    cg.add(var.set_constant_brightness(config[CONF_CONSTANT_BRIGHTNESS]))
    cg.add(var.set_reversed(config[CONF_REVERSED]))
    cg.add(var.set_min_brightness(config[CONF_MIN_BRIGHTNESS]))
    cg.add(var.set_tx_duration(config[CONF_DURATION]))


@automation.register_action(
    "bleadvlight.pair", PairAction, ACTION_ON_PAIR_SCHEMA
)
@automation.register_action(
    "bleadvlight.unpair", UnpairAction, ACTION_ON_UNPAIR_SCHEMA
)
async def bleadvlight_pair_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(action_id, template_arg, parent)
