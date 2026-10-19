// Magma — hover tips and the in-plugin help pages. License: MIT.
#pragma once
#include "MagmaEngine.hpp"

namespace magma {

static const char* const kManualURL = "https://github.com/natehouk/magma/blob/main/docs/MANUAL.md";

inline const char* styleTip(int s) {
    switch (s) {
    case 0: return "Clean Tube: gentle, slightly asymmetric warmth. Adds mostly even harmonics.";
    case 1: return "Warm Tube: rounder and thicker, with more even harmonics.";
    case 2: return "Hot Tube: two driven stages. Thick, compressed and singing.";
    case 3: return "Broken Tube: a starved, lopsided stage with a small dead zone: gated, spitting fuzz.";
    case 4: return "Clean Tape: soft, symmetric saturation with a gentle high-frequency roll-off.";
    case 5: return "Warm Tape: more drive and roll-off, with a touch of tape-like memory.";
    case 6: return "Hot Tape: saturated tape pushed into compression.";
    case 7: return "Clean Amp: a guitar amp at the edge of break-up, with a speaker cabinet roll-off.";
    case 8: return "Crunch Amp: two gain stages and a mid lift. Classic rock crunch.";
    case 9: return "Lead Amp: three gain stages, a strong mid lift and a resonant cabinet. Sustain for days.";
    case 10: return "Soft Saturation: the smoothest curve. Rounds peaks without much color.";
    case 11: return "Hard Clip: flat tops. Brutal and loud; use lightly or in parallel.";
    case 12: return "Rectify: folds the negative half of the wave up; more drive, more rectified. Octave-up, ring-like tones.";
    case 13: return "Crush: bit-depth and sample-rate reduction; Drive takes it from subtle grit to 2-bit chaos.";
    case 14: return "Smear: replays tiny grains from the recent past and feeds them back. Drive sets how much smears.";
    case 15: return "Fold: a wavefolder. Past full scale the wave folds back on itself: bright, metallic, synthetic.";
    default: return "";
    }
}

inline const char* paramTip(uint32_t p) {
    if (p >= kBandFirst && p < kXlfoFirst) {
        switch ((p - kBandFirst) % kBandFields) {
        case kBStyle: return "The distortion style for this band. Click to choose from 16 styles in five families: Tube, Tape, Amp, Saturation and Experimental.";
        case kBDrive: return "How hard this band is driven into the distortion. The level is compensated, so Drive changes the character, not the volume.";
        case kBDynamics: return "Right: compresses the band before the distortion (denser, more sustain). Left: expands it (more punch, quieter tails). Centre: off.";
        case kBFbAmt: return "Feeds the band's output back into its input through a resonant filter. Small amounts add a ringing tone; large amounts howl. Auto Mute stops it when the input goes silent.";
        case kBFbFreq: return "The frequency the feedback resonates at.";
        case kBBass: return "Low shelf at 120 Hz, after the distortion, +-12 dB.";
        case kBMid: return "Mid bell at 1 kHz, after the distortion, +-12 dB.";
        case kBTreble: return "High shelf at 4.5 kHz, after the distortion, +-12 dB.";
        case kBPresence: return "Positive: adds air above 10 kHz. Negative: rolls off the top like a speaker cabinet, down to 1.5 kHz at full.";
        case kBMix: return "Dry/wet for this band only. Lower values blend the clean band back in (parallel distortion).";
        case kBLevel: return "Output level of this band, +-24 dB.";
        case kBPan: return "Moves this band left or right (in mid/side mode: towards mid or side).";
        case kBMute: return "Silences this band.";
        case kBSolo: return "Plays only the soloed bands.";
        }
    }
    switch (p) {
    case kXSlope: return "Steepness of the crossovers between bands: 12, 24 or 48 dB per octave. Steeper keeps each band's distortion to its own range. The bands always add up flat.";
    case kChannelMode: return "STEREO processes left and right. MID/SIDE processes the centre and the stereo difference instead; pan then moves a band between mid and side.";
    case kQuality: return "NORMAL runs at 2x oversampling. HQ runs at 8x: cleaner high frequencies with heavy drive, at about four times the CPU.";
    case kAutoMute: return "Stops feedback from ringing on once the input has been silent for a quarter of a second.";
    case kMix: return "Global dry/wet. The dry signal is delayed to stay in time with the processed one.";
    case kOutGain: return "Final output level.";
    case kMidiOn: return "Turns MIDI control on or off (notes, controllers and MIDI Learn).";
    default: break;
    }
    if (p >= kXlfoFirst && p < kEgFirst) {
        const int f = (p - kXlfoFirst) % kXlfoFields;
        switch (f) {
        case kLMode: return "WAVE: a classic LFO shape. STEPS: a 16-step sequencer you draw in the step display.";
        case kLWave: return "The LFO shape: sine, triangle, saw up, saw down, square, smooth random, sample & hold.";
        case kLRate: return "LFO speed in Hz when not synced.";
        case kLSync: return "FREE runs at the Rate. STRAIGHT, DOTTED and TRIPLET lock to the host tempo and follow the song position.";
        case kLDiv: return "Length of one cycle when synced, from 16 bars down to a 1/64 note.";
        case kLBalance: return "Skews the shape: for triangle and sine from falling to rising ramp; for square it sets the duty cycle.";
        case kLGlide: return "In step mode, how smoothly the value slides from one step to the next.";
        case kLSteps: return "How many steps the sequence uses before it repeats.";
        default: return "One step of the sequence. Drag up and down in the step display.";
        }
    }
    if (p >= kEgFirst && p < kEfFirst) {
        switch ((p - kEgFirst) % kEgFields) {
        case kEAttack: return "Envelope attack time."; case kEDecay: return "Envelope decay time."; case kESustain: return "Level the envelope holds while triggered.";
        case kERelease: return "Envelope release time.";
        case kETrigger: return "MIDI: the envelope starts with each note. AUDIO: it starts when the input rises above the threshold.";
        case kEThreshold: return "Input level that triggers the envelope in AUDIO mode.";
        }
    }
    if (p >= kEfFirst && p < kXyFirst) {
        switch ((p - kEfFirst) % kEfFields) {
        case kFAttack: return "How fast the follower rises with the level."; case kFRelease: return "How fast it falls back.";
        case kFGain: return "Sensitivity: raises or lowers the level the follower sees.";
        case kFSource: return "INPUT follows the plugin's input. SIDECHAIN follows the sidechain input (route another track to it in your DAW).";
        }
    }
    if (p >= kXyFirst && p < kSlotFirst) return "XY controller position. Drag the dot in the pad; X and Y are two separate sources.";
    if (p >= kSlotFirst && p < kOutLearn) {
        switch ((p - kSlotFirst) % kSlotFields) {
        case kSlotSrc: return "The modulation source. Click to change.";
        case kSlotDst: return "What this slot modulates. Click to change.";
        case kSlotAmt: return "How much, and in which direction. Turn left to invert.";
        case kSlotBypass: return "Switches this slot off without deleting it.";
        }
    }
    return "";
}

enum TipTarget { kTipUndo, kTipRedo, kTipLearn, kTipMidi, kTipHelp, kTipA, kTipB, kTipCopy, kTipPrev, kTipPresets, kTipNext,
                 kTipDisplay, kTipDivider, kTipChip, kTipStepEdit, kTipXyPad, kTipRemoveBand, kTipSourceTab, kNumTipTargets };
inline const char* targetTitle(int t) {
    static const char* const n[kNumTipTargets] = { "Undo", "Redo", "MIDI Learn", "MIDI", "Help", "Setting A", "Setting B", "Copy A/B",
        "Previous preset", "Presets", "Next preset", "Band display", "Crossover", "Modulation source", "Steps", "XY pad", "Remove band", "Sources" };
    return n[t];
}
inline const char* targetTip(int t) {
    switch (t) {
    case kTipUndo: return "Steps back through your edits, including preset loads and A/B switches. Cmd/Ctrl-Z.";
    case kTipRedo: return "Re-applies an edit you undid. Cmd/Ctrl-Shift-Z or Cmd/Ctrl-Y.";
    case kTipLearn: return "Click, then click any control and move a knob or fader on your MIDI controller to assign it. Right-click to unassign.";
    case kTipMidi: return paramTip(kMidiOn);
    case kTipHelp: return "Opens the help pages: bands, styles, the band controls, modulation, MIDI and shortcuts.";
    case kTipA: case kTipB: return "Magma keeps two complete settings, A and B. Click the other letter to switch and compare.";
    case kTipCopy: return "Copies the current setting to the other slot.";
    case kTipPrev: return "Load the previous preset. The mouse wheel over the preset name also steps through presets.";
    case kTipPresets: return "Open the preset browser. A * after the name means you have changed it.";
    case kTipNext: return "Load the next preset.";
    case kTipDisplay: return "The spectrum (input faint, output bright) with your bands. Click a band to edit it. Double-click to split a band in two at that frequency (up to six). Drag a band up and down to change its drive.";
    case kTipDivider: return "A crossover between two bands. Drag it left and right. Right-click to remove it and merge the two bands.";
    case kTipChip: return "Drag this onto any knob to modulate it. A coloured ring then shows the knob is modulated, and the slot appears in the list on the right.";
    case kTipStepEdit: return "Draw the sequence: click or drag up and down across the steps. Cmd/Ctrl-click resets a step to zero.";
    case kTipXyPad: return "Drag the dot. X and Y are separate modulation sources; drag their chips onto knobs.";
    case kTipRemoveBand: return "Removes this band and merges it into its neighbour.";
    case kTipSourceTab: return "Choose a modulation source to edit. Each source has a chip you drag onto the knobs it should move.";
    default: return "";
    }
}

struct HelpPage { const char* title; const char* body; };
static const HelpPage kHelpPages[] = {
    { "Overview",
      "Magma is a multiband distortion: it splits the sound into up to six frequency bands and drives each one through its own distortion style, from subtle tube "
      "warmth to bit crushing. A modulation system with LFOs, step sequencers, envelopes, envelope followers, XY pads and MIDI can move almost every control.\n\n"
      "Start with one band: pick a style, turn up Drive, then shape the result with the tone controls. Add bands by double-clicking the display.\n\n"
      "Hover over anything for a moment to see what it does. Turn the tips off at the bottom of this window." },
    { "Bands",
      "Double-click the display to split the band under the pointer in two (up to six). Drag a crossover line to move it; right-click it to merge the two bands. "
      "Click a band to edit it in the panel below; drag a band up or down to change its drive.\n\n"
      "Crossovers are 12, 24 or 48 dB/oct and always add up to a flat response, so a band with no drive is completely transparent.\n\n"
      "MUTE silences a band, SOLO plays only the soloed bands." },
    { "Styles",
      "TUBE: Clean, Warm, Hot and Broken, mostly even harmonics.\nTAPE: Clean, Warm and Hot, soft symmetric saturation with a gentle top-end roll-off.\n"
      "AMP: Clean, Crunch and Lead guitar amps, with speaker-cabinet filtering.\nSATURATION: Soft and Hard Clip.\n"
      "EXPERIMENTAL: Rectify, Crush (bits and sample rate), Smear (grain feedback) and Fold (wavefolder).\n\n"
      "The level is compensated as you turn up Drive, so you hear the character change, not just a louder signal." },
    { "Band controls",
      "DRIVE: how hard the band hits the distortion.\nDYNAMICS: right compresses before the distortion, left expands.\n"
      "FEEDBACK and FREQ: feed the band back into itself through a resonant filter for ringing and howling.\n"
      "BASS, MID, TREBLE, PRESENCE: tone shaping after the distortion. Negative Presence acts like a speaker cabinet.\n"
      "MIX: parallel blend for this band. LEVEL and PAN: the band's level and position.\n\n"
      "Globally: STEREO or MID/SIDE, NORMAL (2x) or HQ (8x oversampling), crossover slope, AUTO MUTE for feedback, MIX and OUTPUT." },
    { "Modulation",
      "Pick a source in the tabs at the bottom left: four XLFOs (wave or 16-step), two envelope generators (triggered by MIDI notes or by the input level), "
      "two envelope followers (input or sidechain), two XY pads and the MIDI sources.\n\n"
      "Drag the source's chip onto any knob. That creates a modulation slot (up to 50) shown in the list at the bottom right, where you can change its amount, "
      "destination or source, bypass it or delete it. A slot's amount can itself be modulated by another slot.\n\n"
      "Modulated knobs get a coloured ring, and a small dot shows where the modulation has taken them right now." },
    { "A/B, undo & MIDI",
      "A/B holds two complete settings; COPY copies the current one to the other slot. Every edit is an undo step (Cmd/Ctrl-Z, Cmd/Ctrl-Shift-Z).\n\n"
      "MIDI LEARN: click LEARN, click a control, move a controller. MIDI notes also drive the Velocity and Key sources and can trigger the envelopes.\n\n"
      "Double-click a knob to type a value; Cmd/Ctrl-click resets it. Shift-drag is fine adjustment. Esc closes this window." },
};
static constexpr int kNumHelpPages = sizeof(kHelpPages) / sizeof(kHelpPages[0]);

} // namespace magma
