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
from esphome.core.entity_helpers import setup_entity
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

FanLampEncoderV1 = bleadvcontroller_ns.class_('FanLampEncoderV1')
FanLampEncoderV2 = bleadvcontroller_ns.class_('FanLampEncoderV2')
ZhijiaEncoderV0 = bleadvcontroller_ns.class_('ZhijiaEncoderV0')
ZhijiaEncoderV1 = bleadvcontroller_ns.class_('ZhijiaEncoderV1')
ZhijiaEncoderV2 = bleadvcontroller_ns.class_('ZhijiaEncoderV2')

BLE_ADV_ENCODERS = {
    "fanlamp_pro" :{
        "variants": {
            "v1": {
                "class": FanLampEncoderV1,
                "args": [ 0x83, False ],
                "max_forced_id": 0xFFFFFF,
                "ble_param": [ 0x19, 0x03 ],
                "header": [0x77, 0xF8],
            },
            "v2": {
                "class": FanLampEncoderV2,
                "args": [ [0x10, 0x80, 0x00], 0x0400, False ],
                "ble_param": [ 0x19, 0x03 ],
                "header": [0xF0, 0x08],
            },
            "v3": {
                "class": FanLampEncoderV2,
                "args": [ [0x20, 0x80, 0x00], 0x0400, True ],
                "ble_param": [ 0x19, 0x03 ],
                "header": [0xF0, 0x08],
            },
            "v1a": {
                "legacy": True,
                "msg": "please use 'other - v1a' for exact replacement, or 'fanlamp_pro' v1 / v2 / v3 if effectively using FanLamp Pro app",
            },
            "v1b": {
                "legacy": True,
                "msg": "please use 'other - v1b' for exact replacement, or 'fanlamp_pro' v1 / v2 / v3 if effectively using FanLamp Pro app",
            },
        },
        "default_variant": "v3",
        "default_forced_id": 0,
    },
    "lampsmart_pro": {
        "variants": {
            "v1": {
                "class": FanLampEncoderV1,
                "args": [ 0x81 ],
                "max_forced_id": 0xFFFFFF,
                "ble_param": [ 0x19, 0x03 ],
                "header": [0x77, 0xF8],
            },
            # v2 is only used by LampSmart Pro - Soft Lighting
            "v2": {
                "class": FanLampEncoderV2,
                "args": [ [0x10, 0x80, 0x00], 0x0100, False ],
                "ble_param": [ 0x19, 0x03 ],
                "header": [0xF0, 0x08],
            },
            "v3": {
                "class": FanLampEncoderV2,
                "args": [ [0x30, 0x80, 0x00], 0x0100, True ],
                "ble_param": [ 0x19, 0x03 ],
                "header": [0xF0, 0x08],
            },
            "v1a": {
                "legacy": True,
                "msg": "please use 'other - v1a' for exact replacement, or 'lampsmart_pro' v1 / v3 if effectively using LampSmart Pro app",
            },
            "v1b": {
                "legacy": True,
                "msg": "please use 'other - v1b' for exact replacement, or 'lampsmart_pro' v1 / v3 if effectively using LampSmart Pro app",
            },
        },
        "default_variant": "v3",
        "default_forced_id": 0,
    },
    "zhijia": {
        "variants": {
            "v0": {
                "class": ZhijiaEncoderV0,
                "args": [],
                "max_forced_id": 0xFFFF,
                "ble_param": [ 0x1A, 0xFF ],
                "header": [ 0xF9, 0x08, 0x49 ],
            },
            "v1": {
                "class": ZhijiaEncoderV1,
                "args": [],
                "max_forced_id": 0xFFFFFF,
                "ble_param": [ 0x1A, 0xFF ],
                "header": [ 0xF9, 0x08, 0x49 ],
            },
            "v2": {
                "class": ZhijiaEncoderV2,
                "args": [],
                "max_forced_id": 0xFFFFFF,
                "ble_param": [ 0x1A, 0xFF ],
                "header": [ 0x22, 0x9D ],
            },
        },
        "default_variant": "v2",
        "default_forced_id": 0xC630B8,
    },
    "remote" : {
        "variants": {
            "v1": {
                "class": FanLampEncoderV1, 
                "args": [ 0x83, False, True ],
                "max_forced_id": 0xFFFFFF,
                "ble_param": [ 0x00, 0xFF ],
                "header":[0x56, 0x55, 0x18, 0x87, 0x52], 
            },
            "v3": {
                "class": FanLampEncoderV2,
                "args": [ [0x10, 0x00, 0x56], 0x0400, True ],
                "ble_param": [ 0x02, 0x16 ],
                "header": [0xF0, 0x08],
            },
        },
        "default_variant": "v3",
        "default_forced_id": 0,
    },
# legacy lampsmart_pro variants v1a / v1b / v2 / v3
# None of them are actually matching what FanLamp Pro / LampSmart Pro apps are generating
# Maybe generated by some remotes, kept here for backward compatibility, with some raw sample
    "other" : {
        "variants": {
            "v1b": {
                "class": FanLampEncoderV1,
                "args": [ 0x81, True, True, 0x55 ],
                "max_forced_id": 0xFFFFFF,
                "ble_param": [ 0x02, 0x16 ],
                "header":  [0xF9, 0x08],
                # 02.01.02.1B.03.F9.08.49.13.F0.69.25.4E.31.51.BA.32.08.0A.24.CB.3B.7C.71.DC.8B.B8.97.08.D0.4C (31)
            },
            "v1a": {
                "class": FanLampEncoderV1, 
                "args": [ 0x81, True, True ],
                "max_forced_id": 0xFFFFFF,
                "ble_param": [ 0x02, 0x03 ],
                "header": [0x77, 0xF8],
                # 02.01.02.1B.03.77.F8.B6.5F.2B.5E.00.FC.31.51.50.CB.92.08.24.CB.BB.FC.14.C6.9E.B0.E9.EA.73.A4 (31)
            },
            "v2": {
                "class": FanLampEncoderV2,
                "args": [ [0x10, 0x80, 0x00], 0x0100, False ],
                "ble_param": [ 0x19, 0x16 ],
                "header": [0xF0, 0x08],
                # 02.01.02.1B.16.F0.08.10.80.0B.9B.DA.CF.BE.B3.DD.56.3B.E9.1C.FC.27.A9.3A.A5.38.2D.3F.D4.6A.50 (31)
            },
            "v3": {
                "class": FanLampEncoderV2,
                "args": [ [0x10, 0x80, 0x00], 0x0100, True ],
                "ble_param": [ 0x19, 0x16 ],
                "header": [0xF0, 0x08],
                # 02.01.02.1B.16.F0.08.10.80.33.BC.2E.B0.49.EA.58.76.C0.1D.99.5E.9C.D6.B8.0E.6E.14.2B.A5.30.A9 (31)
            },
        },
        "default_variant": "v1b",
        "default_forced_id": 0,
    },
}

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
        cv.Optional(CONF_INDEX, default=0): cv.All(cv.positive_int, cv.Range(min=0, max=255)),
    }
)

def validate_legacy_variant(config):
    encoding = config[CONF_BLE_ADV_ENCODING]
    variant = config[CONF_VARIANT]
    pv = BLE_ADV_ENCODERS[ encoding ]["variants"][ variant ]
    if pv.get("legacy", False):
        raise cv.Invalid("DEPRECATED '%s - %s', %s" % (encoding, variant, pv["msg"]))
    return config

def validate_forced_id(config):
    encoding = config[CONF_BLE_ADV_ENCODING]
    variant = config[CONF_VARIANT]
    forced_id = config[CONF_BLE_ADV_FORCED_ID]
    params = BLE_ADV_ENCODERS[ encoding ]
    max_forced_id = params["variants"][ variant ].get("max_forced_id", 0xFFFFFFFF)
    if forced_id > max_forced_id :
        raise cv.Invalid("Invalid 'forced_id' for %s - %s: %s. Maximum: 0x%X." % (encoding, variant, forced_id, max_forced_id))
    return config

CONFIG_SCHEMA = cv.All(
    cv.Any(
        *[ CONTROLLER_BASE_CONFIG.extend(
            {
                cv.Required(CONF_BLE_ADV_ENCODING): cv.one_of(encoding),
                cv.Optional(CONF_VARIANT, default=params["default_variant"]): cv.one_of(*params["variants"].keys()),
                cv.Optional(CONF_BLE_ADV_FORCED_ID, default=params["default_forced_id"]): cv.hex_uint32_t,
            }
        ) for encoding, params in BLE_ADV_ENCODERS.items() ]
    ),
    validate_forced_id,
    validate_legacy_variant,
    cv.only_on([PLATFORM_ESP32]),
)

async def entity_base_code_gen(var, config, platform):
    await cg.register_parented(var, config[CONF_BLE_ADV_CONTROLLER_ID])
    await cg.register_component(var, config)
    await setup_entity(var, config, platform)

class BleAdvRegistry:
    handler = None
    @classmethod
    def get(cls):
        if not cls.handler:
            hdl_id = ID("ble_adv_static_handler", type=BleAdvHandler)
            cls.handler = cg.new_Pvariable(hdl_id)
            cg.add(cls.handler.set_component_source(cg.RawExpression('LOG_STR("ble_adv_handler")')))
            cg.add(cg.App.register_component(cls.handler))
            for encoding, params in BLE_ADV_ENCODERS.items():
                for variant, param_variant in params["variants"].items():
                    if "class" in param_variant:
                        enc_id = ID("enc_%s_%s" % (encoding, variant), type=param_variant["class"])
                        enc = cg.new_Pvariable(enc_id, encoding, variant, *param_variant["args"])
                        cg.add(enc.set_ble_param(*param_variant["ble_param"]))
                        cg.add(enc.set_header(param_variant["header"]))
                        cg.add(cls.handler.add_encoder(enc))
        return cls.handler

async def to_code(config):
    hdl = BleAdvRegistry.get()
    var = cg.new_Pvariable(config[CONF_ID])
    cg.add(var.set_setup_priority(300)) # start after Bluetooth
    await cg.register_component(var, config)
    await setup_entity(var, config, "ble_adv_controller")
    cg.add(var.set_handler(hdl))
    cg.add(var.set_encoding_and_variant(config[CONF_BLE_ADV_ENCODING], config[CONF_VARIANT]))
    cg.add(var.set_min_tx_duration(config[CONF_DURATION], 100, 500, 10))
    cg.add(var.set_max_tx_duration(config[CONF_BLE_ADV_MAX_DURATION]))
    cg.add(var.set_seq_duration(config[CONF_BLE_ADV_SEQ_DURATION]))
    cg.add(var.set_reversed(config[CONF_REVERSED]))
    if CONF_BLE_ADV_FORCED_ID in config and config[CONF_BLE_ADV_FORCED_ID] > 0:
        cg.add(var.set_forced_id(config[CONF_BLE_ADV_FORCED_ID]))
    else:
        cg.add(var.set_forced_id(config[CONF_ID].id))
    cg.add(var.set_show_config(config[CONF_BLE_ADV_SHOW_CONFIG]))


