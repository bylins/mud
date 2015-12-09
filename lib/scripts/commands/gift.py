# -*- coding: koi8-r -*-
import mud, os, random, sys
from utils import load_obj



# сама команда
command_text = "подарок"
# позиция, начиная с которой команду можно выполнять. constants.POS_XXX
position_min = 3
# мин. уровень игрока для выполнения команды
level_min = 0
# вероятность разхайдиться
unhide_percent = 100
# массив, в котором будем хранить мыла
array_email = []
# массив, в котором хранятся внумы подарков
array_vnum = []
def command(ch, cmd, args):
	""" первый аргумент - чар, второй команда, третий аргументы"""
	global array_email, array_vnum
	email = ch.email
	if ch.remort < 4:
		ch.send("\r\nПодрасти немного.\r\n")
		return
	if email in array_email:
		ch.send("\r\nВы уже получили свой подарок!\r\n")
		return
	array_email.append(email)
	with open("misc/emails_gift.txt", "a") as fp:
		fp.write(email + "\n")
	vnum = random.choice(array_vnum)
	obj = load_obj(vnum)
	ch.obj_to_char(obj)
	ch.send("\r\nВ этом году вы вели себя хорошо и получили {}.\r\n".format(obj.get_pad(3)))
	
def init():
	global array_email, array_vnum
	if os.path.exists("misc/emails_gift.txt"):
		with open("misc/emails_gift.txt", "r") as fp:
			array_email = [line.strip() for line in fp]
	else:
		with open("misc/emails_gift.txt", "w") as fp:
			pass
	with open("misc/vnums_gift.lst", "r") as fp:
		array_vnum = [int(line.strip()) for line in fp]
		
		
		
		
		