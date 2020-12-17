"""Generate json report of a run"""
from __future__ import absolute_import, unicode_literals

import json
import os
import socket
import sys

from tox.version import __version__

from .command import CommandLog
from .env import EnvLog


class ResultLog(object):
    """The result of a tox session"""

    def __init__(self):
        command_log = []
        self.command_log = CommandLog(None, command_log)
        self.dict = {
            "reportversion": "1",
            "toxversion": __version__,
            "platform": sys.platform,
            "host": os.getenv(str("HOSTNAME")) or socket.getfqdn(),
            "commands": command_log,
        }

    @classmethod
    def from_json(cls, data):
        result = cls()
        result.dict = json.loads(data)
        result.command_log = CommandLog(None, result.dict["commands"])
        return result

    def get_envlog(self, name):
        """Return the env log of an environment (create on first call)"""
        test_envs = self.dict.setdefault("testenvs", {})
        env_data = test_envs.setdefault(name, {})
        return EnvLog(self, name, env_data)

    def dumps_json(self):
        """Return the json dump of the current state, indented"""
        return json.dumps(self.dict, indent=2)
