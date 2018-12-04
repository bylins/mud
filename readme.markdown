# BYLINS MUD README

## Введение

Эта инструкция предназначается для сборки мада под Windows, под линукс вы можете посмотреть немного устаревшую статью вот здесь:
https://github.com/bylins/mud/wiki/%D0%A1%D1%82%D0%B0%D1%80%D1%8B%D0%B9-README

## Необходимые действия дл работы в Ubuntu

Для начала нам необходимо установить последний Git или как минимум версию 2.18 (в комплекте идет 2.17 и он не подходит)

    sudo sudo add-apt-repository ppa:git-core/ppa
    sudo apt update
    sudo apt install git

После этого необходимо добавить пакеты необходимые для работы с исходниками Былин

    sudo apt-get install libz-dev cmake g++ clang python-dev libboost-python-dev libboost-system-dev mercurial libboost-dev libboost-filesystem-dev

Если я правильно помню mercurial мы не используем но на будущее если вернемся на него пусть будет : ). В целом все готово для работы с исходниками поэтому делаем:

    git clone https://github.com/bylins/mud
    
Теперь на вашем копьютере/ноутбуке есть копия былин, нам необходимо собрать бинарник для запуска игры

    cd mud
    mkdir build
    cd build
    cmake -DBUILD_TESTS=NO .. 

после этого у нас должно появится сообщение 

-- Configuring done
-- Generating done

Ну чтож запускаем сборку

    cmake --build .
    
получаем

    [100%] Linking CXX executable circle
    [100%] Built target circle

все, код собран осталось несколько манипуляций для запуска игры. Сначала скопируем полученный бинарник circle в корень игры. Дальше необходимо создать папку lib и скопировать в нее содержимое папки lib.template. Теперь мы готовы к игре полностью : )

    ./circle

Урааа :

    Using log file 'syslog' with LINE buffering. Opening in APPEND mode.
    Using log file 'log/errlog.txt' with NO buffering. Opening in REWRITE mode.
    Using log file 'log/imlog.txt' with FULL buffering. Opening in REWRITE mode.
    Using log file 'log/msdp.txt' with LINE buffering. Opening in REWRITE mode.
    Using log file 'log/money.txt' with LINE buffering. Opening in REWRITE mode.
    Bylins server will use asynchronous output into syslog file.
    Game started.

игра запущенна.

Теперь поговорим о правках. Если вы не хотите каждый раз копировать полный код Былин, а забирать только правки необходимо в каталоге с локальной версией игры выполнить команду:
    
    git remote add upstream https://github.com/bylins/mud
    
После этого вы сможете отслеживать изменения в исходниках на серевере

    git fetch upstream
    git merge upstream/master

Насчет внесения изменения в сам код вам необходимо связатся с руководством проэкта.


## Необходимое окружение для сборки проекта под Windows

ВНИМАНИЕ! АХТУНГ! АЛЯРМ! Если у вас что-то уже стояло из нижеперчисленного, то установка всего свежего может привести к весьма странным багам при сборке Былин. Прошу это учитывать.

[Cmake](https://cmake.org/download/) - Подойдет любая версия, начиная с 2.8

[Boost](https://sourceforge.net/projects/boost/files/boost-binaries/1.68.0/) - Нужен файл boost_1_68_0-msvc-14.1-64.exe, если у вас студия отличается от Visual Studio Community 2017, то вам, вероятно, нужна другая версия буста. Сама версия буста нужна выше 1.54 с библиотеками system, filesystem, locale

[Git](https://git-scm.com/downloads) - Нужна версия начиная с 2.19 (учтите, что у гита версия 2.19 новее версии 2.6/2.7)

[Microsoft Visual Studio 2017 Community](https://visualstudio.microsoft.com/ru/downloads/) - Она полностью бесплатная

## Установка

При установке git, boost, можно оставить все значения дефолтными. 

При установке cmake отметьте галочку Add Cmake to system path

При установки студии нужно отметить:
  1. Базовые компоненты Visual Studio C++
  2. Инструменты Visual C++ для CMake
  3. Последние инструменты v141 версии 14.15 VC++ 2017 версии 15.8 (здесь у вас версия может быть немного другая)
  4. MSBuild
  5. Средства профилирования C++
  6. Пакет SDK для Windows 10

## Настройка окружения

Открываем консоль (cmd.exe).

Дальше скачиваем наш репозиторий. Пишем:

    git clone https://github.com/bylins/mud

После этого переходим в каталог репозитория командой cd:

    cd mud

Создаем каталог build, переходим в него, и запускаем сборку проекта под студию

    mkdir build
    cd build
    cmake -DBUILD_TESTS=NO -DCMAKE_BUILD_TYPE=Test -DBOOST_ROOT:PATH=O:/boost/ -DCMAKE_LIBRARY_PATH:PATH=O:/boost/lib/ -DBOOST_LIBRARYDIR=O:/boost/lib/ -G "Visual Studio 15 2017 Win64" ..

Где O:/boost - это путь до буста, а O:/boost/lib - путь до каталога с библиотеками (по дефолту вместо lib там lib_какие-то_циферки)

Дальше открываем решение bylins.sln, которое появилось в каталоге build (нужно зайти туда через Проводник)

## Компиляция, запуск и отладка

Нажимаем Сборка->Собрать решение

Заходим в наш каталог mud. Копируем lib.template (ИМЕННО КОПИРУЕМ) в build и переименовываем в lib

Дальше разворачиваем студию и нажимаем Локальный отладчик Windows, вуаля, мад должен заработать. Подключаться по адресу localhost 4000. Первый созданный персонаж автоматически становится иммортал 34 уровня с максимальными привилегиями.

## Редактирование кода

Чтобы редактировать исходники в koi8-r, в студии, в обозревателе решения, правой кнопкой по .cpp или .h файлу->открыть с помощью->Редактор исходного кода C++(с кодировкой)->Выбираем koi8-r

## По работе с репозиторием
Чтобы перенести новые изменения из официального репозитория в свой рабочий, делаем следующее:

    $ cd fork/mud
    $ hg pull -r default https://bitbucket.org/bylins/mud
    $ hg update
    $ hg push

Далее, если Вы сделали в коде Вашей рабочей копии (`..\fork\mud`) какие-либо изменения и хотите, чтобы их применили на официальном сервере, то делать надо следующее:

  * cоздаете в директори `..\fork\mud` текстовой файл `commit.txt`.
  * описываете в нем изменения, которые вы сделали.
  * сохраняете файл (`commit.txt`) в кодировке 65001 (UTF-8).
  * запускаете Cygwin.
  * `cd fork/mud`
  * `hg commit -l commit.txt -u "UNAME <UEMAIL>"`, что является первым способом, с использованием текстового файла.

Второй же способ - прямая передача сообщения:

    $ hg commit -m "UMESSAGE" -u "UNAME <UEMAIL>"

*UNAME* – должно быть Вашим именем на сайте bitbucket.com.  
*UEMAIL* – должен быть Вашей почтой, указанной при регистрации на bitbucket.com.  
*UMESSAGE* – должно содержать написанное Вами сообщение об изменениях.

  * После того, как добавили коммит вводим: `hg push`.
  * Пройдите на Ваш репозиторий на bitbucket’е, там уже должна была появится запись с изменениями, которые Вы сделали в коде.
  * Если запись не появилась, то попробуйте все сделать с начала со вниманием и не забудьте выполнить перед этим команду: `hg rollback` – чтобы откатить запись коммита в логе.

Итак. Запись появилась, и теперь Вы хотите применить изменения в официальном коде Былин. Делаете следующее:

  - пройдите на свой репозиторий на bitbucked.com.
  - сверху справа будет большая кнопка «Create pull request», нажимайте на неё.
  - описывайте изменения в Title и Description и нажимайте на кнопку внизу «Send pull request». После этого на официальном репозитории, во вкладке «Pull requests» должна появится Ваша заявка. Теперь остается только ждать, когда её одобрят старшие админы.
