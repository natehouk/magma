# Magma

**A multiband distortion plugin with a drag-and-drop modulation system, for macOS, Windows and Linux (VST3 · AU · CLAP · VST2).**

Part of **[Epsilon Audio](https://github.com/natehouk/epsilon-plugins)**, a family of free, open-source plugins: Monolith, Caldera, Obsidian, Strata, Basalt, Gneiss, Mica, Flint, Granite, Magma, Schist, Slate, Talc, Geode and Malachite.

![Magma](screenshot.png)

Magma is a clean-room design with the feature set of FabFilter Saturn 2 (2017), the second version of FabFilter's multiband distortion. It was written from scratch, working only from FabFilter's public product pages, release announcements and reviews. No FabFilter code, binaries, presets, artwork or trademarks were used.

It is part of a family of plugins:
- [Monolith](https://github.com/natehouk/monolith) (synth)
- [Caldera](https://github.com/natehouk/caldera) (filter)
- [Obsidian](https://github.com/natehouk/obsidian) (polysynth)
- [Strata](https://github.com/natehouk/strata) (tape delay)
- [Basalt](https://github.com/natehouk/basalt) (live dual filter)
- [Gneiss](https://github.com/natehouk/gneiss) (compressor)
- [Mica](https://github.com/natehouk/mica) (equalizer)
- [Flint](https://github.com/natehouk/flint) (small filter)
- [Granite](https://github.com/natehouk/granite) (limiter)
- **Magma**, molten rock

## Download

Get the latest build from [**Releases**](../../releases):

| Platform | File | Contains |
|---|---|---|
| macOS 10.13+ (Apple Silicon + Intel) | `Magma-macos-universal.zip` | VST3, AU, CLAP, manual |
| Windows x64 | `Magma-windows-x64.zip` | VST3, CLAP, VST2, manual |
| Linux x64 | `Magma-linux-x64.zip` | VST3, CLAP, VST2, manual |

### Install

| Platform | Copy | To |
|---|---|---|
| macOS | `Magma.vst3` / `Magma.component` / `Magma.clap` | `~/Library/Audio/Plug-Ins/VST3` / `Components` / `CLAP` |
| Windows | `Magma.vst3` (folder) / `Magma.clap` | `C:\Program Files\Common Files\VST3\` / `CLAP\` |
| Linux | `Magma.vst3` / `Magma.clap` | `~/.vst3/` / `~/.clap/` |

**macOS:** the plugins are ad-hoc signed, not notarized. After copying, run:

```
xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/*/Magma*
```

## Features

**Bands**
- **Up to six bands.**
  - Double-click the display to split a band.
  - Drag crossovers to move them.
  - Right-click a crossover to merge two bands.
- **Crossovers at 6, 12, 24 or 48 dB/oct**, in two flavours:
  - **Natural phase:** Linkwitz-Riley with phase compensation and no latency. Six bands with no drive sum within 0.02 dB.
  - **Linear phase:** FIR crossovers with the same slopes that leave the phase untouched (about 50 ms of latency). Six bands with no drive rebuild the input to better than −60 dB.
- **Interactive display** shows:
  - input and output spectrum
  - band regions with their styles
  - drive lines you can drag
  - live modulation markers
  - band activity

**28 distortion styles**

| Family | Styles |
|---|---|
| Tube | Gentle, Clean, Warm, Hot, Sagging (supply sag), Broken |
| Tape | Clean, Warm, Hot, Dirty |
| Amp | Clean, Breakup, Crunch, Lead, Metal (with speaker cabinets) |
| Saturation | Soft, Heavy, Hard Clip, Transformer (lows saturate first) |
| FX | Rectify, Crush (bit/sample-rate), Downsample, Destroy, Smear (grain feedback), Fold (wavefolder), Ring Mod, Octave Up, Sub Octave |

Drive is **level-compensated**: every style stays within about 2 dB of the input level at any drive, so you hear character, not volume.

**Per band**
- Drive and dynamics (compress or expand)
- Feedback with its frequency
- Bass, mid, treble and presence after the distortion (negative presence acts as a cabinet roll-off)
- Mix, level and pan
- Mute and solo

**Global**
- Stereo or mid/side
- Normal (2×), Good (8×) or Superb (32×) oversampling
- Auto-mute for feedback
- Global mix, time-aligned with the dry signal
- Output gain

**Modulation**
- **50 slots, drag and drop:** drag a source's chip onto any knob.
  - Modulated knobs show their range and a live position dot. Click the ring (or right-click the knob) for a panel with every slot that moves it.
  - A slot's amount can be modulated by another slot.
- **Sources:**
  - 4 **XLFOs**: 7 waveforms or a 16-step sequencer with glide, free or tempo-synced (straight, dotted, triplet)
  - 2 **envelope generators**: MIDI- or audio-triggered
  - 2 **envelope followers**: input or sidechain
  - 2 **XY pads**: X and Y each a source
  - **MIDI**: velocity, key, mod wheel, pitch bend, aftertouch
  - **Presets per source**: 23 factory LFO shapes and sequences, envelopes and followers, plus your own
- **140 destinations:**
  - every continuous band control
  - crossovers
  - global mix and output
  - LFO rates and balances
  - envelope stages
  - slot amounts

**Workflow**
- A/B with copy
- Undo/redo for every edit, including adding and removing bands
- MIDI Learn
- Smoothed, sample-accurate automation (420 parameters)
- Resizable interface
- **59 factory presets** in seven categories, plus user presets
- **Hover tips** everywhere, a **HELP window**, and a **[full manual](docs/MANUAL.md)** (also in every release zip as `Magma-Manual.html`)

### Compared with FabFilter Saturn 2

| Saturn 2 | Magma |
|---|---|
| Multiband processing, up to six bands | ✓ |
| 28 distortion styles in five families (tube, tape, amp, saturation, FX) | ✓ 28 original styles in the same five families |
| Per-band drive, mix, feedback, dynamics, tone and level | ✓ plus pan, mute and solo |
| Linear-phase crossover option | ✓ |
| Crossover slopes from 6 to 48 dB/oct | ✓ 6, 12, 24, 48 dB/oct |
| Normal, Good and Superb quality (oversampling) | ✓ 2×, 8×, 32× |
| Mid/side processing | ✓ |
| XLFOs (16-step), XY controllers, envelope generators, envelope followers, MIDI sources | ✓ |
| Presets for individual modulation sources | ✓ XLFO, EG and EF presets, factory and user |
| Drag-and-drop modulation, 50-slot matrix, source visualisation | ✓ |
| Floating slot panel on modulated knobs | ✓ |
| Interactive multiband display | ✓ |
| MIDI Learn, undo/redo, A/B, parameter interpolation, sample-accurate automation | ✓ |
| Large factory preset library | 59 factory presets |
| Resizable interface, full-screen mode | Freely resizable interface |
| Mono and surround | Stereo |
| AAX, AudioSuite | Not supported (needs Avid's SDK and signing) |

## Build from source

The plugin is built on [DPF](https://github.com/DISTRHO/DPF):

```
git clone --recursive https://github.com/DISTRHO/DPF.git dpf
make -C plugins/Magma                    # Linux / macOS (macOS also builds the AU)
make -C plugins/Magma WINDOWS=true CC=x86_64-w64-mingw32-gcc CXX=x86_64-w64-mingw32-g++
```

GitHub Actions builds all three platforms on every push. The macOS build is universal and ad-hoc signed. A `v*` tag, or a release published on GitHub, attaches the zips to that release.

## Tests

```
cd tests
g++ -O2 -std=c++17 -I../plugins/Magma features.cpp -o features && ./features     # flat band sum, level compensation for all styles, harmonics, band isolation,
                                                                                # mute/solo/level/pan, tone, dynamics, feedback + auto-mute, mid/side,
                                                                                # aligned mix, XLFO, steps, EG, EF (sidechain), XY, MIDI, slot-on-slot
g++ -O2 -std=c++17 -I../plugins/Magma features2.cpp -o features2 && ./features2  # 2.0: linear-phase reconstruction, isolation per slope, 6 dB/oct sum,
                                                                                # aliasing per quality, new styles, sag, transformer, switching while playing
g++ -O2 -std=c++17 -I../plugins/Magma compat.cpp -o compat && ./compat           # random 1.x settings through the frozen 1.0 engine and 2.0: identical
g++ -O2 -std=c++17 -I../plugins/Magma fuzz.cpp -o fuzz && ./fuzz                 # random parameters, modulation and MIDI at 44.1-192 kHz: always finite
g++ -O2 -std=c++17 -I../plugins/Magma presets.cpp -o presets && ./presets        # plays a test mix through every preset
g++ -O2 -std=c++17 -I../plugins/Magma modpresets.cpp -o modpresets && ./modpresets  # source presets in range, user files round-trip
g++ -O3 -std=c++17 -I../plugins/Magma bench.cpp -o bench && ./bench              # CPU per phase, quality and band count
```

CI runs all of them except the benchmark on every push.

CPU use with modulation running (one core of a cloud build machine, 48 kHz):

| Bands | Normal (2×) | Good (8×) | Superb (32×) |
|---|---|---|---|
| 1 | 1.5% | 5.2% | 20% |
| 3 | 3.5% | 14% | 58% |
| 6 | 7.8% | 29% | 127% |
| 6, linear phase | 16% | 38% | 122% |

Superb is meant for rendering: with many bands it can need more than one core in real time.

Release checks:
- Steinberg's VST3 validator passes 47/47.
- The interface is exercised with scripted input and a live test mix:
  - splitting and merging bands, undo
  - the style picker
  - drag-and-drop modulation, the step sequencer, the XY pad
  - presets, tips and help

## Source layout

| File | Contents |
|---|---|
| `plugins/Magma/MagmaEngine.hpp` | All DSP: crossovers (natural and linear phase), styles, band processing, oversampling, modulation sources and matrix. Header-only |
| `plugins/Magma/ParamDefs.hpp` | Parameter names, labels, value mapping, source and destination names |
| `plugins/Magma/Presets.hpp` | Factory presets |
| `plugins/Magma/UserPresets.hpp` | User preset files |
| `plugins/Magma/ComponentPresets.hpp` | Factory and user presets for single modulation sources |
| `plugins/Magma/common/` | Shared Epsilon Audio code (oversampling, convolution, FFT), vendored from epsilon-common |
| `plugins/Magma/Help.hpp` | Hover tip text and in-plugin help pages |
| `plugins/Magma/MagmaPlugin.hpp/.cpp` | Plugin wrapper: parameters, MIDI, tempo, sidechain, latency, MIDI Learn |
| `plugins/Magma/MagmaUI.cpp` | Custom vector interface (NanoVG) |
| `plugins/Magma/Fft.hpp` | Small FFT for the analyser |
| `docs/MANUAL.md` | User manual (`Magma-Manual.html` is generated from it with pandoc) |
| `tests/legacy/` | Frozen copy of the 1.0 engine, for the compatibility test |

## Changelog

- **2.0:** Brings Magma up to Saturn 2. New in this version:
  - Twelve new styles (28 in all); the Experimental family is renamed FX.
  - Linear-phase crossovers.
  - A 6 dB/oct crossover slope.
  - Superb quality, 32× oversampling (HQ is now called Good).
  - Presets for single modulation sources (XLFO, EG, EF).
  - A slot panel on every modulated knob.
  - Seventeen new factory presets.

  Sessions, automation and presets from 1.x load and sound the same. A test checks this against the frozen 1.0 engine.

  The Style and Crossover Slope parameters gained values at the end of their ranges. In hosts that store automation as a 0-1 position, check those lanes after upgrading.
- **1.0:** First release.

## License

[MIT](LICENSE). Built with [DPF](https://github.com/DISTRHO/DPF) and its bundled NanoVG, which are under the ISC license. `plugins/Magma/ResizeHandle.hpp` comes from DPF's examples and keeps its ISC notice.

"FabFilter" and "FabFilter Saturn" are trademarks of FabFilter Software Instruments. Magma is not affiliated with or endorsed by FabFilter.
