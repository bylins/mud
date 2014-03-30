# -*- coding: koi8-r -*-

import sys
import traceback
import mud, constants

def log_err(msg):
    mud.log(msg + "\n" + traceback.format_exc(), channel=constants.ERRLOG)
    