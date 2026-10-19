# Magma 1.0 — User Manual

Magma is a multiband distortion. It splits the sound into up to six frequency bands and drives each band through its own distortion. There are 16 styles, from subtle tube and tape warmth to guitar amps, hard clipping, bit crushing, grain smearing and wavefolding. Each band has its own dynamics, feedback, tone, mix, level and pan.

A modulation system can move almost every control from:

- four XLFOs (waveform or 16‑step sequencer)
- two envelope generators
- two envelope followers
- two XY pads
- MIDI

Magma runs as VST3, AU, CLAP and VST2 on macOS, Windows and Linux.

**Contents**

1. [Installing](#1-installing)
2. [A quick tour](#2-a-quick-tour)
3. [Bands and crossovers](#3-bands-and-crossovers)
4. [Distortion styles](#4-distortion-styles)
5. [Band controls](#5-band-controls)
6. [Global controls](#6-global-controls)
7. [Modulation](#7-modulation)
8. [Modulation sources](#8-modulation-sources)
9. [Knobs](#9-knobs)
10. [A/B, undo and redo](#10-ab-undo-and-redo)
11. [MIDI](#11-midi)
12. [Presets](#12-presets)
13. [Help and hover tips](#13-help-and-hover-tips)
14. [Recipes](#14-recipes)
15. [Keyboard and mouse reference](#15-keyboard-and-mouse-reference)
16. [Parameter reference](#16-parameter-reference)
17. [Troubleshooting](#17-troubleshooting)

---

## 1. Installing

Download the zip for your system from the project's **Releases** page and copy the plugin files:

| System | Copy | To |
|---|---|---|
| macOS | `Magma.vst3`, `Magma.component` (AU), `Magma.clap` | `~/Library/Audio/Plug-Ins/VST3`, `.../Components`, `.../CLAP` |
| Windows | the `Magma.vst3` folder, `Magma.clap` | `C:\Program Files\Common Files\VST3\`, `...\CLAP\` |
| Linux | `Magma.vst3`, `Magma.clap` | `~/.vst3/`, `~/.clap/` |

**macOS:** the builds are signed ad hoc but not notarized. If macOS blocks them, open Terminal and run:

```
xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/*/Magma*
```

Rescan plugins in your DAW. Magma appears under **Epsilon Audio** as a distortion effect.

---

## 2. A quick tour

![Magma](../screenshot.png)

- **Header:**
  - preset browser
  - A/B and COPY
  - UNDO/REDO
  - LEARN (MIDI Learn) and MIDI on/off
  - HELP
  - the current latency, at the far right
- **Band display:**
  - the input spectrum (faint) and the output spectrum (bright)
  - each band as a coloured region with its style name
  - a drive line you can drag up and down
  - the crossovers between bands, which you can drag left and right
- **Band panel:** the controls of the selected band.
- **Global panel:** channel mode, quality, crossover slope, auto‑mute, global mix and output.
- **Modulation sources** (bottom left): tabs for each source, its editor, and its chip(s) to drag onto knobs.
- **Modulation slots** (bottom right): every active modulation connection.

---

## 3. Bands and crossovers

Magma starts with one band covering the whole spectrum.

- **Add a band:** double‑click the display. The band under the pointer is split at that frequency, and the new band starts as a copy of the old one. Up to six bands.
- **Select a band:** click it. Its controls appear in the band panel, and its region is highlighted.
- **Change a band's drive:** drag its horizontal line up or down, or use the mouse wheel over the band.
- **Move a crossover:** drag the handle on the vertical line between two bands.
- **Remove a band:**
  - right‑click a crossover to merge the two bands on either side of it, or
  - click **REMOVE** in the band panel.
  Modulation that targeted a removed band is deleted. Modulation of the other bands moves with them.

The crossovers are Linkwitz‑Riley filters with phase compensation, so the bands always add up to a flat response. A band with Mix at 0% is completely transparent. **CROSSOVER SLOPE** (12, 24 or 48 dB/oct) sets how sharply the bands are separated: steeper slopes keep each band's distortion in its own range.

---

## 4. Distortion styles

Click the style button in the band panel to choose a style. Drive is level‑compensated: turning it up changes the character, not the volume, so you can judge the sound fairly.

| Family | Style | Character |
|---|---|---|
| Tube | **Clean Tube** | Gentle, slightly asymmetric warmth; mostly even harmonics |
| | **Warm Tube** | Rounder and thicker |
| | **Hot Tube** | Two driven stages; thick, compressed, singing |
| | **Broken Tube** | A starved, lopsided stage with a dead zone: gated, spitting fuzz |
| Tape | **Clean Tape** | Soft symmetric saturation with a gentle high roll‑off |
| | **Warm Tape** | More drive and roll‑off, with a little tape‑like memory |
| | **Hot Tape** | Pushed into compression |
| Amp | **Clean Amp** | A guitar amp at the edge of break‑up, with a speaker cabinet |
| | **Crunch Amp** | Two gain stages and a mid lift |
| | **Lead Amp** | Three gain stages, strong mids, resonant cabinet |
| Saturation | **Soft Saturation** | The smoothest curve; rounds peaks without much colour |
| | **Hard Clip** | Flat tops. Brutal and loud; use lightly or in parallel |
| Experimental | **Rectify** | Folds the negative half up; octave‑up, ring‑like tones |
| | **Crush** | Bit‑depth and sample‑rate reduction, from grit to 2‑bit chaos |
| | **Smear** | Replays tiny grains from the recent past and feeds them back. Drive sets the amount of smearing |
| | **Fold** | A wavefolder: bright, metallic, synthetic |

---

## 5. Band controls

| Control | What it does |
|---|---|
| **DRIVE** | How hard the band hits the distortion, level‑compensated |
| **DYNAMICS** | Right of centre compresses the band before the distortion: denser, more sustain. Left of centre expands it: more punch, quieter tails |
| **FEEDBACK** | Feeds the band's output back into its input through a resonant filter. Small amounts add a ringing tone; large amounts howl |
| **FB FREQ** | The frequency the feedback rings at |
| **MIX** | Dry/wet for this band only (parallel distortion) |
| **LEVEL** | The band's output level, ±24 dB |
| **PAN** | Moves the band left or right (in mid/side mode, towards mid or side) |
| **BASS / MID / TREBLE** | Tone after the distortion: low shelf at 120 Hz, bell at 1 kHz, high shelf at 4.5 kHz, ±12 dB |
| **PRESENCE** | Positive adds air above 10 kHz. Negative rolls the top off like a speaker cabinet, down to 1.5 kHz at full |
| **MUTE / SOLO** | Silence a band, or hear only the soloed bands |

---

## 6. Global controls

- **CHANNELS:**
  - **STEREO** processes left and right.
  - **MID/SIDE** processes the centre (mid) and the stereo difference (side) instead. Pan then moves a band between mid and side; for example, distort only the sides for width.
- **QUALITY:**
  - **NORMAL** runs at 2× oversampling.
  - **HQ** runs at 8×, which gives cleaner highs with heavy drive at about four times the CPU.
  - Latency is 32 samples (Normal) or 56 samples (HQ). It is reported to your DAW.
- **CROSSOVER SLOPE:** 12, 24 or 48 dB/oct.
- **AUTO MUTE:** stops feedback from ringing on once the input has been silent for a quarter of a second.
- **MIX:** global dry/wet. The dry signal is delayed to stay exactly in time with the processed one, so there is no comb filtering at any setting.
- **OUTPUT:** final level.

---

## 7. Modulation

1. Choose a source in the tabs at the bottom left.
2. Drag its **chip** onto any knob. While you drag, valid targets are circled and the status line names the destination.
3. A **slot** is created in the list at the bottom right. Its row shows:
   - **Source:** click to change it.
   - **Destination:** click to choose from every modulatable parameter.
   - **Amount:** a bipolar mini knob. Turn it left to invert; Cmd/Ctrl‑click resets it.
   - **Bypass:** a dot that switches the slot off without deleting it.
   - **Delete:** the ×.

Up to **50** slots. A slot's amount can itself be modulated: drop a chip on a slot's amount knob. For example, the mod wheel can control how much an LFO moves the drive.

Modulated knobs get a coloured **ring** showing the modulation range, and a white **dot** showing where the modulation has taken the knob right now. In the band display, a dot shows each band's live drive, and a line shows each modulated crossover.

Modulation amounts are relative to each control's full range. For example, ±100% on a band level moves it across the whole ±24 dB.

---

## 8. Modulation sources

| Source | Controls | Notes |
|---|---|---|
| **XLFO 1–4** | MODE (Wave/Steps), WAVE, RATE or tempo DIVISION, BALANCE, GLIDE, STEPS | Wave mode: sine, triangle, saw up/down, square, smooth random, sample & hold. Steps mode: a 16‑step sequencer you draw in the display. Sync locks to the host tempo and song position (straight, dotted, triplet) |
| **EG 1–2** | TRIGGER (MIDI/Audio), ATTACK, DECAY, SUSTAIN, RELEASE, THRESHOLD | MIDI: starts with each note. Audio: starts when the input rises above the threshold |
| **EF 1–2** | LISTENS TO (Input/Sidechain), ATTACK, RELEASE, GAIN | Follows the input level, or the level of the sidechain input (route another track to Magma's sidechain in your DAW) |
| **XY 1–2** | the pad | Each pad gives two sources, X and Y. Drag the dot, automate it, or map it with MIDI Learn |
| **MIDI** | — | Velocity, Key (last note, centred on middle C), Mod Wheel, Pitch Bend, Aftertouch (channel or poly) |

A small dot on a tab means that source is in use. Each chip shows the source's current value as a thin bar.

---

## 9. Knobs

- **Drag** up or down; **Shift** for fine adjustment. **Grab the rim** of a large knob to turn it in a circle.
- **Mouse wheel** turns a knob.
- **Double‑click** to type a value, for example `1.5k`, `-6` or `250ms`.
- **Cmd/Ctrl‑click** resets to the default.

---

## 10. A/B, undo and redo

- **A / B** hold two complete settings. Click the other letter to switch; the first time, B starts as a copy of A.
- **COPY** copies the current setting into the other slot.
- Every edit is an undo step, including adding and removing bands, modulation changes, preset loads and A/B switches.
  - Undo: Cmd/Ctrl‑Z
  - Redo: Cmd/Ctrl‑Shift‑Z or Cmd/Ctrl‑Y

---

## 11. MIDI

- **MIDI Learn:**
  1. Click **LEARN**.
  2. Click a control.
  3. Move a knob or fader on your controller.
  4. Click **LEARN** again (or press Esc) to finish.

  While learning, right‑click a control to unassign it. Assignments are saved with your project.
- **Notes** feed the Velocity and Key sources and can trigger the envelope generators. CC1 drives the Mod Wheel source unless you have learned CC1 for something else.
- The **MIDI** button switches all MIDI handling off, for hosts that send MIDI to every effect.

---

## 12. Presets

42 factory presets in seven groups, plus your own:

| Group | Presets |
|---|---|
| Warmth | Tape Glue, Tube Sheen, Warm Blanket, Subtle Saturation, Analog Console |
| Drums | Drum Smash, Kick Weight, Snare Crack, Lo‑Fi Break, Parallel Crush, Room Pump |
| Bass | Bass Grit, Sub Safe Drive, Reese Growl, Fuzz Bass, Tape Bass |
| Vocals & Keys | Vocal Presence, Radio Voice, Rhodes Bark, Piano Air, Organ Drive |
| Guitar | Clean Amp, Crunch, Lead, Fuzz Face, Feedback Howl |
| Mix & Master | Mix Bus Warmth, Master Exciter, Mid/Side Width, Three‑Band Warmth, Loudness Fattener |
| Creative | Step Gate, Wobble Drive, Smear Cloud, Bit Decay, Rectifier, Fold Sweep, Envelope Fuzz, XY Morph, MIDI Drive, Crossover Dance |

- **SAVE CURRENT AS…** stores your own presets, in one of these folders:
  - macOS: `~/Library/Audio/Presets/Epsilon Audio/Magma`
  - Windows: `%APPDATA%\Epsilon Audio\Magma\Presets`
  - Linux: `~/.config/epsilon-audio/magma/presets`
- To delete one of your presets, right‑click it in the browser and click again to confirm.

---

## 13. Help and hover tips

- Rest the pointer on any control, chip, slot, crossover, band or tab to see what it does and its current value.
- **HELP** opens the in‑plugin pages. The switch at the bottom of the help window turns tips on or off.
- **FULL MANUAL (WEB)** opens this manual.

---

## 14. Recipes

- **Warm up a mix bus:** Tape Glue, or one band of Clean Tape at 20–30% drive.
- **Heavy bass that stays tight:** two bands at about 100 Hz. Leave the low band clean (Mix 0%) and drive the top band with Crunch Amp or Hot Tape. That's the Sub Safe Drive preset.
- **Parallel drum crush:** Hard Clip or Hot Tape at high drive, band Mix around 30–40%, Dynamics to the right.
- **Wider mix:** MID/SIDE mode, distort only the side band, pan toward side.
- **Rhythmic distortion:** an XLFO in Steps mode, synced to 1/8 or 1/16, on Drive or Level (see Step Gate).
- **Auto‑wah fuzz:** EF 1 on Drive and Presence (see Envelope Fuzz).
- **Performance macro:** put XY 1 X and Y on several controls at once and play the pad (see XY Morph).

---

## 15. Keyboard and mouse reference

| Action | How |
|---|---|
| Add a band | Double‑click the display |
| Select a band | Click it |
| Band drive | Drag the band's line up/down, or wheel over the band |
| Move a crossover | Drag its handle |
| Remove a band | Right‑click a crossover, or REMOVE |
| Modulate | Drag a source chip onto a knob (or a slot's amount) |
| Draw steps | Click/drag in the step display; Cmd/Ctrl‑click resets a step |
| Fine adjustment | Shift‑drag, Shift‑wheel |
| Type a value | Double‑click a knob |
| Reset | Cmd/Ctrl‑click a knob or a slot amount |
| Undo / redo | Cmd/Ctrl‑Z / Cmd/Ctrl‑Shift‑Z or Cmd/Ctrl‑Y |
| Scroll the slot list | Mouse wheel over it |
| Close popups, help, Learn; cancel typing | Esc |

---

## 16. Parameter reference

There are 418 parameters, all automatable. New parameters are only ever added at the end, so sessions and automation keep working across versions.

- **Global:**
  - Bands (1–6)
  - Crossover 1–5 (20 Hz – 20 kHz)
  - Crossover Slope
  - Channel Mode
  - High Quality
  - Auto Mute
  - Mix
  - Output Gain (±24 dB)
  - MIDI
- **Band 1–6** (14 each):
  - Style
  - Drive
  - Dynamics (±100%)
  - Feedback
  - Feedback Freq
  - Bass, Mid, Treble, Presence (±12 dB)
  - Mix
  - Level (±24 dB)
  - Pan
  - Mute, Solo
- **XLFO 1–4** (24 each):
  - Mode, Wave, Rate (0.01–50 Hz), Sync, Division, Balance, Glide
  - Steps (1–16)
  - Step 1–16 (±100%)
- **EG 1–2:** Attack, Decay (1 ms – 5 s), Sustain, Release (1 ms – 10 s), Trigger, Threshold (−60 – 0 dB).
- **EF 1–2:** Attack (0.5 ms – 1 s), Release (5 ms – 5 s), Gain (±24 dB), Source.
- **XY 1–2:** X, Y.
- **Slot 1–50:** Source, Destination, Amount (±100%), Bypass.

---

## 17. Troubleshooting

- **Magma doesn't appear in my DAW.** Check the install folders above and rescan plugins. On macOS, run the `xattr` command from section 1.
- **A ringing tone keeps going.** Feedback is high and Auto Mute is off. Lower Feedback, or turn Auto Mute on.
- **The sidechain follower doesn't move.** Route a track to Magma's sidechain input in your DAW, and set the EF's LISTENS TO to SIDECHAIN.
- **Synced LFOs don't follow the song.** Sync follows the host's song position while the transport is playing. When stopped, the LFOs run at the host tempo.
- **CPU is high.** HQ mode with six bands is the most expensive setting. Use NORMAL while mixing and switch to HQ for the final render.
