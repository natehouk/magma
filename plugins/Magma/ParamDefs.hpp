// Parameter metadata shared by the DSP wrapper and the UI. License: MIT.
#pragma once
#include "MagmaEngine.hpp"
#include <cstdio>
#include <string>

namespace magma {

static constexpr uint32_t kHintInteger = 1, kHintBoolean = 2, kHintLog = 4, kHintOutput = 8, kHintBipolar = 16;
struct ParamDef { const char* name; const char* sym; const char* unit; float min, max; uint32_t hints; };

inline const char* sourceName(int s) {
    static const char* const n[kNumSources] = { "None", "XLFO 1", "XLFO 2", "XLFO 3", "XLFO 4", "EG 1", "EG 2", "EF 1", "EF 2",
        "XY 1 X", "XY 1 Y", "XY 2 X", "XY 2 Y", "Velocity", "Key", "Mod Wheel", "Pitch Bend", "Aftertouch" };
    return n[s < 0 || s >= kNumSources ? 0 : s];
}
inline const char* srcShort(int s) {
    static const char* const n[kNumSources] = { "-", "XLFO1", "XLFO2", "XLFO3", "XLFO4", "EG1", "EG2", "EF1", "EF2",
        "XY1 X", "XY1 Y", "XY2 X", "XY2 Y", "VEL", "KEY", "WHEEL", "BEND", "AT" };
    return n[s < 0 || s >= kNumSources ? 0 : s];
}
inline const char* bandFieldName(int f) {
    static const char* const n[kBandFields] = { "Style", "Drive", "Dynamics", "Feedback", "Feedback Freq", "Bass", "Mid", "Treble", "Presence", "Mix", "Level", "Pan", "Mute", "Solo" };
    return n[f];
}
inline const char* destName(int d) {
    static std::string names[kNumDests]; static bool init = false;
    if (!init) {
        init = true; char b[48];
        names[kDstNone] = "None";
        for (int bd = 0; bd < kMaxBands; ++bd) for (int k = 0; k < kNumBandDests; ++k) { snprintf(b, sizeof b, "Band %d %s", bd + 1, bandFieldName(kBandDestFields[k])); names[kDstBandFirst + bd * kNumBandDests + k] = b; }
        for (int i = 0; i < 5; ++i) { snprintf(b, sizeof b, "Crossover %d", i + 1); names[kDstXFirst + i] = b; }
        names[kDstMix] = "Global Mix"; names[kDstOutGain] = "Output Gain";
        for (int l = 0; l < kNumXlfo; ++l) { snprintf(b, sizeof b, "XLFO %d Rate", l + 1); names[kDstXlfoRateFirst + l] = b; snprintf(b, sizeof b, "XLFO %d Balance", l + 1); names[kDstXlfoBalFirst + l] = b; }
        static const char* const adsr[4] = { "Attack", "Decay", "Sustain", "Release" };
        for (int e = 0; e < kNumEg; ++e) for (int k = 0; k < 4; ++k) { snprintf(b, sizeof b, "EG %d %s", e + 1, adsr[k]); names[kDstEgFirst + e * 4 + k] = b; }
        for (int s = 0; s < kNumSlots; ++s) { snprintf(b, sizeof b, "Slot %d Amount", s + 1); names[kDstSlotFirst + s] = b; }
    }
    return names[d < 0 || d >= kNumDests ? 0 : d].c_str();
}

inline const ParamDef& paramDef(uint32_t i) {
    static ParamDef t[kParamCount];
    static std::string nm[kParamCount], sy[kParamCount];
    static bool done = false;
    if (!done) {
        done = true;
        auto set = [](uint32_t i, const std::string& n, const std::string& s, const char* u, uint32_t extra) {
            const Range r = paramRange(i); nm[i] = n; sy[i] = s;
            uint32_t h = extra | (r.log ? kHintLog : 0) | (r.integer ? kHintInteger : 0) | (r.min < 0 ? kHintBipolar : 0);
            t[i] = { nm[i].c_str(), sy[i].c_str(), u, r.min, r.max, h };
        };
        const uint32_t B = kHintBoolean;
        set(kNumBands, "Bands", "bands", "", 0); set(kXSlope, "Crossover Slope", "xslope", "", 0);
        for (int i = 0; i < 5; ++i) set(kX1 + i, "Crossover " + std::to_string(i + 1), "x" + std::to_string(i + 1), "Hz", 0);
        set(kChannelMode, "Channel Mode", "chmode", "", 0); set(kQuality, "High Quality", "hq", "", B); set(kAutoMute, "Auto Mute", "automute", "", B);
        set(kMix, "Mix", "mix", "", 0); set(kOutGain, "Output Gain", "outgain", "dB", 0); set(kMidiOn, "MIDI", "midion", "", B);
        static const char* const bs[kBandFields] = { "style", "drive", "dyn", "fb", "fbfreq", "bass", "mid", "treble", "pres", "mix", "level", "pan", "mute", "solo" };
        static const char* const bu[kBandFields] = { "", "", "", "", "Hz", "dB", "dB", "dB", "dB", "", "dB", "", "", "" };
        for (int b = 0; b < kMaxBands; ++b) for (int f = 0; f < kBandFields; ++f)
            set(bp(b, f), "Band " + std::to_string(b + 1) + " " + bandFieldName(f), "b" + std::to_string(b + 1) + bs[f], bu[f], (f == kBMute || f == kBSolo) ? B : 0);
        static const char* const ln[kLStep1] = { "Mode", "Wave", "Rate", "Sync", "Division", "Balance", "Glide", "Steps" };
        static const char* const ls[kLStep1] = { "mode", "wave", "rate", "sync", "div", "bal", "glide", "steps" };
        for (int l = 0; l < kNumXlfo; ++l) {
            const std::string pre = "XLFO " + std::to_string(l + 1) + " ", sp_ = "l" + std::to_string(l + 1);
            for (int f = 0; f < kLStep1; ++f) set(lp_(l, f), pre + ln[f], sp_ + ls[f], f == kLRate ? "Hz" : "", 0);
            for (int s = 0; s < kNumSteps; ++s) set(lp_(l, kLStep1 + s), pre + "Step " + std::to_string(s + 1), sp_ + "s" + std::to_string(s + 1), "", 0);
        }
        static const char* const en[kEgFields] = { "Attack", "Decay", "Sustain", "Release", "Trigger", "Threshold" };
        static const char* const es[kEgFields] = { "a", "d", "s", "r", "trig", "thr" };
        static const char* const eu[kEgFields] = { "s", "s", "", "s", "", "dB" };
        for (int e = 0; e < kNumEg; ++e) for (int f = 0; f < kEgFields; ++f) set(ep(e, f), "EG " + std::to_string(e + 1) + " " + en[f], "eg" + std::to_string(e + 1) + es[f], eu[f], 0);
        static const char* const fn[kEfFields] = { "Attack", "Release", "Gain", "Source" };
        static const char* const fs_[kEfFields] = { "a", "r", "gain", "src" };
        static const char* const fu[kEfFields] = { "s", "s", "dB", "" };
        for (int e = 0; e < kNumEf; ++e) for (int f = 0; f < kEfFields; ++f) set(fp(e, f), "EF " + std::to_string(e + 1) + " " + fn[f], "ef" + std::to_string(e + 1) + fs_[f], fu[f], 0);
        for (int x = 0; x < kNumXy; ++x) { set(xyp(x, 0), "XY " + std::to_string(x + 1) + " X", "xy" + std::to_string(x + 1) + "x", "", 0); set(xyp(x, 1), "XY " + std::to_string(x + 1) + " Y", "xy" + std::to_string(x + 1) + "y", "", 0); }
        static const char* const sn[kSlotFields] = { "Source", "Destination", "Amount", "Bypass" };
        static const char* const ss[kSlotFields] = { "src", "dst", "amt", "byp" };
        for (int s = 0; s < kNumSlots; ++s) for (int f = 0; f < kSlotFields; ++f) set(sp(s, f), "Slot " + std::to_string(s + 1) + " " + sn[f], "s" + std::to_string(s + 1) + ss[f], "", f == kSlotBypass ? B : 0);
        set(kOutLearn, "MIDI Learn", "o_learn", "", kHintOutput);
    }
    return t[i];
}

inline const char* const* enumLabels(uint32_t p, int& count) {
    static const char* styles[kNumStyles]; static bool sinit = false;
    if (!sinit) { sinit = true; for (int i = 0; i < kNumStyles; ++i) styles[i] = styleName(i); }
    static const char* const slope[] = { "12 dB/oct", "24 dB/oct", "48 dB/oct" };
    static const char* const chm[] = { "Stereo", "Mid/Side" };
    static const char* const lmode[] = { "Wave", "Steps" };
    static const char* const wave[] = { "Sine", "Triangle", "Saw Up", "Saw Down", "Square", "Random", "Sample & Hold" };
    static const char* const sync[] = { "Free", "Straight", "Dotted", "Triplet" };
    static const char* const divs[kNumDivs] = { "16 bars", "8 bars", "4 bars", "2 bars", "1 bar", "1/2", "1/4", "1/8", "1/16", "1/32", "1/64" };
    static const char* const trig[] = { "MIDI", "Audio" };
    static const char* const efs[] = { "Input", "Sidechain" };
    static const char* srcs[kNumSources]; static const char* dsts[kNumDests]; static bool init = false;
    if (!init) { init = true; for (int i = 0; i < kNumSources; ++i) srcs[i] = sourceName(i); for (int i = 0; i < kNumDests; ++i) dsts[i] = destName(i); }
    count = 0;
    if (p == kXSlope) { count = 3; return slope; }
    if (p == kChannelMode) { count = 2; return chm; }
    if (p >= kBandFirst && p < kXlfoFirst && (p - kBandFirst) % kBandFields == kBStyle) { count = kNumStyles; return styles; }
    if (p >= kXlfoFirst && p < kEgFirst) {
        const int f = (p - kXlfoFirst) % kXlfoFields;
        if (f == kLMode) { count = 2; return lmode; }
        if (f == kLWave) { count = 7; return wave; }
        if (f == kLSync) { count = 4; return sync; }
        if (f == kLDiv) { count = kNumDivs; return divs; }
    }
    if (p >= kEgFirst && p < kEfFirst && (p - kEgFirst) % kEgFields == kETrigger) { count = 2; return trig; }
    if (p >= kEfFirst && p < kXyFirst && (p - kEfFirst) % kEfFields == kFSource) { count = 2; return efs; }
    if (p >= kSlotFirst && p < kOutLearn) {
        const int f = (p - kSlotFirst) % kSlotFields;
        if (f == kSlotSrc) { count = kNumSources; return srcs; }
        if (f == kSlotDst) { count = kNumDests; return dsts; }
    }
    return nullptr;
}

inline bool isSlotAmount(uint32_t p) { return p >= kSlotFirst && p < kOutLearn && (p - kSlotFirst) % kSlotFields == kSlotAmt; }
// normalised 0..1 <-> value; slot amounts use a square-law curve so small amounts are easy to set
inline float toNorm(uint32_t p, float v) {
    if (isSlotAmount(p)) return 0.5f + 0.5f * std::copysign(std::sqrt(std::fabs(clampf(v, -1.f, 1.f))), v);
    return normOf(p, v);
}
inline float fromNorm(uint32_t p, float n) {
    if (isSlotAmount(p)) { const float u = 2.f * clampf(n, 0.f, 1.f) - 1.f; return u * std::fabs(u); }
    return valueOf(p, n);
}

} // namespace magma
