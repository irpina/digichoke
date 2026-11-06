# CHOKE GROUPS, Digitone mk1

Choke groups for the Digitone mk1 and Digitone Keys: a note on a track
silences what the other tracks of its group are still playing, as a closed
hi-hat cuts an open one.

- **CHK** is knob B on TRIG's second page (press TRIG twice: "Trig
  Parameters (2/2)"): OFF, A, B, C or D. Each pattern keeps a value per synth
  track.
- **P-lock it** like any trig parameter: hold a trig in GRID REC and turn
  CHK. That trig's note plays in that group (or in none); the track's other
  notes keep the track's value.
- A note in a group (from the sequencer, the keyboard, MIDI or the
  arpeggiator) chokes every voice the group's other tracks are playing: it
  fades out over one block (32 samples, 0.7 ms) and stays silent until that
  voice starts a note again. A track's own notes never choke each other, and
  OFF is never choked.

For OS 1.43 and 1.44, on [core-dn1](https://github.com/irpina/elekloader/tree/main/mods/core-dn1) 5.1. It names no firmware address: its
1.44 port is empty, and both builds carry the same code.

## Install

Not in elekloader's shop yet. You need:
- **core-dn1 5.1,** which is not released yet: build it from elekloader's source
  (`mods/core-dn1`, its README says how) with the stock OS file. Older cores refuse this mod.
- **elekloader from source with core-dn1 5.1** (its `digitone-mk1/core3.h` declares what this
  mod calls), with Python 3.9 or newer, to build the mod and the OS.
- **This mod:** `choke-0.2.elemod` for OS 1.43, `choke-0.2-os1.44.elemod` for 1.44, from
  [this repository's releases](https://github.com/irpina/digichoke/releases) once there are any, or built
  from this folder (Build, below).
- **The stock OS file:** `Digitone_and_Digitone_Keys_OS1.44.syx` or `..._OS1.43.syx`, from
  [Elektron's Digitone downloads](https://www.elektron.se/support-downloads/digitone).
  elekloader recognises the file by its hash.

Build your OS (or in elekloader's window, run from source: **+ Install from file...** both files):

```bash
python -m elekloader.patch --stock Digitone_and_Digitone_Keys_OS1.44.syx \
    --mod core-5.1-os1.44.elemod --mod choke-0.2-os1.44.elemod \
    --out Digitone_OS1.44-choke.syx --version XXXX
```

`--version` is the 4 characters the unit will show. Flash the `.syx` with
Elektron Transfer, as for any OS update: connect the unit over USB, select
it in Transfer and **Connect**, drag the `.syx` onto **Drop files here**,
and press **YES** on the unit. Don't turn it off until the upgrade is done.

**Recovery:** elekloader never changes the bootloader, so the stock OS
file always restores the unit. Hold **FUNC** while powering on for the
startup menu, and press **TRIG 4** for OS UPGRADE. Then send the stock
`.syx` with Transfer.

## How

CHK is a lane parameter (core-dn1 5.1's lane parameters (elekloader's docs/ADAPTING.md, "Lane parameters, knobs on pages and TRIG's second page")): core keeps its values per pattern,
track and step, saves them with the pattern, and turns and shows the knob.
`ev_voice_on` runs in the render as a voice starts a note; `core_note_step`
says which step the note came from (none for the keys and MIDI), and the
group is that step's p-lock, or the track's value. The voice remembers its
group and track. When it has a group, every other voice in that group on
another track is marked choked. Each block, `ev_render_voices` gets the
DSP's eight voices after they reach the main CPU, with their amp envelopes in
and before the filters and the mix: a choked voice's 32 samples are faded the
first time and zeroed after. The voice is not released early. It runs out its
note and its release in silence, so the voice allocator sees what it would
have seen without the mod; a choke is silence, not a shorter AMP release.

## Build

```bash
python -m elekloader.sdk.build . --stock Digitone_and_Digitone_Keys_OS1.43.syx   # out/choke-0.2.elemod
python -m elekloader.sdk.build . --stock Digitone_and_Digitone_Keys_OS1.44.syx   # out/choke-0.2-os1.44.elemod
python -m elekloader.lint out/choke-0.2.elemod --stock Digitone_and_Digitone_Keys_OS1.43.syx --with core-5.1.elemod
```

## What was checked

In digikit's emulator (digiemu, headless `Session`, audio recorded), with
core-dn1 5.1 and a probe mod, on 1.43 and on 1.44, on the factory pattern
(T1 and T4 play):

- TRIG twice showed "Trig Parameters (2/2)" with CHK on knob B; turning it
  changed T1's value (the label showed the value, C).
- T1 and T4 in A: 4 voices choked in 3 s of play, the output's RMS 1618
  against 1775 with no group; T4 in B: none choked (1784).
- T1 in A, T4 in B, T4's step 1 p-locked to A (hold trig 1 in GRID REC, turn
  CHK: the knob drew the lock inverted): T4's step-1 voice played in A and
  T1's in A choked each other (2 voices, RMS 1685), T4's other steps stayed
  in B. PLAY's first step, which the sequencer builds ahead and copies, got
  its step too.
- With the project saved: FUNC+YES saved the pattern with its CHK values and
  lock, they were changed, and FUNC+NO brought the saved ones back.

The values were set by poking core's lane store or by the knob; the emulator's
knob takes several detents to move a step, so exact values came from pokes.
`lint` says it patches no site and links with core-dn1 5.1 on 1.43 and
1.44, and it links beside digitables 1.4, machines 0.2 and the other quick-win
mods.

Not checked: hearing the choke (which notes were cut, and whether the fade
clicks), a project loaded on the stock OS, the Digitone Keys, and anything on
hardware.

## Licence

GPL-2.0-or-later, as elekloader's core it builds on (LICENSE).
