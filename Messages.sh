#! /bin/sh
$PREPARETIPS > tips.cpp
$XGETTEXT *.cpp -o $podir/kgpg.pot
rm -f tips.cpp
