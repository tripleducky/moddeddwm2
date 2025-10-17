#!/bin/sh

run() {
  if ! pgrep -f "$1" ;
  then
    "$@"&
  fi
}

# Check if an external monitor (e.g., HDMI-1 or DP-1) is connected and set it as primary.
# You may need to adjust "HDMI-1", "DP-1", and "eDP-1" to match your actual monitor names,
# which can be found by running 'xrandr' in your terminal.
# Also, adjust the resolutions (e.g., 1920x1080, 2560x1440) to your preferred settings.

# Get xrandr output once to avoid multiple calls and transient states
CURRENT_XRANDR_OUTPUT=$(xrandr)

# Prioritize DP-1 connection if connected
if echo "$CURRENT_XRANDR_OUTPUT" | awk '/^DP-1 connected/ {found=1} END {exit !found}'; then
  xrandr --output DP-1 --primary --mode 2560x1440 --pos 0x0 --output eDP-1 --off
# Check for HDMI-1 connection if DP-1 is not connected
elif echo "$CURRENT_XRANDR_OUTPUT" | awk '/^DP-2 connected/ {found=1} END {exit !found}'; then
  xrandr --output eDP-1 --off
  xrandr --output DP-2 --primary --mode 1920x1200 --pos 0x0
elif echo "$CURRENT_XRANDR_OUTPUT" | awk '/^HDMI-1 connected/ {found=1} END {exit !found}'; then
  xrandr --output HDMI-1 --primary --mode 1920x1080 --pos 0x0 --output eDP-1 --off
else
  # If no external monitor is detected, ensure the internal display is active
  xrandr --output eDP-1 --primary --mode 1920x1080 --pos 0x0 --output HDMI-1 --off --output DP-1 --off
fi

feh --bg-fill ~/Pictures/wallpaper/pexel1.jpg
