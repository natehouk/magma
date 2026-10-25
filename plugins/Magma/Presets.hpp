// Factory presets. Each preset is a small function that changes the engine defaults. License: MIT.
// Engine defaults: one band, Warm Tube at 30% drive, everything else neutral; modulation empty.
#pragma once
#include "MagmaEngine.hpp"
#include <algorithm>

namespace magma {

struct Preset { const char* name; const char* category; float v[kParamCount]; };
static const char* const kCategories[] = { "Warmth", "Drums", "Bass", "Vocals & Keys", "Guitar", "Mix & Master", "Creative" };
static constexpr int kNumCategories = 7;

namespace pre {
typedef Engine E;
inline void band(E& e, int b, int style, float drive, float mix = 1, float level = 0) { e.p[bp(b, kBStyle)] = (float)style; e.p[bp(b, kBDrive)] = drive; e.p[bp(b, kBMix)] = mix; e.p[bp(b, kBLevel)] = level; }
inline void tone(E& e, int b, float bass, float mid, float treble, float pres) { e.p[bp(b, kBBass)] = bass; e.p[bp(b, kBMid)] = mid; e.p[bp(b, kBTreble)] = treble; e.p[bp(b, kBPresence)] = pres; }
inline void bands(E& e, int n, float x1 = 120, float x2 = 600, float x3 = 2500, float x4 = 6000, float x5 = 12000) {
    e.p[kNumBands] = (float)n; e.p[kX1] = x1; e.p[kX2] = x2; e.p[kX3] = x3; e.p[kX4] = x4; e.p[kX5] = x5; }
inline void lfo(E& e, int l, int wave, float rate, int sync = 0, int div = 4) { e.p[lp_(l, kLWave)] = (float)wave; e.p[lp_(l, kLRate)] = rate; e.p[lp_(l, kLSync)] = (float)sync; e.p[lp_(l, kLDiv)] = (float)div; }
inline void steps(E& e, int l, std::initializer_list<float> v, int div = 4, float glide = 0) {
    e.p[lp_(l, kLMode)] = 1; e.p[lp_(l, kLSync)] = 1; e.p[lp_(l, kLDiv)] = (float)div; e.p[lp_(l, kLGlide)] = glide; int i = 0;
    for (float x : v) e.p[lp_(l, kLStep1 + i++)] = x;
    e.p[lp_(l, kLSteps)] = (float)i; }
inline int dstBand(int b, int field) { for (int k = 0; k < kNumBandDests; ++k) if (kBandDestFields[k] == field) return kDstBandFirst + b * kNumBandDests + k; return 0; }

// ---- Warmth
inline void init(E&) {}
inline void tapeGlue(E& e) { band(e, 0, 5, 0.25f); tone(e, 0, 1, 0, -1, 0); }
inline void tubeSheen(E& e) { band(e, 0, 0, 0.35f, 0.7f); tone(e, 0, 0, 0, 1.5f, 2); }
inline void warmBlanket(E& e) { band(e, 0, 1, 0.4f); tone(e, 0, 2, 0, -2, -3); }
inline void subtleSat(E& e) { band(e, 0, 10, 0.3f, 0.5f); }
inline void analogConsole(E& e) { bands(e, 2, 200); band(e, 0, 4, 0.2f); band(e, 1, 0, 0.3f); tone(e, 1, 0, 0.5f, 1, 0); }
// ---- Drums
inline void drumSmash(E& e) { band(e, 0, 6, 0.7f, 1, -2); e.p[bp(0, kBDynamics)] = 0.6f; tone(e, 0, 2, -1, 2, 1); }
inline void kickWeight(E& e) { bands(e, 2, 150); band(e, 0, 1, 0.55f); tone(e, 0, 3, 0, 0, 0); band(e, 1, 0, 0.1f, 0.3f); }
inline void snareCrack(E& e) { bands(e, 3, 200, 3000); band(e, 0, 0, 0.1f, 0); band(e, 1, 2, 0.5f, 0.8f); band(e, 2, 11, 0.4f, 0.6f, -2); }
inline void loFiBreak(E& e) { band(e, 0, 13, 0.55f, 0.8f); tone(e, 0, 1, 2, -3, -6); }
inline void parallelCrush(E& e) { band(e, 0, 6, 0.85f, 0.35f); e.p[bp(0, kBDynamics)] = 0.8f; }
inline void roomPump(E& e) { band(e, 0, 5, 0.5f); e.p[bp(0, kBDynamics)] = 0.9f; e.setSlot(0, kSrcEf1, dstBand(0, kBLevel), -0.1f); }
// ---- Bass
inline void bassGrit(E& e) { bands(e, 2, 180); band(e, 0, 0, 0.1f); band(e, 1, 8, 0.6f, 0.8f, -3); }
inline void subSafe(E& e) { bands(e, 2, 100); band(e, 0, 0, 0, 0); band(e, 1, 6, 0.6f); }
inline void reeseGrowl(E& e) { bands(e, 3, 120, 900); band(e, 0, 0, 0.1f, 0.3f); band(e, 1, 15, 0.45f); band(e, 2, 9, 0.5f, 0.7f, -4);
    lfo(e, 0, 0, 0.3f); e.setSlot(0, kSrcXlfo1, dstBand(1, kBDrive), 0.3f); }
inline void fuzzBass(E& e) { band(e, 0, 9, 0.6f, 0.8f); tone(e, 0, 3, -2, -2, -4); }
inline void tapeBass(E& e) { band(e, 0, 6, 0.45f); tone(e, 0, 2, 0, -1, -2); }
// ---- Vocals & Keys
inline void vocalPresence(E& e) { bands(e, 3, 250, 4000); band(e, 0, 0, 0, 0); band(e, 1, 1, 0.3f, 0.6f); band(e, 2, 0, 0.35f, 0.5f); tone(e, 2, 0, 0, 1, 2); }
inline void radioVoice(E& e) { band(e, 0, 7, 0.5f); tone(e, 0, -12, 4, -6, -6); }
inline void rhodesBark(E& e) { band(e, 0, 1, 0.55f); e.p[bp(0, kBDynamics)] = -0.3f; }
inline void pianoAir(E& e) { bands(e, 2, 5000); band(e, 0, 0, 0, 0); band(e, 1, 4, 0.4f, 0.5f); tone(e, 1, 0, 0, 0, 3); }
inline void organDrive(E& e) { band(e, 0, 8, 0.45f); tone(e, 0, 0, 2, 0, -2); }
// ---- Guitar
inline void cleanAmp(E& e) { band(e, 0, 7, 0.35f); tone(e, 0, 1, 1, 1, 0); }
inline void crunch(E& e) { band(e, 0, 8, 0.55f); tone(e, 0, 0, 2, 1, -1); }
inline void lead(E& e) { band(e, 0, 9, 0.75f, 1, -2); tone(e, 0, -1, 3, 0, -2); e.p[bp(0, kBFbAmt)] = 0.15f; e.p[bp(0, kBFbFreq)] = 1800; }
inline void fuzzFace(E& e) { band(e, 0, 3, 0.8f); tone(e, 0, 2, -3, 0, -3); }
inline void feedbackHowl(E& e) { band(e, 0, 9, 0.8f, 1, -4); e.p[bp(0, kBFbAmt)] = 0.6f; e.p[bp(0, kBFbFreq)] = 900;
    lfo(e, 0, 0, 0.15f); e.setSlot(0, kSrcXlfo1, dstBand(0, kBFbFreq), 0.12f); }
// ---- Mix & Master
inline void mixBusWarm(E& e) { band(e, 0, 4, 0.2f); tone(e, 0, 0.5f, 0, 0.5f, 0); }
inline void masterExciter(E& e) { bands(e, 2, 6000); band(e, 0, 0, 0, 0); band(e, 1, 0, 0.4f, 0.35f); tone(e, 1, 0, 0, 0, 2); }
inline void msWidth(E& e) { e.p[kChannelMode] = 1; band(e, 0, 1, 0.45f, 0.6f); }
inline void threeBandWarm(E& e) { bands(e, 3, 200, 3500); band(e, 0, 5, 0.3f); band(e, 1, 1, 0.25f); band(e, 2, 0, 0.3f, 0.6f); }
inline void loudnessFatten(E& e) { bands(e, 4, 100, 500, 5000); band(e, 0, 0, 0.2f); band(e, 1, 5, 0.3f); band(e, 2, 1, 0.3f); band(e, 3, 0, 0.25f, 0.6f); }
// ---- Creative
inline void stepGate(E& e) { band(e, 0, 6, 0.5f, 1, -12); steps(e, 0, { 1, -1, 1, 1, -1, 1, -1, -1, 1, -1, 1, 1, -1, -1, 1, -1 }, 8); e.setSlot(0, kSrcXlfo1, dstBand(0, kBLevel), 0.25f); }
inline void wobbleDrive(E& e) { band(e, 0, 9, 0.4f); lfo(e, 0, 0, 1, 1, 7); e.setSlot(0, kSrcXlfo1, dstBand(0, kBDrive), 0.4f); e.setSlot(1, kSrcXlfo1, dstBand(0, kBPresence), 0.3f); }
inline void smearCloud(E& e) { band(e, 0, 14, 0.6f, 0.7f); tone(e, 0, 0, 0, -2, -3); }
inline void bitDecay(E& e) { band(e, 0, 13, 0.3f, 0.8f); e.p[ep(0, kETrigger)] = 1; e.setSlot(0, kSrcEg1, dstBand(0, kBDrive), 0.5f); }
inline void rectifier(E& e) { band(e, 0, 12, 0.6f, 0.7f); tone(e, 0, -3, 0, 0, 0); }
inline void foldSweep(E& e) { band(e, 0, 15, 0.4f); lfo(e, 0, 1, 0.25f); e.setSlot(0, kSrcXlfo1, dstBand(0, kBDrive), 0.35f); }
inline void envelopeFuzz(E& e) { band(e, 0, 9, 0.2f); e.setSlot(0, kSrcEf1, dstBand(0, kBDrive), 0.6f); }
inline void xyMorph(E& e) { bands(e, 2, 800); band(e, 0, 6, 0.4f); band(e, 1, 15, 0.4f); e.setSlot(0, kSrcXy1X, dstBand(0, kBDrive), 0.5f);
    e.setSlot(1, kSrcXy1Y, dstBand(1, kBDrive), 0.5f); e.setSlot(2, kSrcXy1X, kDstXFirst, 0.4f); e.p[xyp(0, 0)] = 0.3f; e.p[xyp(0, 1)] = 0.3f; }
inline void midiDrive(E& e) { band(e, 0, 8, 0.2f); e.setSlot(0, kSrcVelocity, dstBand(0, kBDrive), 0.5f); e.setSlot(1, kSrcWheel, dstBand(0, kBPresence), -0.5f); }
inline void crossoverDance(E& e) { bands(e, 2, 600); band(e, 0, 0, 0.1f); band(e, 1, 11, 0.6f, 1, -4); lfo(e, 0, 1, 1, 1, 5); e.setSlot(0, kSrcXlfo1, kDstXFirst, 0.3f); }
// ---- 2.0
inline void linear(E& e, int slope = kSlope48) { e.p[kXPhase] = 1; e.p[kXSlope] = (float)slope; }
inline void gentleGlow(E& e) { band(e, 0, 16, 0.45f); tone(e, 0, 0.5f, 0, 1, 1); }
inline void ironTransformer(E& e) { band(e, 0, 22, 0.4f); tone(e, 0, 1, 0, -0.5f, 0); }
inline void linearMasterWarmth(E& e) { bands(e, 4, 120, 800, 6000); linear(e); band(e, 0, 22, 0.25f); band(e, 1, 4, 0.2f); band(e, 2, 16, 0.25f); band(e, 3, 0, 0.2f, 0.5f); }
inline void dirtyTapeDrums(E& e) { band(e, 0, 18, 0.55f); e.p[bp(0, kBDynamics)] = 0.4f; tone(e, 0, 1.5f, 0, -1, -2); }
inline void destroyedKick(E& e) { bands(e, 2, 200); band(e, 0, 24, 0.5f, 0.3f); band(e, 1, 0, 0.1f, 0.5f); }
inline void subOctaveBass(E& e) { bands(e, 2, 250); band(e, 0, 27, 0.6f); band(e, 1, 0, 0.1f, 0.5f); }
inline void linearBassSplit(E& e) { bands(e, 2, 120); linear(e); band(e, 0, 16, 0.15f); band(e, 1, 20, 0.45f, 0.8f, -4); }
inline void saggingVocal(E& e) { band(e, 0, 17, 0.35f, 0.6f); tone(e, 0, -1, 0, 1, 1); }
inline void breakupKeys(E& e) { band(e, 0, 19, 0.45f); tone(e, 0, 0, 1.5f, 0, -1); }
inline void metalRhythm(E& e) { band(e, 0, 20, 0.6f, 1, -3); tone(e, 0, 1, -3, 1, -1); }
inline void breakupAmp(E& e) { band(e, 0, 19, 0.55f); tone(e, 0, 0, 2, 1, -1); }
inline void superbSheen(E& e) { bands(e, 3, 250, 5000); linear(e, kSlope24); e.p[kQuality] = 1; e.p[kSuperb] = 1; band(e, 0, 16, 0.15f); band(e, 1, 4, 0.15f); band(e, 2, 0, 0.3f, 0.5f); tone(e, 2, 0, 0, 0, 1.5f); }
inline void heavySatBus(E& e) { band(e, 0, 21, 0.4f, 0.7f); tone(e, 0, 0.5f, 0, 0.5f, 0); }
inline void ringSweep(E& e) { band(e, 0, 25, 0.4f, 0.6f); lfo(e, 0, 1, 0.2f); e.setSlot(0, kSrcXlfo1, dstBand(0, kBDrive), 0.3f); }
inline void octaveFuzz(E& e) { band(e, 0, 26, 0.5f, 0.8f); tone(e, 0, -2, 1, 0, -3); }
inline void downsampled(E& e) { band(e, 0, 23, 0.4f, 0.8f); e.p[ep(0, kETrigger)] = 1; e.setSlot(0, kSrcEg1, dstBand(0, kBDrive), -0.3f); }
inline void gentleSplit(E& e) { bands(e, 3, 200, 2500); e.p[kXSlope] = kSlope6; band(e, 0, 22, 0.3f); band(e, 1, 1, 0.35f); band(e, 2, 18, 0.3f, 0.7f); }
} // namespace pre

struct PresetDef { const char* name; const char* category; void (*fn)(Engine&); };
static const PresetDef kPresetDefs[] = {
    { "Init", "Init", pre::init },
    { "Tape Glue", "Warmth", pre::tapeGlue }, { "Tube Sheen", "Warmth", pre::tubeSheen }, { "Warm Blanket", "Warmth", pre::warmBlanket },
    { "Subtle Saturation", "Warmth", pre::subtleSat }, { "Analog Console", "Warmth", pre::analogConsole },
    { "Drum Smash", "Drums", pre::drumSmash }, { "Kick Weight", "Drums", pre::kickWeight }, { "Snare Crack", "Drums", pre::snareCrack },
    { "Lo-Fi Break", "Drums", pre::loFiBreak }, { "Parallel Crush", "Drums", pre::parallelCrush }, { "Room Pump", "Drums", pre::roomPump },
    { "Bass Grit", "Bass", pre::bassGrit }, { "Sub Safe Drive", "Bass", pre::subSafe }, { "Reese Growl", "Bass", pre::reeseGrowl },
    { "Fuzz Bass", "Bass", pre::fuzzBass }, { "Tape Bass", "Bass", pre::tapeBass },
    { "Vocal Presence", "Vocals & Keys", pre::vocalPresence }, { "Radio Voice", "Vocals & Keys", pre::radioVoice }, { "Rhodes Bark", "Vocals & Keys", pre::rhodesBark },
    { "Piano Air", "Vocals & Keys", pre::pianoAir }, { "Organ Drive", "Vocals & Keys", pre::organDrive },
    { "Clean Amp", "Guitar", pre::cleanAmp }, { "Crunch", "Guitar", pre::crunch }, { "Lead", "Guitar", pre::lead },
    { "Fuzz Face", "Guitar", pre::fuzzFace }, { "Feedback Howl", "Guitar", pre::feedbackHowl },
    { "Mix Bus Warmth", "Mix & Master", pre::mixBusWarm }, { "Master Exciter", "Mix & Master", pre::masterExciter }, { "Mid/Side Width", "Mix & Master", pre::msWidth },
    { "Three-Band Warmth", "Mix & Master", pre::threeBandWarm }, { "Loudness Fattener", "Mix & Master", pre::loudnessFatten },
    { "Step Gate", "Creative", pre::stepGate }, { "Wobble Drive", "Creative", pre::wobbleDrive }, { "Smear Cloud", "Creative", pre::smearCloud },
    { "Bit Decay", "Creative", pre::bitDecay }, { "Rectifier", "Creative", pre::rectifier }, { "Fold Sweep", "Creative", pre::foldSweep },
    { "Envelope Fuzz", "Creative", pre::envelopeFuzz }, { "XY Morph", "Creative", pre::xyMorph }, { "MIDI Drive", "Creative", pre::midiDrive },
    { "Crossover Dance", "Creative", pre::crossoverDance },
    // 2.0 (appended, so 1.x program numbers stay the same)
    { "Gentle Tube Glow", "Warmth", pre::gentleGlow }, { "Iron Transformer", "Warmth", pre::ironTransformer }, { "Linear Master Warmth", "Warmth", pre::linearMasterWarmth },
    { "Dirty Tape Drums", "Drums", pre::dirtyTapeDrums }, { "Destroyed Kick", "Drums", pre::destroyedKick },
    { "Sub Octave Bass", "Bass", pre::subOctaveBass }, { "Linear Bass Split", "Bass", pre::linearBassSplit },
    { "Sagging Tube Vocal", "Vocals & Keys", pre::saggingVocal }, { "Breakup Keys", "Vocals & Keys", pre::breakupKeys },
    { "Metal Rhythm", "Guitar", pre::metalRhythm }, { "Breakup Amp", "Guitar", pre::breakupAmp },
    { "Superb Sheen", "Mix & Master", pre::superbSheen }, { "Heavy Saturation Bus", "Mix & Master", pre::heavySatBus },
    { "Ring Mod Sweep", "Creative", pre::ringSweep }, { "Octave Fuzz", "Creative", pre::octaveFuzz }, { "Downsampled", "Creative", pre::downsampled },
    { "Gentle 6 dB Split", "Creative", pre::gentleSplit },
};
static constexpr uint32_t kNumPresets = sizeof(kPresetDefs) / sizeof(kPresetDefs[0]);

inline const Preset* presets() {
    static Preset* table = nullptr;
    if (table) return table;
    table = new Preset[kNumPresets];
    for (uint32_t i = 0; i < kNumPresets; ++i) {
        Engine* e = new Engine();                  // on the heap: the engine carries analyser buffers
        kPresetDefs[i].fn(*e);
        table[i].name = kPresetDefs[i].name; table[i].category = kPresetDefs[i].category;
        std::copy(e->p, e->p + kParamCount, table[i].v);
        delete e;
    }
    return table;
}

} // namespace magma
