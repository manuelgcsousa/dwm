#!/bin/env sh

#
# global variables
#
sec=0


#
# Modules
#
M_datetime() {
  datetime="$(date '+%a, %b %d :: %H:%M')"
}

M_battery() {
  local percentage=$(upower -i /org/freedesktop/UPower/devices/battery_BAT0 | grep 'percentage' | awk -F: '{print int($2)}')
  local state=$(upower -i /org/freedesktop/UPower/devices/battery_BAT0 | grep 'state' | awk -F: '{ gsub(/ /, ""); print $2 }')

  if [ "$state" = 'discharging' ]; then
    battery="BAT  $percentage%"
  else
    battery="CHR  $percentage%"
  fi
}

M_volume() {
  local statusLine=$(amixer get Master | tail -n 1)
  local status=$(echo "${statusLine}" | grep -wo "on")
  volume=$(echo "${statusLine}" | awk -F ' ' '{print $4}' | tr -d '[]%')

  if [ "${status}" = 'on' ]; then
    volume="VOL  ${volume}%"
  else
    volume="VOL  0%"
  fi
}


#
# Status bar display
#
display() {
  xsetroot -name "[ $volume ]  [ $battery ]  [ $datetime ]"
}


#
# Entrypoint
#
main() {
  # register PID of bar script
  printf "$$" > $HOME/.cache/pidofbar

  # manually run modules that don't update in their own
  M_volume

  # register signals
  trap "M_volume; display"  "RTMIN"
  trap "M_battery; display" "RTMIN+2"

  while true; do
    sleep 1 & wait && {
      [ $((sec % 5 )) -eq 0 ] && M_datetime
      [ $((sec % 60)) -eq 0 ] && M_battery

      [ $((sec % 5 )) -eq 0 ] && display

      sec=$((sec + 1))
    }
  done
}
main
