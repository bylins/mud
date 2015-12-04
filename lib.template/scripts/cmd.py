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
			command = __import__("commands." + name, globals(), locals(), ("commands",)).Command
		except:
			log_sys("Error importing command " + name)
			continue
		try:
			mud.register_global_command(command.command_text, command.command_text.encode("koi8-r"), command.command, command.position_min, command.level_min, command.unhide_percent)
			reg_cmd(command.command_text, command.command, command.position_min, command.level_min, command.unhide_percent)
			log_sys("Команда {} зарегистрирована.".format(command.command_text))
		except:
			log_sys("[PythonError] Команда {} не зарегистрирована.".format(command.command_text))
			continue


