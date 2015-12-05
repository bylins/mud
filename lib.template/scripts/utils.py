# -*- coding: koi8-r -*-

import sys
import traceback
import mud, constants

def log_sys(msg):
    try:
        stacktrace = traceback.format_exc()
    except AttributeError:
        stacktrace = "<no any exceptions were occurred>"
    mud.log(msg + "\n" + stacktrace, channel=constants.SYSLOG)
	
def reg_cmd(cmd_text, command,  position_min, level_min, unhide_percent):
    mud.register_global_command(cmd_text, cmd_text.encode("koi8-r"), command, position_min, level_min, unhide_percent)
