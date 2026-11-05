/* SPDX-License-Identifier: GPL-2.0-or-later */
/* CHOKE GROUPS for the Digitone mk1 and Digitone Keys (core-dn1 3.1 and later): a note on a track silences what the other
 * tracks of its group are still playing.
 *
 * Each synth track is in group A, B or none (CHOKE in the Mod Menu, for the held track; saved with the project,
 * "CHOK"). When a voice starts a note (ev_voice_on, in the render), every other voice playing a track of the same
 * group is choked. A choked voice's block (ev_render_voices: the DSP's output for it, its amp envelope in, before
 * the filter and the mix) fades to nothing over the first block, 32 samples, and is silent after, until the voice
 * starts a note again. The voice is not released early: it runs out its note and its release in silence, so the
 * voice allocator sees what it would have seen, and the voice is free again when the stock frees it. */
#include "digitone-mk1/core3.h"

typedef void (*rect_fn)(void *bmp, int x0, int y0, int x1, int y1, int colour);
typedef void (*text_fn)(void *bmp, const void *font, int x, int y, int maxlen, const char *fmt, ...);

struct save {
    int32_t group[4];                    /* per synth track: 0 none, 1 A, 2 B */
};
static struct save save;

static volatile uint32_t choked;         /* bit v: voice v is silenced */
static volatile uint32_t fading;         /* bit v: its first silent block fades */
volatile uint32_t choke_count;           /* voices choked, for the probe */
static int32_t shown, shown_track;

/* The render: voice `voice` starts a note on `track`. */
void choke_voice_on(int32_t voice, int32_t track, void *event)
{
    int32_t g;
    uint32_t c = choked & ~(1u << voice), f = fading & ~(1u << voice);
    (void)event;
    if (track >= 0 && track < 4 && (g = save.group[track]) != 0)
        for (int32_t v = 0; v < 8; v++) {
            int32_t t = fw_voice_track[v];
            if (v == voice || t == track || t < 0 || t > 3 || save.group[t] != g || (c >> v & 1))
                continue;
            c |= 1u << v;
            f |= 1u << v;
            choke_count++;
        }
    fading = f;
    choked = c;
}

/* The render, each block: the choked voices' 32 samples (Q1.31), faded the first time, then zero. */
void choke_render(int32_t *voices)
{
    uint32_t c = choked, f = fading;
    if (!c)
        return;
    for (int32_t v = 0; v < 8; v++) {
        int32_t *s = voices + 32 * v;
        if (!(c >> v & 1))
            continue;
        if (f >> v & 1)
            for (int32_t i = 0; i < 32; i++)
                s[i] = (s[i] >> 5) * (31 - i);      /* no 64-bit multiply: the mod has no libgcc */
        else
            for (int32_t i = 0; i < 32; i++)
                s[i] = 0;
    }
    fading = 0;
}

/* ---- the Mod Menu: CHOKE, for the held track ----------------------------------------------------- */
struct item {
    const char *name;
    void (*open)(void *brain, void *ev, int32_t track);
    unsigned long tag;
    const unsigned short *icon;
};

static const unsigned short choke_icon[16] = {         /* a cymbal, a hand */
    0x0000, 0x0180, 0x0ff0, 0x7ffe, 0x0000, 0x0180, 0x0180, 0x0180,
    0x1998, 0x3ffc, 0x3ffc, 0x1ff8, 0x0ff0, 0x0000, 0xffff, 0x0000
};

static const struct item groups[3] = {
    { "NO CHOKE", 0, CORE_MENU_ICON, choke_icon },
    { "GROUP A", 0, CORE_MENU_ICON, choke_icon },
    { "GROUP B", 0, CORE_MENU_ICON, choke_icon },
};
static const void *const group_list[4] = { &groups[0], &groups[1], &groups[2], 0 };

static int32_t menu_track;

static void group_pick(void *brain, void *event, int32_t track, int32_t index)
{
    (void)brain; (void)event; (void)track;
    save.group[menu_track] = index;
    shown_track = menu_track;
    shown = 30;
}

static void choke_open(void *brain, void *event, int32_t track)
{
    (void)brain; (void)event;
    menu_track = track & 3;
    core_menu_open(group_list, save.group[menu_track], group_pick);
}

const struct item choke_menu = { "CHOKE", choke_open, CORE_MENU_ICON, choke_icon };

static void choke_loaded(int32_t found)
{
    (void)found;
    for (int32_t t = 0; t < 4; t++)
        if (save.group[t] < 0 || save.group[t] > 2)
            save.group[t] = 0;
}
const struct core_projdata choke_projdata = { 0x43484f4bu /* "CHOK" */, sizeof(save), &save, choke_loaded };

void choke_tick(void *ctrl)
{
    if (shown > 0) {
        shown--;
        *((unsigned char *)ctrl + 0x20) = 1;
    }
}

void choke_draw(void *bmp, void *ctrl)
{
    static const char *const names[3] = { "-", "A", "B" };
    (void)ctrl;
    if (shown <= 0)
        return;
    ((rect_fn)fw_fillrect)(bmp, 84, 54, 127, 63, 0);
    ((text_fn)fw_textf)(bmp, (const void *)fw_font5, 87, 57, -1, "T%d CHOKE %s", (int)shown_track + 1,
                        names[save.group[shown_track]]);
    ((rect_fn)fw_fillrect)(bmp, 84, 54, 127, 63, -1);
}
