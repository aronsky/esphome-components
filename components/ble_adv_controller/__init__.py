import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.core import ID
from esphome.const import (
    CONF_DURATION,
    CONF_ID,
    CONF_NAME,
    CONF_REVERSED,
    CONF_TYPE,
    CONF_INDEX,
    CONF_VARIANT,
    PLATFORM_ESP32,
)
from esphome.cpp_helpers import setup_entity
from .const import (
    CONF_BLE_ADV_CONTROLLER_ID,
    CONF_BLE_ADV_ENCODING,
    CONF_BLE_ADV_FORCED_ID,
    CONF_BLE_ADV_MAX_DURATION,
    CONF_BLE_ADV_SEQ_DURATION,
    CONF_BLE_ADV_SHOW_CONFIG,
)

AUTO_LOAD = ["esp32_ble", "select", "number"]
DEPENDENCIES = ["esp32"]
MULTI_CONF = True

bleadvcontroller_ns = cg.esphome_ns.namespace('bleadvcontroller')
BleAdvController = bleadvcontroller_ns.class_('BleAdvController', cg.Component, cg.EntityBase)
BleAdvEncoder = bleadvcontroller_ns.class_('BleAdvEncoder')
BleAdvMultiEncoder = bleadvcontroller_ns.class_('BleAdvMultiEncoder', BleAdvEncoder)
BleAdvHandler = bleadvcontroller_ns.class_('BleAdvHandler', cg.Component)
BleAdvEntity = bleadvcontroller_ns.class_('BleAdvEntity', cg.Component)

FanLampEncoder = bleadvcontroller_ns.class_('FanLampEncoder', BleAdvEncoder)
FanLampEncoder_ns = bleadvcontroller_ns.namespace('FanLampEncoder')
ZhijiaEncoder = bleadvcontroller_ns.class_('ZhijiaEncoder', BleAdvEncoder)
ZhijiaEncoder_ns = bleadvcontroller_ns.namespace('ZhijiaEncoder')

FanLampVariant = bleadvcontroller_ns.enum("FanLampVariant")
CONTROLLER_FANLAMP_VARIANTS = {
    "v3": FanLampVariant.VARIANT_3,
    "v2": FanLampVariant.VARIANT_2,
    "v1a": FanLampVariant.VARIANT_1A,
    "v1b": FanLampVariant.VARIANT_1B,
}

ZhijiaVariant = bleadvcontroller_ns.enum("ZhijiaVariant")
CONTROLLER_ZHIJIA_VARIANTS = {
    "v0": ZhijiaVariant.VARIANT_V0,
    "v1": ZhijiaVariant.VARIANT_V1,
    "v2": ZhijiaVariant.VARIANT_V2,
}

def validate_fanlamp_encoding(value):
    accepted_encoding = ["fanlamp_pro", "lampsmart_pro"]
    if value in accepted_encoding:
        return "fanlamp_pro"
    raise cv.Invalid("Invalid encoding: %s. accepted values: %s" % (value, accepted_encoding))

ENTITY_BASE_CONFIG_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_BLE_ADV_CONTROLLER_ID): cv.use_id(BleAdvController),
    }
)

CONTROLLER_BASE_CONFIG = cv.ENTITY_BASE_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(BleAdvController),
        cv.Optional(CONF_DURATION, default=200): cv.All(cv.positive_int, cv.Range(min=100, max=500)),
        cv.Optional(CONF_BLE_ADV_MAX_DURATION, default=3000): cv.All(cv.positive_int, cv.Range(min=300, max=10000)),
        cv.Optional(CONF_BLE_ADV_SEQ_DURATION, default=100): cv.All(cv.positive_int, cv.Range(min=0, max=150)),
        cv.Optional(CONF_REVERSED, default=False): cv.boolean,
        cv.Optional(CONF_BLE_ADV_SHOW_CONFIG, default=True): cv.boolean,
    }
)

CONFIG_SCHEMA = cv.All(
    cv.Any(
        CONTROLLER_BASE_CONFIG.extend(
            {
                cv.Required(CONF_BLE_ADV_ENCODING): validate_fanlamp_encoding,
                cv.Optional(CONF_VARIANT, default="v3"): cv.enum(CONTROLLER_FANLAMP_VARIANTS, lower=True),
                cv.Optional(CONF_BLE_ADV_FORCED_ID, default=0): cv.hex_uint32_t,
            }
        ),
        CONTROLLER_BASE_CONFIG.extend(
            {
                cv.Required(CONF_BLE_ADV_ENCODING): cv.one_of("zhijia"),
                cv.Optional(CONF_VARIANT, default="v2"): cv.enum(CONTROLLER_ZHIJIA_VARIANTS, lower=True),
                cv.Optional(CONF_BLE_ADV_FORCED_ID, default=0xC630B8): cv.All(cv.hex_uint32_t, cv.Range(min=0, max=0xFFFFFF)),
            }
        ),
    ),
    cv.only_on([PLATFORM_ESP32]),
)

async def entity_base_code_gen(var, config):
    await cg.register_parented(var, config[CONF_BLE_ADV_CONTROLLER_ID])
    await cg.register_component(var, config)
    await setup_entity(var, config)

class BleAdvRegistry:
    handler = None
    @classmethod
    def get(cls):
        if not cls.handler:
            hdl_id = ID("ble_adv_static_handler", type=BleAdvHandler)
            cls.handler = cg.new_Pvariable(hdl_id)
            cg.add(cls.handler.set_component_source("ble_adv_handler"))
            cg.add(cg.App.register_component(cls.handler))
            cg.add(ZhijiaEncoder_ns.register_encoders(cls.handler, "zhijia"))
            cg.add(FanLampEncoder_ns.register_encoders(cls.handler, "fanlamp_pro"))
        return cls.handler

async def to_code(config):
    hdl = BleAdvRegistry.get()
    var = cg.new_Pvariable(config[CONF_ID])
    cg.add(var.set_setup_priority(300)) # start after Bluetooth
    await cg.register_component(var, config)
    await setup_entity(var, config)
    cg.add(var.set_handler(hdl))
    cg.add(var.set_encoding_and_variant(config[CONF_BLE_ADV_ENCODING], config[CONF_VARIANT]))
    cg.add(var.set_min_tx_duration(config[CONF_DURATION]))
    cg.add(var.set_max_tx_duration(config[CONF_BLE_ADV_MAX_DURATION]))
    cg.add(var.set_seq_duration(config[CONF_BLE_ADV_SEQ_DURATION]))
    cg.add(var.set_reversed(config[CONF_REVERSED]))
    if CONF_BLE_ADV_FORCED_ID in config and config[CONF_BLE_ADV_FORCED_ID] > 0:
        cg.add(var.set_forced_id(config[CONF_BLE_ADV_FORCED_ID]))
    else:
        cg.add(var.set_forced_id(config[CONF_ID].id))
    cg.add(var.set_show_config(config[CONF_BLE_ADV_SHOW_CONFIG]))
    if config[CONF_BLE_ADV_SHOW_CONFIG]:
        cg.add(cg.App.register_select(var.get_select_encoding()))
        cg.add(cg.App.register_number(var.get_number_duration()))
    


