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

XR=$(xrandr --query)

# Detect internal and external outputs by name patterns
internal=$(printf "%s\n" "$XR" | awk '/ connected/{print $1}' | awk 'BEGIN{IGNORECASE=1} /eDP|LVDS|DSI/ {print; exit}')
external=$(printf "%s\n" "$XR" | awk '/ connected/{print $1}' | awk 'BEGIN{IGNORECASE=1} /HDMI|(^|[^E])DP|DISPLAYPORT|DVI|VGA|TYPEC|USB-C/ {print; exit}')

if [ -n "$external" ]; then
  # External connected: turn off internal(s) and enable external
  # Turn off all detected internal outputs
  for out in $(printf "%s\n" "$XR" | awk '/ connected/{print $1}' | awk 'BEGIN{IGNORECASE=1} /eDP|LVDS|DSI/ {print}'); do
    xrandr --output "$out" --off 2>/dev/null || true
  done
  xrandr --output "$external" --auto --primary
else
  # No external: enable detected internal (fallback to common names)
  if [ -n "$internal" ]; then
    xrandr --output "$internal" --auto --primary
  else
    # Fallback best-effort for typical internal names
    xrandr --output eDP-1 --auto --primary 2>/dev/null || \
    xrandr --output eDP --auto --primary 2>/dev/null || \
    xrandr --output LVDS-1 --auto --primary 2>/dev/null || true
  fi
fi

feh --bg-fill ~/Pictures/wallpaper/pexel1.jpg
