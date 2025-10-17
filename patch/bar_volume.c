#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <X11/Xlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>


static char volume_text[32] = "VOL";

int width_volume(Bar *bar, BarArg *a) {
    return TEXTW(volume_text);
}

int draw_volume(Bar *bar, BarArg *a) {
    drw_setscheme(drw, scheme[SchemeNorm]);
    return drw_text(drw, a->x, a->y, a->w, a->h, lrpad / 2, volume_text, 0, False);
}

int click_volume(Bar *bar, Arg *arg, BarArg *a) {
    {
        pid_t pid = fork();
        if (pid == 0) {
            /* child */
            execl("/usr/bin/wpctl", "wpctl", "set-mute", "@DEFAULT_AUDIO_SINK@", "toggle", (char *)NULL);
            _exit(127);
        }
    }
    /* ask dwm to refresh status immediately */
    extern void refresh_status(void);
    refresh_status();
    return 1;
}

int scroll_volume(Bar *bar, Arg *arg, BarArg *a) {
    // arg->i: +1 for scroll up, -1 for scroll down
    if (arg->i > 0) {
        pid_t pid = fork();
        if (pid == 0) {
            execl("/usr/bin/wpctl", "wpctl", "set-volume", "-l", "1.5", "@DEFAULT_AUDIO_SINK@", "5%+", (char *)NULL);
            _exit(127);
        }
    } else {
        pid_t pid = fork();
        if (pid == 0) {
            execl("/usr/bin/wpctl", "wpctl", "set-volume", "-l", "1.5", "@DEFAULT_AUDIO_SINK@", "5%-", (char *)NULL);
            _exit(127);
        }
    }
    /* ask dwm to refresh status immediately */
    extern void refresh_status(void);
    refresh_status();
    return 1;
}

void update_volume(void) {
    /* Use a single worker child so we don't spawn multiple wpctl processes
     * when updatestatus() runs frequently. We keep the read fd and pid in
     * static variables and only spawn a new child when the previous one has
     * finished. This prevents bursts of wpctl processes that can stall the
     * system if wpctl is slow. */
    static pid_t vol_pid = 0;
    static int vol_fd = -1;
    char out[2048] = "";
    ssize_t r;
    char buf[256];

    if (vol_pid == 0) {
        int pipefd[2];
        if (pipe(pipefd) == -1)
            return;
        pid_t pid = fork();
        if (pid == -1) {
            close(pipefd[0]); close(pipefd[1]);
            return;
        }
        if (pid == 0) {
            /* child */
            close(pipefd[0]);
            dup2(pipefd[1], STDOUT_FILENO);
            dup2(pipefd[1], STDERR_FILENO);
            close(pipefd[1]);
            execl("/usr/bin/wpctl", "wpctl", "get-volume", "@DEFAULT_AUDIO_SINK@", (char *)NULL);
            _exit(127);
        }
        /* parent */
        close(pipefd[1]);
        vol_pid = pid;
        vol_fd = pipefd[0];
        /* set non-blocking so we don't stall the event loop */
        int flags = fcntl(vol_fd, F_GETFL, 0);
        if (flags != -1)
            fcntl(vol_fd, F_SETFL, flags | O_NONBLOCK);
    }

    /* try to read whatever the worker has produced so far */
    while ((r = read(vol_fd, buf, sizeof(buf) - 1)) > 0) {
        buf[r] = '\0';
        strncat(out, buf, sizeof(out) - strlen(out) - 1);
    }
    if (r == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
        /* no data available right now; leave the previous displayed value */
        return;
    }
    if (r == 0) {
        /* EOF: child closed its end — read complete */
        close(vol_fd);
        vol_fd = -1;
        /* Try to reap the child without blocking; if still running, leave vol_pid set
         * and we'll reap it on a later call. */
        int status;
        pid_t res = waitpid(vol_pid, &status, WNOHANG);
        if (res == -1 || res > 0) {
            vol_pid = 0; /* reaped or error (no longer valid) */
        }
        /* if res == 0, child still running; keep vol_pid to avoid spawning overlaps */
    }
    /* if out is empty here, nothing was produced */
    if (out[0] == '\0')
        return;

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
                /* wpctl may print fractional multiplier like 1.5 for 150% when -l used */
                if (tmp > 2.0f) /* e.g. "150" (percent) */
                    vol = tmp / 100.0f;
                else if (tmp > 1.0f) /* e.g. "1.5" multiplier */
                    vol = tmp; /* keep as fraction of 1.0..2.0 */
                else
                    vol = tmp;
            }
        } else {
            /* last resort: try to parse any float from the output */
            float tmp = 0.0f;
            if (sscanf(out, "%f", &tmp) == 1) {
                if (tmp > 2.0f)
                    vol = tmp / 100.0f;
                else if (tmp > 1.0f)
                    vol = tmp;
                else
                    vol = tmp;
            }
        }
    }

    /* round to nearest percent */
    int percent = (int)(vol * 100.0f + 0.5f);
    snprintf(volume_text, sizeof(volume_text), "VOL: %d%%%s", percent, muted ? " (M)" : "");
}