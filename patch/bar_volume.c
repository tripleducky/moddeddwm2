#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>


static char volume_text[32] = "VOL";

int width_volume(Bar *bar, BarArg *a) {
    return TEXTW(volume_text);
}

int draw_volume(Bar *bar, BarArg *a) {
    drw_setscheme(drw, scheme[SchemeNorm]);
    return drw_text(drw, a->x, a->y, a->w, a->h, lrpad / 2, volume_text, 0, False);
}

int click_volume(Bar *bar, Arg *arg, BarArg *a) {
    system("/usr/bin/wpctl set-mute @DEFAULT_AUDIO_SINK@ toggle");
    return 1;
}

int scroll_volume(Bar *bar, Arg *arg, BarArg *a) {
    // arg->i: +1 for scroll up, -1 for scroll down
    char cmd[128];
    if (arg->i > 0)
        snprintf(cmd, sizeof(cmd), "/usr/bin/wpctl set-volume -l 1.0 @DEFAULT_AUDIO_SINK@ 5%%+");
    else
        snprintf(cmd, sizeof(cmd), "/usr/bin/wpctl set-volume -l 1.0 @DEFAULT_AUDIO_SINK@ 5%%-");
    system(cmd);
    return 1;
}

void update_volume(void) {
    FILE *fp = popen("/usr/bin/wpctl get-volume @DEFAULT_AUDIO_SINK@", "r");
    if (!fp) return;
    float vol = 0.0;
    int muted = 0;
    char buf[128];
    if (fgets(buf, sizeof(buf), fp)) {
        // Find the float after "Volume:"
        char *p = strstr(buf, "Volume:");
        if (p) {
            p += strlen("Volume:");
            while (*p == ' ') p++; // skip spaces
            sscanf(p, "%f", &vol);
        }
        if (strstr(buf, "[MUTED]")) muted = 1;
    }
    pclose(fp);
    snprintf(volume_text, sizeof(volume_text), "VOL: %d%%%s", (int)(vol * 100), muted ? " (M)" : "");
}