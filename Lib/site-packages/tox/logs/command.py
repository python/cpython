from __future__ import absolute_import, unicode_literals


class CommandLog(object):
    """Report commands interacting with third party tools"""

    def __init__(self, env_log, list):
        self.envlog = env_log
        self.list = list

    def add_command(self, argv, output, retcode):
        data = {"command": argv, "output": output, "retcode": retcode}
        self.list.append(data)
        return data
