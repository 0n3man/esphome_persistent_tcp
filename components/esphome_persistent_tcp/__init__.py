import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID

DEPENDENCIES = ['wifi']
AUTO_LOAD = []

esphome_persistent_tcp_ns = cg.esphome_ns.namespace('esphome_persistent_tcp')
PersistentTCPClient = esphome_persistent_tcp_ns.class_('PersistentTCPClient', cg.Component)

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(PersistentTCPClient),
    cv.Required('host'): cv.string,
    cv.Required('port'): cv.port,
}).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    cg.add(var.set_host(config['host']))
    cg.add(var.set_port(config['port']))
