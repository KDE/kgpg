#! /bin/sh
$PREPARETIPS > tips.cpp
$XGETTEXT *.cpp core/*.cpp editor/*.cpp model/*.cpp -o $podir/kgpg.pot
rm -f tips.cpp
