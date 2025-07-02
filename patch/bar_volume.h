#pragma once

int width_volume(Bar *bar, BarArg *a);
int draw_volume(Bar *bar, BarArg *a);
int click_volume(Bar *bar, Arg *arg, BarArg *a);
int scroll_volume(Bar *bar, Arg *arg, BarArg *a);
void update_volume(void);