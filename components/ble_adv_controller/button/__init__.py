import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import button

from esphome.const import (
    CONF_OUTPUT_ID,
    ENTITY_CATEGORY_CONFIG,
    DEVICE_CLASS_IDENTIFY,
)

from .. import (
    bleadvcontroller_ns,
    ENTITY_BASE_CONFIG_SCHEMA,
    entity_base_code_gen,
    BleAdvEntity,
)

from ..const import (
    CONF_BLE_ADV_COMMANDS,
    CONF_BLE_ADV_CMD,
    CONF_BLE_ADV_ARGS,
    CONF_BLE_ADV_NB_ARGS,
)

def validate_cmd(cmd):
    if not cmd in CONF_BLE_ADV_COMMANDS:
        raise cv.Invalid("%s '%s' not in %s" % (CONF_BLE_ADV_CMD, cmd, str(CONF_BLE_ADV_COMMANDS.keys())))
    return cmd

BleAdvButton = bleadvcontroller_ns.class_('BleAdvButton', button.Button, BleAdvEntity)

CONFIG_SCHEMA = cv.All(
    button.button_schema(
        BleAdvButton,
        device_class=DEVICE_CLASS_IDENTIFY,
        entity_category=ENTITY_CATEGORY_CONFIG,
    ).extend(
        {
            cv.Required(CONF_BLE_ADV_CMD): validate_cmd,
            cv.Optional(CONF_BLE_ADV_ARGS): cv.ensure_list(cv.uint8_t),
        }
    ).extend(ENTITY_BASE_CONFIG_SCHEMA),
)

async def to_code(config):
    # validate the number of args
    nb_args = 0
    if CONF_BLE_ADV_ARGS in config:
        nb_args = len(config[CONF_BLE_ADV_ARGS])
    cmd = config[CONF_BLE_ADV_CMD]
    params = CONF_BLE_ADV_COMMANDS[cmd]
    nb_args_cmd = params[CONF_BLE_ADV_NB_ARGS]
    if nb_args != nb_args_cmd:
        raise cv.Invalid("Invalid number of arguments for '%s': %d, should be %d" % (cmd, nb_args, nb_args_cmd))

    # perform code gen
    var = await button.new_button(config)
    await entity_base_code_gen(var, config, "button")
    cg.add(var.set_cmd(params[CONF_BLE_ADV_CMD]))
    if nb_args > 0:
        cg.add(var.set_args(config[CONF_BLE_ADV_ARGS]))
