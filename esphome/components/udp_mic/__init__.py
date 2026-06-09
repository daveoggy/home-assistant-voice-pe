import os
import subprocess

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import microphone
from esphome.const import CONF_ID, CONF_PORT

DEPENDENCIES = ["microphone", "network"]
CODEOWNERS = ["@daveoggy"]

udp_mic_ns = cg.esphome_ns.namespace("udp_mic")
UdpMic = udp_mic_ns.class_("UdpMic", cg.Component)

CONF_MICROPHONE = "microphone"
CONF_IP = "ip"
CONF_CHANNELS = "channels"
CONF_BITS_SHIFT = "bits_shift"


def _git_version():
    """Short git hash of the (cached) component checkout — also reveals stale caches."""
    try:
        d = os.path.dirname(os.path.abspath(__file__))
        return subprocess.check_output(
            ["git", "-C", d, "describe", "--always", "--dirty", "--tags"],
            stderr=subprocess.DEVNULL,
        ).decode().strip()
    except Exception:
        return "nogit"


CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(UdpMic),
        cv.Required(CONF_MICROPHONE): cv.use_id(microphone.Microphone),
        cv.Required(CONF_IP): cv.string,
        cv.Optional(CONF_PORT, default=10500): cv.port,
        cv.Optional(CONF_CHANNELS, default=2): cv.int_range(min=1, max=2),
        cv.Optional(CONF_BITS_SHIFT, default=16): cv.int_range(min=0, max=24),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    mic = await cg.get_variable(config[CONF_MICROPHONE])
    cg.add(var.set_microphone(mic))
    cg.add(var.set_target(config[CONF_IP], config[CONF_PORT]))
    cg.add(var.set_channels(config[CONF_CHANNELS]))
    cg.add(var.set_bits_shift(config[CONF_BITS_SHIFT]))
    cg.add(var.set_version(_git_version()))
