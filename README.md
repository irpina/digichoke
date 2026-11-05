# CHOKE GROUPS, Digitone mk1

Choke groups for the Digitone mk1 and Digitone Keys: a note on a track
silences what the other tracks of its group are still playing, as a closed
hi-hat cuts an open one.

- **CHOKE** in the Mod Menu (hold a track key) puts the held track in
  GROUP A, GROUP B or NO CHOKE. The groups are saved with the project, and
  the corner shows T2 CHOKE A for a second.
- A note on a track of a group (from the sequencer, the keyboard, MIDI or
  the arpeggiator) chokes every voice the group's other tracks are playing:
  it fades out over one block (32 samples, 0.7 ms) and stays silent until
  that voice starts a note again.
- A track's own notes never choke each other, and tracks with no group are
  never choked.

For OS 1.43 and 1.44, on [core-dn1](https://github.com/irpina/elekloader/tree/main/mods/core-dn1) 3.1 or later (4.x, 5.x):
it needs only 3.1's voice start (`ev_voice_on`), the voices' blocks
(`ev_render_voices`) and `fw_voice_track`. It names no firmware address: its
1.44 port is empty, and both builds carry the same code.

## Install

Not in elekloader's shop yet. You need:
- **core-dn1 3.1 or newer:** `core-dn1-4.1.elemod` (or `-os1.44`) from elekloader's
  [core-dn1-v4.1](https://github.com/irpina/elekloader/releases/tag/core-dn1-v4.1) pre-release does, as do 4.2, 5.0 and 5.0.1.
- **elekloader** with Python 3.9 or newer (see [its README](https://github.com/irpina/elekloader/#install)), to build the mod
  and the OS.
- **This mod:** `choke-0.1.elemod` for OS 1.43, `choke-0.1-os1.44.elemod` for 1.44, from
  [this repository's releases](https://github.com/irpina/digichoke/releases) once there are any, or built
  from this folder (Build, below).
- **The stock OS file:** `Digitone_and_Digitone_Keys_OS1.44.syx` or `..._OS1.43.syx`, from
  [Elektron's Digitone downloads](https://www.elektron.se/support-downloads/digitone).
  elekloader recognises the file by its hash.

Build your OS (or in elekloader's window, run from source: **+ Install from file...** both files):

```bash
python -m elekloader.patch --stock Digitone_and_Digitone_Keys_OS1.44.syx \
    --mod core-dn1-4.1-os1.44.elemod --mod choke-0.1-os1.44.elemod \
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

`ev_voice_on` runs in the render as a voice starts a note, with the voice's
track. When that track has a group, every other voice whose track
(`fw_voice_track`) is in the same group is marked choked. Each block,
`ev_render_voices` gets the DSP's eight voices after they reach the main CPU,
with their amp envelopes in and before the filters and the mix: a choked
voice's 32 samples are faded the first time and zeroed after. The voice is
not released early. It runs out its note and its release in silence, so the
voice allocator sees what it would have seen without the mod; a choke is
silence, not a shorter AMP release. (Releasing a voice early would need a
note-off into the render, which no core export gives yet: setting
`fw_voice_len` is the timeline's countdown, and what it does to a voice with
no length was not checked.)

## Build

```bash
python -m elekloader.sdk.build . --stock Digitone_and_Digitone_Keys_OS1.43.syx   # out/choke-0.1.elemod
python -m elekloader.sdk.build . --stock Digitone_and_Digitone_Keys_OS1.44.syx   # out/choke-0.1-os1.44.elemod
python -m elekloader.lint out/choke-0.1.elemod --stock Digitone_and_Digitone_Keys_OS1.43.syx --with core-5.0.1.elemod
```

## What was checked

In digikit's emulator (digiemu, headless `Session`, audio recorded), with
core-dn1 5.0.1 and a probe mod, on 1.43 and on 1.44: CHOKE > GROUP A for
each of T1-T4 from the Mod Menu (the corner showed T1 CHOKE A), then the
factory pattern played for 3 s. 11 voices were choked, and the output's RMS
over those 3 s was 1627 against 1768 with no group set (both OS alike).

`lint` says it patches no site and links, with the same code on both OS: with core-dn1 5.0.1 and 5.0
on 1.43, with 5.0.1 on 1.44, and with the released core-dn1 4.1 (the core-dn1-v4.1 pre-release's
files) on 1.43 and 1.44.

Not checked: hearing the choke (which notes were cut, and whether the fade
clicks), running it on core 4.1 in the emulator, the Digitone Keys, and anything
on hardware.

## Licence

GPL-2.0-or-later, as elekloader's core it builds on (LICENSE).
