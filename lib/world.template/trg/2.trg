* BRusMUD trigger file v1.0
#200
триггнаелке~
1 c0 100
хоровод~
osend %actor.name% Вы вспомнили детство и с радостью стали прыгать вокруг елочки.
wait 2s
if %actor.sex%==1
oechoaround %actor% %actor.name% вспомнил детство и с радостью стал прыгать вокруг елочки.
else
oechoaround %actor% %actor.name% вспомнила детство и с радостью стала прыгать вокруг елочки.
end
eval mhitp %actor.maxhitp%-10
if %actor.hitp% < %mhitp%
%actor.hitp(+10)%
osend %actor.name% Вдруг с елки упала на Вас иголка и вспыхнула.
osend %actor.name% Вы почувстсвовали себя лучше.
end

~
$
$
