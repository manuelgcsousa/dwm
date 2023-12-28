#!/bin/sh

$HOME/.config/dwm/resources/bar.sh &
nm-applet &
xsetroot -solid "#252525"

exec dwm
