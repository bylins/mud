# -*- coding: koi8-r -*-
import locale, sys
import constants
import mud
import pkgutil
import commands
from utils import log_sys, reg_cmd

def initialize():
	for loader, name, isPkg in pkgutil.iter_modules(commands.__path__):		
		if name.startswith("_"):			
			continue
		try:
			command = __import__("commands." + name, globals(), locals(), ("commands",))
		except:
			log_sys("Error importing command " + name)
			continue
		try:
			if "init" in dir(command):
				command.init()
		except:
			log_sys("Error init command " + name)
			continue
		try:
			mud.register_global_command(command.command_text, command.command_text.encode("koi8-r"), command.command, command.position_min, command.level_min, command.unhide_percent)
			reg_cmd(command.command_text, command.command, command.position_min, command.level_min, command.unhide_percent)
		except:
			s = "[PythonError] Команда {} не зарегистрирована.".format(command.command_text)
			log_sys(s.encode("koi8-r"))
			continue


