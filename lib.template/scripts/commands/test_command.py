# -*- coding: koi8-r -*-
import mud

# сама команда
command_text = "тест"
# позиция, начиная с которой команду можно выполнять. constants.POS_XXX
position_min = 0
# мин. уровень игрока для выполнения команды
level_min = 0
# вероятность разхайдиться
unhide_percent = 100
def command(ch, cmd, args):
	""" первый аргумент - чар, второй команда, третий аргументы"""
	s = "Аргументы: {0}, Команда: {1}, Имя чара: {2}".format(args.decode("koi8-r"), cmd.decode("koi8-r"), ch.get_pad(0).decode("koi8-r"))
	ch.send(s.encode("koi8-r"))
