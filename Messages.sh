#! /bin/sh
$PREPARETIPS > tips.cpp
$EXTRACTRC `find . -name \*.rc` >> rc.cpp
$XGETTEXT *.cpp core/*.cpp editor/*.cpp model/*.cpp transactions/*.cpp -o $podir/kgpg.pot
rm -f tips.cpp
