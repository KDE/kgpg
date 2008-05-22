#! /bin/sh
$PREPARETIPS > tips.cpp
$XGETTEXT *.cpp core/*.cpp -o $podir/kgpg.pot
rm -f tips.cpp
