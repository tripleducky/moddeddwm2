#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
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
    /* trigger dwm to re-read status by generating a PropertyNotify on root */
    {
        Display *d = XOpenDisplay(NULL);
        if (d) {
            Window r = DefaultRootWindow(d);
            XStoreName(d, r, "");
            XSync(d, False);
            XCloseDisplay(d);
        }
    }
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
    /* trigger dwm to re-read status */
    {
        Display *d = XOpenDisplay(NULL);
        if (d) {
            Window r = DefaultRootWindow(d);
            XStoreName(d, r, "");
            XSync(d, False);
            XCloseDisplay(d);
        }
    }
    return 1;
}

void update_volume(void) {
    /* Capture stdout and stderr in case wpctl writes to stderr in some setups */
    FILE *fp = popen("/usr/bin/wpctl get-volume @DEFAULT_AUDIO_SINK@ 2>&1", "r");
    if (!fp) return;
    char line[256];
    /* accumulate output (small, so fixed buffer is fine) */
    char out[2048] = "";
    while (fgets(line, sizeof(line), fp)) {
        strncat(out, line, sizeof(out) - strlen(out) - 1);
    }
    pclose(fp);

    float vol = 0.0f;
    int muted = 0;

    if (strstr(out, "[MUTED]")) muted = 1;

    /* Try to find a percentage like "34%" first */
    char *pct = strchr(out, '%');
    if (pct) {
        /* scan backwards to find the number before '%' */
        char *q = pct - 1;
        /* skip spaces */
        while (q > out && isspace((unsigned char)*q)) q--;
        /* now q should be on the last digit of the number */
        /* find the start of the number */
        char *start = q;
        while (start > out && (isdigit((unsigned char)*(start-1)) || *(start-1) == '.')) start--;
        size_t len = q - start + 1;
        if (len > 0 && len < 64) {
            char numbuf[64] = {0};
            memcpy(numbuf, start, len);
            numbuf[len] = '\0';
            float perc = 0.0f;
            if (sscanf(numbuf, "%f", &perc) == 1) {
                /* if value looks like a whole percent (e.g. 34), convert to fraction */
                if (perc > 1.0f) vol = perc / 100.0f;
                else vol = perc;
            }
        }
    } else {
        /* fallback: search for "Volume:" then parse a float after it */
        char *p = strstr(out, "Volume:");
        if (p) {
            p += strlen("Volume:");
            while (*p && isspace((unsigned char)*p)) p++;
            float tmp = 0.0f;
            if (sscanf(p, "%f", &tmp) == 1) {
                if (tmp > 1.0f) vol = tmp / 100.0f;
                else vol = tmp;
            }
        } else {
            /* last resort: try to parse any float from the output */
            float tmp = 0.0f;
            if (sscanf(out, "%f", &tmp) == 1) {
                if (tmp > 1.0f) vol = tmp / 100.0f;
                else vol = tmp;
            }
        }
    }

    /* round to nearest percent */
    int percent = (int)(vol * 100.0f + 0.5f);
    snprintf(volume_text, sizeof(volume_text), "VOL: %d%%%s", percent, muted ? " (M)" : "");
}