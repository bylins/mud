# -*- coding: koi8-r -*-

"""Реализация интерактивной консоли для иммов."""

import sys
import code
import pydoc
import constants
import mud

class HelpCommand(object):
	"""Emulation of the 'help' command found in the Python interactive shell.
	"""

	def __repr__(self):
		#return "Type help(object) to get help about object."
		return "Наберите help(object) чтобы получить справку по object."

	def __call__(self,*args,**kwargs):
		return pydoc.help(*args,**kwargs)


class ExitConsoleCommand(object):
	"""An object that can be used as an exit command that can close the console or print a friendly message for its repr.
	"""

	def __init__(self, ch):
		self.ch = ch

	def __repr__(self):
		#return "Type exit() to exit the console"
		return "Наберите exit() чтобы выйти из консоли."

	def __call__(self):
		self.ch.close_console()


class PythonConsole(code.InteractiveConsole):

	def __init__(self, ch):
		self.namespace = {
			"help":HelpCommand(),
			"exit":ExitConsoleCommand(ch),
			"quit":ExitConsoleCommand(ch),
			"mud": mud,
			"constants": constants,
			"self": ch,
		}
		code.InteractiveConsole.__init__(self, locals=self.namespace)
		self.ch = ch
		self.prompt = ">>>"
		self._output = []


	def get_prompt(self):
		return self.prompt

	def write(self, data):
		self._output.append(data)

	def push(self, line):
		#self.write("%s %s\r\n" % (self.prompt, line))
		# Capture stdout/stderr output as well as code interaction.
		stdout, stderr = sys.stdout, sys.stderr
		sys.stdout = sys.stderr = self
		more = code.InteractiveConsole.push(self, line)
		sys.stdout, sys.stderr = stdout, stderr
		self.ch.page_string("".join(self._output))
		self._output = []
		self.prompt = "..." if more else ">>>"
		return more
