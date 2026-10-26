# Magma 2.0 — User Manual

Magma is a multiband distortion. It splits the sound into up to six frequency bands and drives each band through its own distortion. There are 28 styles, from subtle tube and tape warmth to guitar amps, transformer saturation, bit crushing, ring modulation, octave effects and wavefolding. Each band has its own dynamics, feedback, tone, mix, level and pan.

A modulation system can move almost every control from:

- four XLFOs (waveform or 16‑step sequencer)
- two envelope generators
- two envelope followers
- two XY pads
- MIDI

Magma runs as VST3, AU, CLAP and VST2 on macOS, Windows and Linux.

**New in 2.0:** 12 new styles (28 in all), linear‑phase crossovers, a 6 dB/oct crossover slope, SUPERB quality (32× oversampling), presets for single modulation sources, a slot panel on every modulated knob, and 17 new factory presets. Projects and presets made with Magma 1.x load and sound exactly as before (see [Compatibility](#18-compatibility-with-1x)).

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
18. [Compatibility with 1.x](#18-compatibility-with-1x)

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
- **Global panel:** channel mode, quality, crossover slope and phase, global mix and output, auto‑mute.
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

The bands always add up to a flat response, so a band with Mix at 0% is completely transparent.

- **CROSSOVER SLOPE** (6, 12, 24 or 48 dB/oct) sets how sharply the bands are separated. Steeper slopes keep each band's distortion in its own range; 6 dB/oct overlaps the bands gently, which suits broad, natural‑sounding tone shaping.
- **CROSSOVER PHASE:**
  - **NATURAL** uses Linkwitz‑Riley filters with phase compensation. No extra latency. The bands add up flat in level, but the phase of the signal shifts around each crossover, as in any analogue crossover.
  - **LINEAR** uses linear‑phase filters with the same slopes. Undistorted parts of the sound pass through with their phase untouched, which matters on full mixes, parallel processing and bass with a clean low band. It adds about 50 ms of latency (reported to your DAW) and uses more CPU, and very sharp transients can show a little pre‑ringing at the 48 dB slope.

---

## 4. Distortion styles

Click the style button in the band panel to choose a style. Drive is level‑compensated: turning it up changes the character, not the volume, so you can judge the sound fairly.

The style menu shows five families side by side. Styles new in 2.0 are marked *(2.0)*.

| Family | Style | Character |
|---|---|---|
| Tube | **Gentle Tube** *(2.0)* | The lightest tube colour; a trace of even harmonics that stays clean even at high drive |
| | **Clean Tube** | Gentle, slightly asymmetric warmth; mostly even harmonics |
| | **Warm Tube** | Rounder and thicker |
| | **Hot Tube** | Two driven stages; thick, compressed, singing |
| | **Sagging Tube** *(2.0)* | The supply droops under load: loud passages compress and soften, then recover |
| | **Broken Tube** | A starved, lopsided stage with a dead zone: gated, spitting fuzz |
| Tape | **Clean Tape** | Soft symmetric saturation with a gentle high roll‑off |
| | **Warm Tape** | More drive and roll‑off, with a little tape‑like memory |
| | **Hot Tape** | Pushed into compression |
| | **Dirty Tape** *(2.0)* | Worn, over‑driven tape: uneven, smeared saturation and a duller top |
| Amp | **Clean Amp** | A guitar amp at the edge of break‑up, with a speaker cabinet |
| | **Breakup Amp** *(2.0)* | Just past clean: crunches when you dig in, cleans up when you play softly |
| | **Crunch Amp** | Two gain stages and a mid lift |
| | **Lead Amp** | Three gain stages, strong mids, resonant cabinet |
| | **Metal Amp** *(2.0)* | Very high gain, tight low end, pushed upper mids |
| Saturation | **Soft Saturation** | The smoothest curve; rounds peaks without much colour |
| | **Heavy Saturation** *(2.0)* | A dense, hard‑kneed curve between Soft Saturation and Hard Clip |
| | **Hard Clip** | Flat tops. Brutal and loud; use lightly or in parallel |
| | **Transformer** *(2.0)* | Lows saturate much earlier than highs, like an iron‑core transformer |
| FX | **Rectify** | Folds the negative half up; octave‑up, ring‑like tones |
| | **Crush** | Bit‑depth and sample‑rate reduction, from grit to 2‑bit chaos |
| | **Downsample** *(2.0)* | Sample‑rate reduction only. **Drive sets how far the rate drops** |
| | **Destroy** *(2.0)* | Extreme gain into a hard, crushing curve |
| | **Smear** | Replays tiny grains from the recent past and feeds them back. **Drive sets the amount of smearing** |
| | **Fold** | A wavefolder: bright, metallic, synthetic |
| | **Ring Mod** *(2.0)* | Multiplies the band with a sine oscillator, 30 Hz – 3.8 kHz. **Drive sets the frequency** |
| | **Octave Up** *(2.0)* | Full‑wave rectification: a strong upper octave, like an octave fuzz |
| | **Sub Octave** *(2.0)* | Adds a square‑ish tone an octave below the input. **Drive sets how much** |

For the four styles marked in bold, Drive sets an amount rather than the gain into a curve, so there is no level compensation to speak of.

The 1.x family called Experimental is now FX; the styles in it are unchanged.

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
- **QUALITY** (the oversampling of the distortion):
  - **NORMAL:** 2×. Light on CPU; fine for most material.
  - **GOOD:** 8× (called HQ in 1.x). Cleaner highs with heavy drive, at about four times the CPU.
  - **SUPERB:** 32×. The least aliasing, for mixdowns and rendering. It costs far more CPU than GOOD, especially with many bands (see the table in the README).
  - Latency is 32 (Normal), 56 (Good) or 62 (Superb) samples, plus the linear‑phase delay if that is on. It is reported to your DAW and shown at the top right.
- **CROSSOVER SLOPE:** 6, 12, 24 or 48 dB/oct.
- **CROSSOVER PHASE:** NATURAL or LINEAR (see [Bands and crossovers](#3-bands-and-crossovers)).
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

**Slot panel:** click the ring of a modulated knob, or right‑click the knob, to open a small panel listing every slot that moves it. Each row has the source (click to change it), the amount, bypass and delete, so you can adjust a knob's modulation without hunting through the slot list. Click outside the panel or press Esc to close it.

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

**Source presets.** XLFOs, envelope generators and envelope followers have a **PRESETS** menu at the top right of their editor. It loads a setting for that one source without touching the rest of the patch:

| Source | Factory presets |
|---|---|
| XLFO | Slow Sine, Quarter Pulse, Eighth Saw Down, Bar Triangle, Dotted Eighth Sine, Triplet Square, Random Drift, Sample & Hold 1/16, Trance Gate, Rising Steps, Smooth Steps |
| EG | Transient Pluck, Transient Swell, Long Fade, Note Pluck, Note Swell, Note Gate |
| EF | Fast Follow, Smooth Follow, Slow Follow, Hot Follow, Sidechain Fast, Sidechain Smooth |

**Save current…** stores your own, in an `LFO`, `Envelope` or `Follower` folder inside the preset folder (see [Presets](#12-presets)). Right‑click one of your own source presets and click again to delete it.

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

59 factory presets in seven groups, plus your own. Presets new in 2.0 are listed second in each row.

| Group | Presets |
|---|---|
| Warmth | Tape Glue, Tube Sheen, Warm Blanket, Subtle Saturation, Analog Console; *2.0:* Gentle Tube Glow, Iron Transformer, Linear Master Warmth |
| Drums | Drum Smash, Kick Weight, Snare Crack, Lo‑Fi Break, Parallel Crush, Room Pump; *2.0:* Dirty Tape Drums, Destroyed Kick |
| Bass | Bass Grit, Sub Safe Drive, Reese Growl, Fuzz Bass, Tape Bass; *2.0:* Sub Octave Bass, Linear Bass Split |
| Vocals & Keys | Vocal Presence, Radio Voice, Rhodes Bark, Piano Air, Organ Drive; *2.0:* Sagging Tube Vocal, Breakup Keys |
| Guitar | Clean Amp, Crunch, Lead, Fuzz Face, Feedback Howl; *2.0:* Metal Rhythm, Breakup Amp |
| Mix & Master | Mix Bus Warmth, Master Exciter, Mid/Side Width, Three‑Band Warmth, Loudness Fattener; *2.0:* Superb Sheen, Heavy Saturation Bus |
| Creative | Step Gate, Wobble Drive, Smear Cloud, Bit Decay, Rectifier, Fold Sweep, Envelope Fuzz, XY Morph, MIDI Drive, Crossover Dance; *2.0:* Ring Mod Sweep, Octave Fuzz, Downsampled, Gentle 6 dB Split |

- **SAVE CURRENT AS…** stores your own presets, in one of these folders:
  - macOS: `~/Library/Audio/Presets/Epsilon Audio/Magma`
  - Windows: `%APPDATA%\Epsilon Audio\Magma\Presets`
  - Linux: `~/.config/epsilon-audio/magma/presets`
- To delete one of your presets, right‑click it in the browser and click again to confirm.

**Search and favourites.** With the browser open, just type to narrow the list to presets whose name or category matches; Backspace edits, Esc clears the search and then closes the browser. Click the star at the right of a preset to add it to the FAVOURITES column, and again to remove it. Favourites are kept in `favourites.txt` in your presets folder.

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
- **Distort a full mix without smearing it:** CROSSOVER PHASE to LINEAR, three bands, drive only the top band (see Linear Master Warmth).
- **Octave bass:** one band below 150 Hz with Sub Octave, the rest clean or lightly saturated (see Sub Octave Bass).
- **Bell tones from anything:** Ring Mod on a mid band, with an XLFO slowly sweeping its Drive (see Ring Mod Sweep).

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
| A knob's slots | Click its modulation ring, or right‑click the knob |
| Source presets | PRESETS menu in the XLFO, EG or EF editor; right‑click your own to delete |
| Draw steps | Click/drag in the step display; Cmd/Ctrl‑click resets a step |
| Fine adjustment | Shift‑drag, Shift‑wheel |
| Type a value | Double‑click a knob |
| Reset | Cmd/Ctrl‑click a knob or a slot amount |
| Undo / redo | Cmd/Ctrl‑Z / Cmd/Ctrl‑Shift‑Z or Cmd/Ctrl‑Y |
| Scroll the slot list | Mouse wheel over it |
| Close popups, help, Learn; cancel typing | Esc |

---

## 16. Parameter reference

There are 420 parameters, all automatable. New parameters are only ever added at the end, so sessions and automation keep working across versions.

- **Global:**
  - Bands (1–6)
  - Crossover 1–5 (20 Hz – 20 kHz)
  - Crossover Slope
  - Channel Mode
  - High Quality (GOOD; SUPERB overrides it)
  - Auto Mute
  - Mix
  - Output Gain (±24 dB)
  - MIDI
  - Crossover Phase *(2.0, at the end of the list)*
  - Superb Quality *(2.0, at the end of the list)*
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
- **CPU is high.** SUPERB with many bands, especially with LINEAR phase, is meant for rendering: six bands at SUPERB can need more than one CPU core in real time. Use NORMAL or GOOD while mixing and switch to SUPERB for the final render (most DAWs render offline, so it does not need to keep up in real time).
- **Latency jumped.** LINEAR crossover phase adds about 50 ms. Your DAW compensates for it on playback, but it is noticeable when monitoring live input; switch to NATURAL while recording.

---

## 18. Compatibility with 1.x

- Projects, automation and presets from Magma 1.x load unchanged. The new parameters were added at the end of the list, and each one defaults to its 1.x behaviour: NATURAL phase, SUPERB off, and the slope and style lists keep their old values (6 dB/oct and the new styles were added after the old ones).
- A 1.x session renders identically in 2.0. The test suite checks this on every build by playing random 1.x settings through a frozen copy of the 1.x engine and through 2.0 and requiring the outputs to match to better than −120 dB.
- In the user interface, QUALITY's 1.x HQ is now called GOOD, and the Experimental family is now called FX.
- Going back is not supported: a 2.0 preset that uses a new style or the 6 dB slope still loads in 1.x, but those values are clamped to the last 1.x choice (Fold, 48 dB/oct) and the new phase and quality settings are ignored.
