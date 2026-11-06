/* SPDX-License-Identifier: GPL-2.0-or-later */
/* CHOKE GROUPS for the Digitone mk1 and Digitone Keys (core-dn1 5.1): a note on a track silences what the other
 * tracks of its group are still playing.
 *
 * CHOKE is a knob on TRIG's second page (core's, TRIG pressed twice), knob B: OFF, A, B, C or D. It is a core lane
 * parameter: each pattern keeps a value per synth track, and a step may p-lock it (hold the trig in GRID REC and
 * turn), so one trig can choke while the track's others ring, or join another group. A note takes the value of the
 * step it came from (core_note_step), and a note played from the keys or by MIDI the track's value.
 *
 * When a voice starts a note (ev_voice_on, in the render) in a group, every other voice playing a note of another
 * track in that group is choked: its block (ev_render_voices: the DSP's output for it, its amp envelope in, before
 * the filter and the mix) fades to nothing over the first block, 32 samples, and is silent after, until the voice
 * starts a note again. The voice is not released early: it runs out its note and its release in silence, so the
 * voice allocator sees what it would have seen, and the voice is free again when the stock frees it. */
#include "digitone-mk1/core3.h"

#define CHOKE_ID        184             /* the parameter, a lane (param:184) */
#define GROUPS          4               /* A-D */

static volatile uint32_t choked;        /* bit v: voice v is silenced */
static volatile uint32_t fading;        /* bit v: its first silent block fades */
static int8_t voice_group[8];           /* the group each voice's note started in, 0 none */
static int8_t voice_track[8];
volatile uint32_t choke_count;          /* voices choked, for the probe */

/* The group a note plays in: its step's (a p-lock, else the track's value) for a note the sequencer built,
 * the track's value for one from the keys or MIDI. */
static int32_t group_of(int32_t track, void *event)
{
    int32_t step = core_note_step(event);
    return step >= 0 ? core_lane_value(CHOKE_ID, track, step) : core_lane_track(CHOKE_ID, track);
}

/* The render: voice `voice` starts a note on `track`. */
void choke_voice_on(int32_t voice, int32_t track, void *event)
{
    int32_t g;
    uint32_t c = choked & ~(1u << voice), f = fading & ~(1u << voice);
    if (voice < 0 || voice > 7)
        return;
    g = track >= 0 && track < 4 ? group_of(track, event) : 0;
    if (g < 0 || g > GROUPS)
        g = 0;
    voice_group[voice] = (int8_t)g;
    voice_track[voice] = (int8_t)track;
    if (g)
        for (int32_t v = 0; v < 8; v++) {
            if (v == voice || voice_group[v] != g || voice_track[v] == track || (c >> v & 1))
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

/* A voice set free (its note ran out) leaves its group. */
void choke_voice_free(int32_t voice)
{
    if (voice >= 0 && voice < 8)
        voice_group[voice] = 0;
}

/* ---- the knob ------------------------------------------------------------------------------------ */
static void choke_format(int32_t value, char *buf)
{
    int32_t v = value >> 8;
    if (v <= 0 || v > GROUPS) {
        buf[0] = 'O'; buf[1] = 'F'; buf[2] = 'F'; buf[3] = 0;
    } else {
        buf[0] = (char)('A' + v - 1);
        buf[1] = 0;
    }
}

const struct core_param choke_param = {
    CHOKE_ID, 16, CORE_SLOT_LANE, 0, GROUPS << 8, 0, 0, -1, -1, -1, 0,
    "Choke Group", (const char *)fw_str_empty, "CHK", choke_format, (const char *)fw_str_empty, 0, 0
};
const struct core_knob choke_knob = { CORE_PAGE_TRIG2, 1, CHOKE_ID };
