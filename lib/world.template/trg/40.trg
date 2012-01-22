#4000
триггер на скорняке ~
0 j 100
~
wait 2
switch %object.name% 
  case труп мыши
    say  Благодарствую, ты меня выручаешь, получи плату. 
    %self.gold(+1)%
    дать 1 кун %actor.name%
    mpurge труп 
  break
  case труп норки
    say    Благодарствую, получится хорошая шкурка на шубу, получи плату. 
    %self.gold(+20)%
    дать   20 кун %actor.name%
    mpurge труп 
  break
  case труп лисы
    say    Благодарствую, получится хорошая шкурка на шубу, получи плату. 
    %self.gold(+30)%
    дать   30 кун %actor.name%
    mpurge труп 
  break
  case труп медведя
    say    Благодарствую, получится превосходный тулуп, получи плату. 
    %self.gold(+80)%
    дать   80 кун %actor.name%
    mpurge труп 
  break
  case труп зайца
    say    Благодарствую, получится хорошая шапка, получи плату. 
    %self.gold(+10)%
    дать   10 кун %actor.name%
    mpurge труп 
  break
  case труп ондатры
    say    Благодарствую, получится хороший воротник, получи плату. 
    %self.gold(+40)%
    дать   40 кун %actor.name%
    mpurge труп 
  break
  case труп бобра
    say    Благодарствую, хороший мех, получи плату. 
    %self.gold(+25)%
    дать   25 кун %actor.name%
    mpurge труп 
  break
  case труп барсука
    say    Благодарствую, хороший мех, получи плату. 
    %self.gold(+25)%
    дать   25 кун %actor.name%
    mpurge труп 
  break
  case труп выдры
    say    Благодарствую, хороший мех, получи плату. 
    %self.gold(+22)%
    дать   22 кун %actor.name%
    mpurge труп 
  break
  default
    say  Зачем мне это? 
    eval getobject %object.name%
    if  %getobject.car% == труп
      mpurge труп
    else
      броси %getobject.car%.%getobject.cdr%
    end
  break
done 
~
#4001
триггер на хозяйке мельницы ~
0 q 100
~
wait 1
foreach victim %actor.group%
  if (%victim.level% < 10 && (%victim.gold% < 20) && (%victim.bank% < 20) && (%actor% != %victim%))
    msend %victim% Хозяйка сказала Вам:
    msend %victim% - Эх ты горемычн%victim.w%, голодн%victim.w% наверное, %victim.name%.
    взд
    msend %victim% - У меня для голодного всегда кус хлеба найдется...
    msend %victim% - Сейчас, подожди...
    dg_cast 'насыщение' %victim.name%
  end
done
*интересно какой мудаг поломал списки... актор груп пуста, если группы нет :(
if (%actor.level% < 10 && (%actor.gold% < 20) && (%actor.bank% < 20))
  msend %actor% Хозяйка сказала Вам:
  msend %actor% - Эх ты горемычн%actor.w%, голодн%actor.w% наверное, %actor.name%.
  взд
  msend %actor% - У меня для голодного всегда кус хлеба найдется...
  msend %actor% - Сейчас, подожди...
  dg_cast 'насыщение' %actor.name%
end
~
#4002
входнапостой~
0 r 100
~
wait 1
eval factor %self.people%
while %factor%
  eval pc %factor.next_in_room%
  if (%factor.vnum% == -1) && (%factor.level%>19) && (%factor.level%<31)
    if %factor.sex%==1
      tell %factor.name% Вырос ты уже, нечего тебе здесь штаны просиживать.
    else
      tell %factor.name% Выросла ты уже, нечего тебе здесь юбку просиживать.
    end
    tell %factor.name% Не пушу я тебя здесь на постой, иди в город, там где большие нужны.
    tell %factor.name% А от вас только проблемы здесь будут.
    mechoaround %factor% Хозяин вытолкнул %factor.vname% на улицу.
    msend %factor.name% Хозяин вытолкнул вас на улицу.
    mteleport %factor.name% 4014
    mteleport %factor.name% 4014
    mteleport %factor.name% 4014
    mteleport %factor.name% 4014
    mteleport %factor.name% 4014
    mteleport %factor.name% 4014
    mteleport %factor.name% 4014
    if %factor.sex%==1
      mat 4014 mechoaround %factor% Немного взлохмаченный %factor.name% пришел с запада.
    else
      mat 4014 mechoaround %factor% Немного взлохмаченная %factor.name% пришла с запада.
    end
  end
  if %pc.id% && %pc.id%!=%factor.id%
    makeuid factor %pc.id%
  else
    unset factor
  end
done
~
#4003
нападениежителя~
0 k 100
~
set name2 %actor%
if %actor.leader%
  set name1 %actor.leader%
  if (%name1.vnum% == -1) && (%actor.vnum% != -1))
    set name2 %actor.leader%
  end
end
if !%name2.rentable%
  msend %actor% %self.name% СМЕРТЕЛЬHО ударил%self.g% Вас.
  mdamage %actor% 300
  бежать
  halt
end
г %name2.name%, у нас найдется управа на таких обидчиков!
msend %actor.name% Вдруг прибежал здоровый парень и оглушил вас.
mechoaround %actor% Прибежал здоровый парень, оглушил %actor.rname% и куда-то уволок.
wait 1
msend %name2.name% Вы потеряли сознание.
wait 1
msend %name2.name% Вы пришли в сознание в глубокой яме от удара о землю.
if %name2.hitp%>0
  msend %name2% Кто-то сверху сильно руганулся, помянул всех ваших родственников до десятого колена.
  msend %name2% Кто-то сверху сказал: Посиди здесь, пока не успокоишься.
  msend %name2% Да, если хочешь выйти говори: простите меня дурака, может и выпущу.
  mteleport %name2% 4058
  *mat 4058 mechoaround %name2% Решетка в яме открылась и %name2.name% упал%name2.a% сверху в яму.
  *mat 4058 mechoaround %name2% Кто-то сильно руганулся, помянул всех родственников %actor.rname% до десятого колена.
  *mat 4058 mechoaround %name2% Кто-то сверху сказал: 
  *mat 4058 mechoaround %name2% Посиди здесь пока не успокоишься.
  *wait 1
  *if %name2.sex%==1
  *mat 4058 mechoaround %name2% Да, если хочешь выйти говори: простите меня дурака, может и выпущу.
  *else
  *mat 4058 mechoaround %name2% Да, если хочешь выйти говори: простите меня дуру, может и выпущу.
  *end
end
~
#4004
выходаизямы~
2 d 0
"простите меня дурака" "простите меня дуру"~
wait 1
if %random.10%<2
  wecho Решетка в яме открылась, оттуда показалась здоровенная полубритая физиономия.
  wecho Ладно, хватит хныкать, выпущу!
  wecho В следующий раз думай, прежде чем жителей обижать наших!
  if %actor.hitp%>0
    wteleport %actor.name% 4000
    wat 4000 wechoaround %actor% К решетке подошел здоровый парень, открыл замок.
    wat 4000 wechoaround %actor% Поднял решетку и одним движением вытащил кого-то.
    wat 4000 wechoaround %actor% Снова повесил массивный замок.
    if (%actor.sex% == 1)
      wat 4000 wechoaround %actor% %actor.name% размял кости, похоже в яме ему было неудобно.
    else
      wat 4000 wechoaround %actor% %actor.name% размяла кости, похоже в яме ей было неудобно.
    end
  end
else
  wechoaround %actor% Никто не прореагировал на слова %actor.rname%.
  wsend %actor% Никто не прореагировал на ваши слова.
  wsend %actor% Наверное не очень искренне все прозвучало, повторите с чувством.
end
~
#4005
призовой триг на лисе~
0 c 100
тест~
wait 1
say Actor position - %actor.position%
if (%actor.position% < 5)
  say проснуться
end
if (%actor.position% < 7)
  say встать
end
~
#4006
ПКиллер заходит к помощникам~
0 q 80
~
wait 1
if %actor.clan% != null
  halt
end
if !%actor.agressor%
  halt
end
if %actor.agressor% > 4000
  if %actor.agressor% < 4099
    set agr 1
  end
end
if %actor.agressor% > 4300
  if %actor.agressor% < 4399
    set agr 1
  end
end
if %actor.agressor% > 4800
  if %actor.agressor% < 4899
    set agr 1
  end
end
if %agr% == 1
  if %actor.sex% == 1
    крича %actor.name%, - вот он, убивец, хватай его! Вяжи его!
  else
    крича %actor.name%, мерзавка, хватай ее! Вяжи ее!
  end
  Крича %actor.name% - вот он%actor.g% где, хватай, ребяты!
  mkill %actor% 
end
~
#4007
Помощник заходик к ПКиллеру~
0 t 100
~
wait 1
if %actor.clan% != null
  halt
end
foreach char %self.pc%
  if %char.agressor% > 4000
    if %char.agressor% < 4099
      set target %char%
    break
  end
end
if %char.agressor% > 4300
  if %char.agressor% < 4399
    set target %char%
  break
end
end
if %char.agressor% > 4800
  if %char.agressor% < 4899
    set target %char%
  break
end
end
done
if !%target%
  halt
end
if %target.sex% == 1
  крича %target.name%, - вот он, убивец, хватай его! Вяжи его!
else
  крича %target.name%, мерзавка, хватай ее! Вяжи ее!
end
Крича %target.name% - вот он%target.g% где, хватай, ребяты!
появ
mforce %target.name% трепет
mkill %target% 
~
#4008
Пьянка~
0 b 18
~
stand
switch %random.36%
  case 1
    пук
  break
  case 2
    хох
  break
  case 3
    выть
    плюн
    морщ
  break
  case 4
    орать Ай ла-ла-лю-ли-ду-ду, шла девица по воду, а за девкой юной мчали два драгуна!
  break
  case 5
    бух
    тост
  break
  case 6
    просн
    орать Б-белочка.. А? Ббелочка ты хде?!
  break
  case 7
    тоска
    мыча
    ворча
    буб
  break
  case 8
    **msend %self% Нетрезвые ноги понесли вас куда-то...
    n
    e
  break
  case 9
    *msend %actor% Нетрезвые ноги понесли вас куда-то...
    s
    e
  break
  case 10
    *msend %actor% Нетрезвые ноги понесли вас куда-то...
    u
    n
  break
  case 11
    *msend %actor% Нетрезвые ноги понесли вас куда-то...
    e
    s
  break
  case 12
    орать Налейте мне, братцы, налейте ВИНА!!!
  break
  case 13
    орать По улицам ходила нетрезвая Годзилла - трех кошек задавила и семерых козлят... ИК!
  break
  case 14
    болт 'У леса на опушке снесла яйцо старушка, а мы его купили и съели наконец...'
  break
  case 15
    орать Люди злы как прокуроры, дух печального конца. От тоски у всех запоры и землистый цвет лица
  break
  case 16
    орать Царевич жил с лягушкой, как с женою,- Декомпенсированный извращенец, На сивом мерине катался, параноик, Любил других лягушек, многоженец. 
  break
  case 17
    орать Я хожу простой, но гордый, С девальвированной мордой С тягой к чистому искусству, с аллергией на козлов...
  break
  case 18
    say Здравствуй, моя Мурка, здравствуй, дорогая...
  break
  case 19
    say Здравствуй, моя Мурка, здравствуй, дорогая, Здравствуй, Мура-Мурочка моя... 
  break
  case 20
    ора Покайтесь  паразиты! Куды рыжего дели?!
  break
  case 21
    орать Эй, люди!! Тут крыша не проезжала?!
  break
  case 22
    болт  Здравствуй, моя Мурка, здравствуй, дорогая, Здравствуй, Мура-Мурочка моя, Аль тебе жилося плохо между нами, Или не хватало барахла?
  break
  case 23
    болт 'Заявляю это прямо - лично мне Одесса - мама...'
  break
  case 24
    орать Я на вас руки наложу!
  break
  case 25
    say С горя начал тать кудесить и начармил комаров!
  break
  case 26
    emot музицирует : 'А бpадатый Чеpномоp, лукомоpский пеpвый воp, Он давно Людмилу спеp, ой, хитеp!'
  break
  case 27
    say Ловко пользуется тать, тем что может он летать, зазеваешься - он хвать, и тикать.
  break
  case 28
    встать
    петь
    хохот
  break
  case 29
    eval drgold %self.gold%/10
    drop %drgold% кун
    г Берите! Все берите!!! 
    emot истово рванул%actor.q% рубаху на груди
  break
  case 30
    орать Эй, стотыщ тушканчиков, айда за сопку биться!
  break
  case 31
    гс Кто к нам с чем зачем - тот от того и того!!!
  break
  case 32
    гс Если скажем, взять мыша, ну и бережно держа, навтыкать в него иголок - вы получите ежа!
  break
  case 33
    гс Если взять того ежа, нос зажав, чтоб не дышал, в воду где поглыбже бросить - вы получите ерша!
  break
  case 34
    гс Если взять того ерша, голову в тиски зажать, и за хвостик сильно дернуть - вы получите ужа!
  break
  case 35
    гс Если взять того ужа, два заточенных ножа... Впрочм, он помрет наверно - но идея хороша!
  break
  case 36
    гс Об Йожа! Куда ни падыдешь - так ыть и об йожа! Йожимаффффиа!! Йоожжы запалаонили фсе! 
  break
done
~
#4009
Скиллы~
1 c 1
скилл~
log &CСработал триг на выдачу скиллов - %actor.name% - %actor.realroom%&n
oecho Чититят только колзы и педерасты.
~
#4010
Подключение ПЬЯНКИ~
1 c 2
прочитать~
if !%arg.contains(книгу)%
  %send% %actor% чего читаете?
  halt
end
osend %actor% Из раскрывшейся книги вам в нос ударила волна хмельных запахов...
oechoaround %actor% %actor.name% открыл%actor.g% книгу пьянки - крепкий винный запах ударил в нос.
attach 4008 %actor.id%
wait 1
opurge %self%
~
#4011
Тест портала~
2 z 100
~
wportal 50040 2
~
#4012
Подобрали книжку~
1 g 100
~
wait 1
oforce %actor% прочитать книгу
osend %actor% Вы принялись читать книгу.
~
#4014
Экспа~
0 f 100
~
log вызван читерный триггер на прибавление экспы, юзавший %actor.name%
~
#4015
Рента~
0 q 100
~
wait 1
%actor.loadroom(60036)%
mteleport %actor% 60036
log Рента - %actor.name%
~
#4016
Крысса жретЪ~
0 bj 100
~
wait 1
set items %self.objs%
set item %items.car%
if %item%
  mecho Крыса сожрала %item.vname%.
  wait 1
  mpurge %item%
end
~
#4017
Крыса спаит~
0 b 14
~
wait 1
switch %random.8%
  case 1
    emot достала клочок бересты и прислушалась к разговорам дружины
  break
  case 2
    emot отошла в сторонку, достала какую-то зеленую коробочку и начала тихо попискивать в нее
    г пиип-пип-пип, пиип-пиип-пиип, пиип-пип, пип
  break
  case 3
    emot уселась под доской с новостями дружины и стала быстро переписывать ее на клочок бересты
  break
  case 4
    emot по локоть запустила лапу в хранилище Дружины и начала что-то там ощупывать
  break
  case 5
    пищать
  break
  case 6
    mecho Жирная крыса принялась искать чьи-то следы.
  break
  case 7
    set target %random.pc%
    emot тщательно обнюхала %target.vname%
    гад
  break
  case 8
    emot достала откуда-то три цветочных горшка и водрузила на подоконник
  break
done
~
#4018
Падальщик воет~
0 b 15
~
крича Ууууууууу!!!
~
#4019
Крыса жрет стаф~
0 c 1
тест~
foreach item %self.objs%
  mecho %item.name%
  wait 1
done
~
#4020
Сказитель рассказывает сказки~
0 d 3
расскажи сказку~
wait 1
if (%actor.vnum% != -1)
  halt
end
if !%actor.rentable%
  хмур
  say Не дело от боя бегать... Приходи-ка потом.
  halt
end
context 4000
msend %actor% Сказитель внимательно посмотрел на Вас.
mechoaround %actor% Сказитель внимательно посмотрел на %actor.vname%.
wait 1s
switch %actor.class%
  case 3
    if %actor.quested(12200)%
      say Тебе я уже рассказывал сказку, надо и другим оставить.
      хих
      halt
    end
    if (%fighterstory% == 1)
      wait 1s
      say Устал я... Передохну малость.
      say Подходи через денек - утро вечера мудренее..
      halt
    end
    say Сказку... Ну что же, слушай.
    say То ли было, то ли не было, толь недавно, толь давнёшенько...
    msend %actor% Вы заслушались сказочника и ваши мысли сами собой побежали за его словами.
    mechoaround %actor% Странный туман скрыл %actor.vname% и сказочника.
    mteleport %actor% 12200
    set fighterstory 1
    worlds fighterstory
    halt
  break
  case 7
    if %actor.quested(12800)%
      say Тебе я уже рассказывал сказку, надо и другим оставить.
      хих
      halt
    end
    if (%defendermagestory% == 1)
      wait 1s
      say Устал я... Передохну малость.
      say Подходи через денек - утро вечера мудренее..
      halt
    end
    say Сказку... Ну что же, слушай.
    say За лесами, за полями, да за синими морями...
    msend %actor% Вы заслушались сказочника и ваши мысли сами собой побежали за его словами.
    mechoaround %actor% Странный туман скрыл %actor.vname% и сказочника.
    mteleport %actor% 12800
    set defendermagestory 1
    worlds defendermagestory
    halt
  break
  case 13
    if %actor.quested(12900)%
      say Тебе я уже рассказывал сказку, надо и другим оставить.
      хих
      halt
    end
    if (%druidstory% == 1)
      wait 1s
      say Устал я... Передохну малость.
      say Подходи через денек - утро вечера мудренее.
      halt
    end
    say Сказку... Ну что же, слушай.
    say Во времена стародавние, в лета былинные...
    msend %actor% Вы заслушались сказочника и ваши мысли сами собой побежали за его словами.
    mechoaround %actor% Странный туман скрыл %actor.vname% и сказочника.
    mteleport %actor% 12900
    set druidstory 1
    worlds druidstory
    halt
  break
done
say Извини, для тебя у меня нет сказки...
взд
~
$~
