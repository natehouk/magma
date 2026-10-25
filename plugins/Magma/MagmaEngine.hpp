// Magma — multiband distortion engine with a 50-slot modulation matrix.
// Clean-room implementation from public, generic techniques: Linkwitz-Riley crossovers built from
// TPT state-variable filters (Zavalishin; shelving/bell responses after Simper's published SVF
// formulas) with allpass phase compensation, static and stateful waveshapers (tanh, cubic, rectifier,
// wavefolder, bit/rate reduction, granular smear), envelope followers, ADSR envelopes, step LFOs,
// windowed-sinc halfband oversampling (2x, 8x, 32x), linear-phase FIR band splitting (frequency-sampled
// Linkwitz-Riley magnitudes, partitioned FFT convolution). No third-party product code, assets or disassembly were used.
// License: MIT (see LICENSE).

#pragma once
#include "common/Halfband.hpp"
#include "common/Convolver.hpp"
#include "common/ModSystem.hpp"
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <vector>

namespace magma {

static constexpr float kPi = 3.14159265358979f;
inline float clampf(float x, float lo, float hi) { return x < lo ? lo : (x > hi ? hi : x); }
inline float dbToGain(float db) { return std::pow(10.f, db / 20.f); }
inline float gainToDb(float g) { return 20.f * std::log10(std::max(g, 1e-9f)); }

static constexpr int kMaxBands = 6, kNumStyles = 28, kNumStyles1 = 16, kNumSlots = 50;
// modulation sources, fields and the matrix are shared (epsilon-common ModSystem)
using epsilon::mod::kNumXlfo; using epsilon::mod::kNumSteps; using epsilon::mod::kNumEg; using epsilon::mod::kNumEf; using epsilon::mod::kNumXy;
static constexpr int kCtl = 16;                     // modulation and coefficients update every kCtl samples

// ------------------------------------------------------------------ params (append-only)
enum BandField { kBStyle, kBDrive, kBDynamics, kBFbAmt, kBFbFreq, kBBass, kBMid, kBTreble, kBPresence, kBMix, kBLevel, kBPan, kBMute, kBSolo, kBandFields };
using namespace epsilon::mod;     // field enums (kLRate, kEAttack, ...), Source, Xlfo, ADSR, divBars

enum Param : uint32_t {
    kNumBands, kX1, kX2, kX3, kX4, kX5, kXSlope, kChannelMode, kQuality, kAutoMute, kMix, kOutGain, kMidiOn,
    kBandFirst,
    kXlfoFirst = kBandFirst + kMaxBands * kBandFields,
    kEgFirst = kXlfoFirst + kNumXlfo * kXlfoFields,
    kEfFirst = kEgFirst + kNumEg * kEgFields,
    kXyFirst = kEfFirst + kNumEf * kEfFields,
    kSlotFirst = kXyFirst + kNumXy * 2,
    kOutLearn = kSlotFirst + kNumSlots * kSlotFields,
    // ---- 2.0
    kXPhase,        // crossover phase: Natural (minimum phase, 1.x) or Linear
    kSuperb,        // Superb quality: 32x oversampling (overrides High Quality)
    kParamCount
};
enum XSlope { kSlope12, kSlope24, kSlope48, kSlope6 };       // append-only: 6 dB/oct came in 2.0
inline uint32_t bp(int b, int f) { return kBandFirst + b * kBandFields + f; }
inline uint32_t lp_(int l, int f) { return kXlfoFirst + l * kXlfoFields + f; }
inline uint32_t ep(int e, int f) { return kEgFirst + e * kEgFields + f; }
inline uint32_t fp(int e, int f) { return kEfFirst + e * kEfFields + f; }
inline uint32_t xyp(int x, int axis) { return kXyFirst + x * 2 + axis; }
inline uint32_t sp(int s, int f) { return kSlotFirst + s * kSlotFields + f; }

// value ranges (shared with ParamDefs); log ranges are mapped logarithmically for modulation and the UI
struct Range { float min, max, def; bool log, integer; };
inline Range paramRange(uint32_t p) {
    if (p < kBandFirst) {
        static const Range g[kBandFirst] = {
            { 1, 6, 1, false, true }, { 20, 20000, 120, true, false }, { 20, 20000, 600, true, false }, { 20, 20000, 2500, true, false },
            { 20, 20000, 6000, true, false }, { 20, 20000, 12000, true, false }, { 0, 3, 1, false, true }, { 0, 1, 0, false, true },
            { 0, 1, 0, false, true }, { 0, 1, 1, false, true }, { 0, 1, 1, false, false }, { -24, 24, 0, false, false }, { 0, 1, 1, false, true } };
        return g[p];
    }
    if (p < kXlfoFirst) {
        static const Range b[kBandFields] = {
            { 0, kNumStyles - 1, 1, false, true }, { 0, 1, 0.3f, false, false }, { -1, 1, 0, false, false }, { 0, 1, 0, false, false },
            { 20, 20000, 1000, true, false }, { -12, 12, 0, false, false }, { -12, 12, 0, false, false }, { -12, 12, 0, false, false },
            { -12, 12, 0, false, false }, { 0, 1, 1, false, false }, { -24, 24, 0, false, false }, { -1, 1, 0, false, false },
            { 0, 1, 0, false, true }, { 0, 1, 0, false, true } };
        return b[(p - kBandFirst) % kBandFields];
    }
    if (p < kEgFirst) {
        const int f = (p - kXlfoFirst) % kXlfoFields;
        static const Range l[kLStep1] = { { 0, 1, 0, false, true }, { 0, 6, 0, false, true }, { 0.01f, 50, 1, true, false }, { 0, 3, 0, false, true },
            { 0, 10, 4, false, true }, { 0, 1, 0.5f, false, false }, { 0, 1, 0, false, false }, { 1, kNumSteps, 16, false, true } };
        if (f < kLStep1) return l[f];
        return { -1, 1, 0, false, false };
    }
    if (p < kEfFirst) {
        static const Range e[kEgFields] = { { 0.001f, 5, 0.01f, true, false }, { 0.001f, 5, 0.3f, true, false }, { 0, 1, 0.5f, false, false },
            { 0.001f, 10, 0.4f, true, false }, { 0, 1, 1, false, true }, { -60, 0, -30, false, false } };
        return e[(p - kEgFirst) % kEgFields];
    }
    if (p < kXyFirst) {
        static const Range f[kEfFields] = { { 0.0005f, 1, 0.005f, true, false }, { 0.005f, 5, 0.15f, true, false }, { -24, 24, 0, false, false }, { 0, 1, 0, false, true } };
        return f[(p - kEfFirst) % kEfFields];
    }
    if (p < kSlotFirst) return { 0, 1, 0.5f, false, false };
    if (p < kOutLearn) {
        static const Range s[kSlotFields] = { { 0, 17, 0, false, true }, { 0, 139, 0, false, true }, { -1, 1, 0, false, false }, { 0, 1, 0, false, true } };
        return s[(p - kSlotFirst) % kSlotFields];
    }
    if (p == kXPhase || p == kSuperb) return { 0, 1, 0, false, true };
    return { 0, 10000000, 0, false, true };
}
inline float normOf(uint32_t p, float v) {
    const Range r = paramRange(p);
    if (r.log) return std::log(clampf(v, r.min, r.max) / r.min) / std::log(r.max / r.min);
    return (clampf(v, r.min, r.max) - r.min) / (r.max - r.min);
}
inline float valueOf(uint32_t p, float n) {
    const Range r = paramRange(p); n = clampf(n, 0.f, 1.f);
    float v = r.log ? r.min * std::pow(r.max / r.min, n) : r.min + n * (r.max - r.min);
    if (r.integer) v = std::round(v);
    return v;
}

// ------------------------------------------------------------------ modulation sources and destinations
// band destinations: these band fields can be modulated, in this order
static constexpr int kBandDestFields[] = { kBDrive, kBDynamics, kBFbAmt, kBFbFreq, kBBass, kBMid, kBTreble, kBPresence, kBMix, kBLevel, kBPan };
static constexpr int kNumBandDests = sizeof(kBandDestFields) / sizeof(kBandDestFields[0]);
enum DestBase {
    kDstNone = 0,
    kDstBandFirst = 1,
    kDstXFirst = kDstBandFirst + kMaxBands * kNumBandDests,       // crossovers 1..5
    kDstMix = kDstXFirst + 5, kDstOutGain,
    kDstXlfoRateFirst, kDstXlfoBalFirst = kDstXlfoRateFirst + kNumXlfo,
    kDstEgFirst = kDstXlfoBalFirst + kNumXlfo,                     // A, D, S, R per EG
    kDstSlotFirst = kDstEgFirst + kNumEg * 4,
    kNumDests = kDstSlotFirst + kNumSlots
};
static_assert(kNumDests == 140, "slot destination range must match paramRange");
// the parameter a destination modulates (slot amounts are handled separately); -1 if none
inline int destParam(int d) {
    if (d >= kDstBandFirst && d < kDstXFirst) { const int k = d - kDstBandFirst; return (int)bp(k / kNumBandDests, kBandDestFields[k % kNumBandDests]); }
    if (d >= kDstXFirst && d < kDstXFirst + 5) return kX1 + (d - kDstXFirst);
    if (d == kDstMix) return kMix;
    if (d == kDstOutGain) return kOutGain;
    if (d >= kDstXlfoRateFirst && d < kDstXlfoBalFirst) return (int)lp_(d - kDstXlfoRateFirst, kLRate);
    if (d >= kDstXlfoBalFirst && d < kDstEgFirst) return (int)lp_(d - kDstXlfoBalFirst, kLBalance);
    if (d >= kDstEgFirst && d < kDstSlotFirst) { const int k = d - kDstEgFirst; static const int f[4] = { kEAttack, kEDecay, kESustain, kERelease }; return (int)ep(k / 4, f[k % 4]); }
    return -1;
}
inline int destForParam(uint32_t p) {
    for (int d = 1; d < kDstSlotFirst; ++d) if (destParam(d) == (int)p) return d;
    if (p >= kSlotFirst && p < kOutLearn && (p - kSlotFirst) % kSlotFields == kSlotAmt) return kDstSlotFirst + (int)((p - kSlotFirst) / kSlotFields);
    return -1;
}

// ------------------------------------------------------------------ filters
// TPT state-variable filter: v1 = band, v2 = low; responses by mixing m0*x + m1*v1 + m2*v2
struct Svf {
    float ic1 = 0, ic2 = 0;
    void reset() { ic1 = ic2 = 0; }
    inline void tick(float x, float a1, float a2, float a3, float& v1, float& v2) {
        const float v3 = x - ic2;
        v1 = a1 * ic1 + a2 * v3; v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = 2.f * v1 - ic1; ic2 = 2.f * v2 - ic2;
    }
};
struct SvfCoef {
    float a1 = 1, a2 = 0, a3 = 0, m0 = 1, m1 = 0, m2 = 0;
    void set(float g, float k) { a1 = 1.f / (1.f + g * (g + k)); a2 = g * a1; a3 = g * a2; }
    enum Type { Pass, Low, Band, Bell, LowShelf, HighShelf };
    void design(int type, float fc, float q, float db, float fs) {
        fc = clampf(fc, 10.f, 0.49f * fs); const float A = std::pow(10.f, db / 40.f), w = std::tan(kPi * fc / fs);
        switch (type) {
        case Low: set(w, 1.f / q); m0 = 0; m1 = 0; m2 = 1; break;
        case Band: set(w, 1.f / q); m0 = 0; m1 = 1.f / q; m2 = 0; break;             // unity gain at fc
        case Bell: { const float k = 1.f / (q * A); set(w, k); m0 = 1; m1 = k * (A * A - 1); m2 = 0; break; }
        case LowShelf: { const float k = 1.f / q; set(w / std::sqrt(A), k); m0 = 1; m1 = k * (A - 1); m2 = A * A - 1; break; }
        case HighShelf: { const float k = 1.f / q; set(w * std::sqrt(A), k); m0 = A * A; m1 = k * (1 - A) * A; m2 = 1 - A * A; break; }
        default: set(w, 1.4142f); m0 = 1; m1 = 0; m2 = 0; break;
        }
    }
};
inline float runSvf(Svf& s, const SvfCoef& c, float x) { float v1, v2; s.tick(x, c.a1, c.a2, c.a3, v1, v2); return c.m0 * x + c.m1 * v1 + c.m2 * v2; }

// Linkwitz-Riley crossover (12, 24 or 48 dB/oct) and the matching allpass for phase compensation
struct XoverCoef {
    int slope = 1, stages = 2; float g = 0.1f, gOne = 0.1f; float a1[4], a2[4], a3[4], k[4];
    void design(float fc, float fs, int sl) {
        slope = sl; g = std::tan(kPi * clampf(fc, 10.f, 0.45f * fs) / fs); gOne = g / (1.f + g);
        if (sl == kSlope6) { stages = 0; return; }        // first order: low = one-pole, high = the rest (sums exactly)
        static const float k2[1] = { 2.f }, k4[2] = { 1.41421356f, 1.41421356f }, k8[4] = { 1.84775907f, 0.76536686f, 1.84775907f, 0.76536686f };
        const float* kk = sl == 0 ? k2 : (sl == 1 ? k4 : k8); stages = sl == 0 ? 1 : (sl == 1 ? 2 : 4);
        for (int i = 0; i < stages; ++i) { k[i] = kk[i]; a1[i] = 1.f / (1.f + g * (g + k[i])); a2[i] = g * a1[i]; a3[i] = g * a2[i]; }
    }
};
struct Split {
    Svf lo[4], hi[4];
    void reset() { for (int i = 0; i < 4; ++i) { lo[i].reset(); hi[i].reset(); } z1 = 0; }
    float z1 = 0;
    inline void process(float x, const XoverCoef& c, float& low, float& high) {
        if (c.slope == kSlope6) { const float v = (x - z1) * c.gOne, lp = v + z1; z1 = lp + v; low = lp; high = x - lp; return; }
        float l = x, h = x, v1, v2;
        for (int i = 0; i < c.stages; ++i) { lo[i].tick(l, c.a1[i], c.a2[i], c.a3[i], v1, v2); l = v2; }
        for (int i = 0; i < c.stages; ++i) { hi[i].tick(h, c.a1[i], c.a2[i], c.a3[i], v1, v2); h = h - c.k[i] * v1 - v2; }
        low = l; high = c.slope == 0 ? -h : h;                  // LR2 needs the high band inverted to sum flat
    }
};
struct Allpass {
    Svf s[2]; float z = 0;
    void reset() { s[0].reset(); s[1].reset(); z = 0; }
    inline float process(float x, const XoverCoef& c) {
        if (c.slope == kSlope6) return x;                  // complementary first-order splits need no compensation
        if (c.slope == 0) { const float v = (x - z) * c.gOne, lp = v + z; z = lp + v; return 2.f * lp - x; }   // first-order allpass
        float v1, v2;
        if (c.slope == 1) { s[0].tick(x, c.a1[0], c.a2[0], c.a3[0], v1, v2); return x - 2.f * c.k[0] * v1; }
        s[0].tick(x, c.a1[0], c.a2[0], c.a3[0], v1, v2); const float y = x - 2.f * c.k[0] * v1;
        s[1].tick(y, c.a1[1], c.a2[1], c.a3[1], v1, v2); return y - 2.f * c.k[1] * v1;
    }
};

// ------------------------------------------------------------------ oversampling: 2x (normal), 8x (high quality), 32x (superb)
// epsilon::OversamplerN gives exactly 1.x's 2x and 8x output; latency in base-rate samples 32 / 56 / 62
using epsilon::OversamplerN;
static constexpr int kMaxOs = 32;
inline int osFactor(const float* p) { return p[kSuperb] > 0.5f ? 32 : (p[kQuality] > 0.5f ? 8 : 2); }

// ------------------------------------------------------------------ linear-phase band split
// Each band's FIR is the difference of zero-phase Linkwitz-Riley magnitudes |LP(f)| = 1 / (1 + (f/fc)^n), n = 1, 2, 4, 8
// for 6, 12, 24, 48 dB/oct, sampled on an FFT grid, made causal by half the length and windowed. The bands sum to an
// exact delay of L/2 samples.
inline int slopeOrder(int sl) { return sl == kSlope6 ? 1 : (sl == kSlope12 ? 2 : (sl == kSlope24 ? 4 : 8)); }
inline void designLinearBands(const float* xf, int nb, int sl, float fs, int L, std::vector<std::vector<float>>& h) {
    const int n = slopeOrder(sl); h.assign(nb, std::vector<float>(L, 0.f));
    epsilon::Fft f; f.init(L); std::vector<epsilon::cpx> buf(L);
    auto lp = [&](int i, float hz) { return 1.0 / (1.0 + std::pow(hz / xf[i], (double)n)); };
    for (int b = 0; b < nb; ++b) {
        for (int k = 0; k <= L / 2; ++k) {
            const float hz = (float)k * fs / L;
            const double hi = b < nb - 1 ? lp(b, hz) : 1.0, lo = b > 0 ? lp(b - 1, hz) : 0.0;
            const double m = hi - lo;
            // a linear-phase (delay L/2) response: alternate signs on the grid
            buf[k] = epsilon::cpx((float)(m * ((k & 1) ? -1.0 : 1.0)), 0.f);
            if (k > 0 && k < L / 2) buf[L - k] = buf[k];
        }
        f.run(buf.data(), true);
        for (int i = 0; i < L; ++i) {
            const double w = 0.42 - 0.5 * std::cos(2 * 3.14159265358979 * i / L) + 0.08 * std::cos(4 * 3.14159265358979 * i / L);   // Blackman
            h[b][i] = (float)(buf[i].real() * (i == L / 2 ? 1.0 : w));
        }
    }
    // the window changes each kernel a little; put the difference back into the last band so the sum stays an exact delay
    std::vector<double> sum(L, 0.0); for (int b = 0; b < nb; ++b) for (int i = 0; i < L; ++i) sum[i] += h[b][i];
    for (int i = 0; i < L; ++i) h[nb - 1][i] += (float)((i == L / 2 ? 1.0 : 0.0) - sum[i]);
}

// ------------------------------------------------------------------ distortion styles
inline const char* styleName(int s) {
    static const char* const n[kNumStyles] = { "Clean Tube", "Warm Tube", "Hot Tube", "Broken Tube", "Clean Tape", "Warm Tape", "Hot Tape",
        "Clean Amp", "Crunch Amp", "Lead Amp", "Soft Saturation", "Hard Clip", "Rectify", "Crush", "Smear", "Fold",
        // 2.0 (appended)
        "Gentle Tube", "Sagging Tube", "Dirty Tape", "Breakup Amp", "Metal Amp", "Heavy Saturation", "Transformer",
        "Downsample", "Destroy", "Ring Mod", "Octave Up", "Sub Octave" };
    return n[s < 0 ? 0 : (s >= kNumStyles ? kNumStyles - 1 : s)];
}
inline int styleCategory(int s) {
    static const int c[kNumStyles] = { 0, 0, 0, 0, 1, 1, 1, 2, 2, 2, 3, 3, 4, 4, 4, 4,  0, 0, 1, 2, 2, 3, 3, 4, 4, 4, 4, 4 };
    return c[s < 0 ? 0 : (s >= kNumStyles ? kNumStyles - 1 : s)];
}
inline const char* categoryName(int c) { static const char* const n[5] = { "Tube", "Tape", "Amp", "Saturation", "FX" }; return n[c]; }
// styles where DRIVE sets an amount (smear, rate, ring frequency, sub level) instead of the gain into the curve
inline bool driveIsAmount(int s) { return s == 14 || s == 23 || s == 25 || s == 27; }

inline float ftanh(float x) {                           // rational tanh approximation, exact at 0 and saturating at +-3
    x = x < -3.f ? -3.f : (x > 3.f ? 3.f : x); const float x2 = x * x; return x * (27.f + x2) / (27.f + 9.f * x2);
}
inline float tubeStage(float x, float b) {             // asymmetric tanh with unity small-signal slope, no DC at rest
    const float tb = ftanh(b); return (ftanh(x + b) - tb) / (1.f - tb * tb);
}
inline float cubicClip(float x) { if (x > 1.f) return 2.f / 3.f; if (x < -1.f) return -2.f / 3.f; return x - x * x * x / 3.f; }
// the memoryless core of each style (d = drive 0..1, used by Rectify); stateful parts are added in Engine::shape
inline float staticCurve(int s, float x, float d) {
    switch (s) {
    case 0: return tubeStage(x, 0.12f);
    case 1: return tubeStage(x * 1.1f, 0.35f);
    case 2: return tubeStage(1.8f * tubeStage(x * 1.4f, 0.3f), 0.2f) * 0.75f;
    case 3: { const float y = x > 0 ? ftanh(x * 1.4f) : -0.55f * ftanh(-2.2f * x); const float gap = 0.04f; return std::fabs(y) < gap ? 0.f : y - std::copysign(gap, y); }
    case 4: case 5: return cubicClip(x * 0.9f) * 1.5f;
    case 6: return ftanh(1.4f * cubicClip(x * 1.2f) * 1.5f);
    case 7: return ftanh(x + 0.1f) - ftanh(0.1f);
    case 8: return ftanh(2.5f * ftanh(x + 0.1f) + 0.15f) - ftanh(2.5f * ftanh(0.1f) + 0.15f);
    case 9: return ftanh(3.f * ftanh(2.5f * ftanh(x + 0.1f) + 0.15f));
    case 10: return x / (1.f + std::fabs(x));
    case 11: case 13: return x > 1.f ? 1.f : (x < -1.f ? -1.f : x);
    case 12: { const float y = ftanh(x), r = clampf(d * 1.4f, 0.f, 1.f); return y * (1.f - r) + (2.f * std::fabs(y) - 0.6366f) * r; }
    case 14: return ftanh(x * 0.5f) * 2.f;
    case 15: return std::sin(1.5707963f * x);
    // 2.0
    case 16: return tubeStage(x * 0.8f, 0.06f) * 1.25f;
    case 17: return tubeStage(1.6f * tubeStage(x * 1.3f, 0.25f), 0.25f) * 0.7f;
    case 18: return ftanh(1.2f * cubicClip(x * 1.1f) * 1.5f);
    case 19: return ftanh(2.f * ftanh(x + 0.1f) + 0.2f) - ftanh(2.f * ftanh(0.1f) + 0.2f);
    case 20: return ftanh(4.f * ftanh(3.f * x + 0.05f)) - ftanh(4.f * ftanh(0.05f));
    case 21: { const float a = std::fabs(x); return a < 0.7f ? x : std::copysign(0.7f + 0.3f * ftanh((a - 0.7f) / 0.3f), x); }
    case 22: return 0.5f * (ftanh(x * 1.6f) / 1.6f + ftanh(x * 0.5f) / 0.5f);       // (the stateful version saturates the lows harder)
    case 24: { const float y = clampf(x * 4.f, -1.f, 1.f); return y * (1.5f - 0.5f * y * y); }
    case 26: { const float y = ftanh(x * 1.5f); return 2.f * std::fabs(y) - 0.6366f; }
    }
    return x;
}
// level correction: the curve's RMS gain for a -18 dBFS sine at this drive, so drive changes the character, not the volume
inline float styleMakeup(int s, float g, float d) {
    const float a = 0.125f; double acc = 0, mean = 0; float y[48];
    for (int i = 0; i < 48; ++i) { y[i] = staticCurve(s, g * a * std::sin(2.f * kPi * (i + 0.5f) / 48.f), d); mean += y[i]; }
    mean /= 48; for (int i = 0; i < 48; ++i) acc += (y[i] - mean) * (y[i] - mean);
    const float gain = (float)std::sqrt(acc / 48) / (a * 0.70710678f);
    return clampf(1.f / std::max(gain, 1e-4f), 0.03f, 4.f);
}

struct StyleState {
    float lp = 0, hp = 0, prev = 0, hold = 0; int holdCount = 0;
    float env = 0, ph = 0, sub = 1, subLp = 0, lowLp = 0;           // 2.0 styles
    Svf cab, pre; float dcx = 0, dcy = 0;
    std::vector<float> smear; int sw = 0, g1Pos = 0, g2Pos = 0, g1Age = 0, g2Age = 0; uint32_t rng = 12345;
    void reset() { lp = hp = prev = hold = 0; holdCount = 0; env = ph = subLp = lowLp = 0; sub = 1; cab.reset(); pre.reset(); dcx = dcy = 0; std::fill(smear.begin(), smear.end(), 0.f); sw = 0; g1Age = 0; g2Age = 0; }
};
// per-style constants, computed per block for the band's sample rate
struct StyleCoef {
    float preLp = 1, preHp = 0, makeup = 1, dc = 0.999f;
    SvfCoef cab, pre;
    int grainLen = 1000, holdN = 1; float bits = 65536;
    float envA = 0.01f, envR = 0.001f, lowK = 0.01f, ringInc = 0.01f, subK = 0.01f;
    int calStyle = -1; float calDrive = -1;
};

// ------------------------------------------------------------------ engine
class Engine {
public:
    float p[kParamCount];
    // live values for the UI (direct access)
    float srcVal[kNumSources] = {};
    float modOff[kNumDests] = {};               // normalised offset currently applied to each destination
    static constexpr int kAna = 8192;
    float anaIn[kAna], anaOut[kAna];
    struct AtomicIndex {
        std::atomic<uint32_t> v { 0 };
        AtomicIndex() {}
        AtomicIndex(const AtomicIndex& o) : v(o.v.load()) {}
        AtomicIndex& operator=(const AtomicIndex& o) { v.store(o.v.load()); return *this; }
        uint32_t load() const { return v.load(std::memory_order_acquire); }
        void store(uint32_t x) { v.store(x, std::memory_order_release); }
    } anaWrite;
    float bandLevel[kMaxBands] = {};            // output peak per band (linear), for the display

    Engine() {
        for (uint32_t i = 0; i < kParamCount; ++i) p[i] = paramRange(i).def;
        // a few sensible defaults for the XLFO steps: a rising pattern
        for (int l = 0; l < kNumXlfo; ++l) for (int s = 0; s < kNumSteps; ++s) p[lp_(l, kLStep1 + s)] = (s % 4) / 1.5f - 1.f;
        p[kOutLearn] = 0;
        std::copy(p, p + kParamCount, effv);
        std::fill(anaIn, anaIn + kAna, 0.f); std::fill(anaOut, anaOut + kAna, 0.f);
    }
    void setSlot(int s, int src, int dst, float amt) { p[sp(s, kSlotSrc)] = (float)src; p[sp(s, kSlotDst)] = (float)dst; p[sp(s, kSlotAmt)] = amt; p[sp(s, kSlotBypass)] = 0; }

    void setSampleRate(float sr) {
        fs = sr;
        const int maxSmear = (int)(0.12f * sr * 8) + 16;
        for (auto& b : chan) for (auto& c : b) c.st.smear.assign(maxSmear, 0.f);
        // linear-phase split: FIR length about 85 ms (a power of two), processed in blocks of kLinBlock
        linLen = 1024; while (linLen < (int)(0.085f * sr)) linLen <<= 1;
        for (int c = 0; c < 2; ++c) for (int b = 0; b < kMaxBands; ++b) { conv[c][b].init(kLinBlock, linLen); linOut[c][b].assign(kLinBlock, 0.f); }
        for (int c = 0; c < 2; ++c) linIn[c].assign(kLinBlock, 0.f);
        int dl = 64; while (dl < linLen / 2 + kLinBlock + OversamplerN::latency(kMaxOs) + 8) dl <<= 1;
        dry[0].assign(dl, 0.f); dry[1].assign(dl, 0.f);
        configure();
    }
    void reset() { configure(); }
    uint32_t latency() const { return (uint32_t)(OversamplerN::latency(os) + (linear ? linLen / 2 + kLinBlock : 0)); }
    bool linearPhase() const { return linear; }
    float sampleRate() const { return fs; }

    // ---------------------------------------------------------------- MIDI and transport
    void noteOn(int note, int v) { mods.noteOn(p, layout(), note, v); }
    void noteOff(int note) { mods.noteOff(p, layout(), note); }
    void allNotesOff() { mods.allNotesOff(p, layout()); }
    void pitchBend(int v14) { mods.pitchBend(v14); }
    void modWheel(int v) { mods.modWheel(v); }
    void channelPressure(int v) { mods.channelPressure(v); }
    void polyPressure(int note, int v) { mods.polyPressure(note, v); }
    static const Layout& layout() { static const Layout L = { kXlfoFirst, kEgFirst, kEfFirst, kXyFirst, kSlotFirst, kNumSlots, kDstSlotFirst, kNumDests }; return L; }
    double xlfoPhase(int l) const { return mods.xlfo[l].phase; }
    void setTransport(bool valid, bool playing, double barPos, double bpm) { tValid = valid; tPlaying = playing; tBarPos = barPos; tBpm = bpm > 1 ? bpm : 120; }

    // effective (modulated) value of a parameter, as used by the audio thread
    float eff(uint32_t p_) const { return effv[p_]; }

    // ---------------------------------------------------------------- render
    void process(const float* inL, const float* inR, const float* scL, const float* scR, float* outL, float* outR, uint32_t n) {
        const int wantOs = osFactor(p); const bool wantLin = p[kXPhase] > 0.5f;
        if ((wantOs != os || wantLin != linear) && fadeState == 0) fadeState = 1;
        for (uint32_t i = 0; i < n; ++i) {
            if (--ctl <= 0) { ctl = kCtl; control(); }
            const bool ms = msMode;
            float x[2] = { inL[i], inR[i] };
            if (ms) { const float m = 0.5f * (x[0] + x[1]), s = 0.5f * (x[0] - x[1]); x[0] = m; x[1] = s; }
            // envelope followers and audio-triggered envelopes listen to the input (or the sidechain)
            const float aIn = std::max(std::fabs(inL[i]), std::fabs(inR[i]));
            const float aSc = (scL && scR) ? std::max(std::fabs(scL[i]), std::fabs(scR[i])) : 0.f;
            mods.follow(aIn, aSc);
            if (aIn > 1e-4f) silent = 0; else if (silent < 1 << 30) ++silent;

            // dry copy, delayed by the latency, for the global mix
            const int lat = (int)latency(), dm = (int)dry[0].size() - 1;
            float dryS[2];
            for (int c = 0; c < 2; ++c) { dry[c][dryPos] = (c ? inR[i] : inL[i]); dryS[c] = dry[c][(dryPos - lat + dm + 1) & dm]; }
            dryPos = (dryPos + 1) & dm;

            float up[2][kMaxOs], y[2][kMaxOs];
            if (!linear) {
                ovs[0].up(x[0], os, up[0]); ovs[1].up(x[1], os, up[1]);
                for (int k = 0; k < os; ++k) { osPhase = k; processOs(up[0][k], up[1][k], y[0][k], y[1][k]); }
            } else {
                // linear phase: split at the base rate with FIRs, then oversample each band
                float bup[2][kMaxBands][kMaxOs];
                for (int c = 0; c < 2; ++c) {
                    linIn[c][linPos] = x[c];
                    for (int b = 0; b < nb; ++b) bandUp[c][b].up(linOut[c][b][linPos], os, bup[c][b]);
                }
                if (++linPos == kLinBlock) {
                    linPos = 0;
                    if (linDirty && ++linAge >= 4) { designLinear(); linAge = 0; }
                    for (int c = 0; c < 2; ++c) for (int b = 0; b < kMaxBands; ++b) { if (b < nb) conv[c][b].process(linIn[c].data(), linOut[c][b].data()); else std::fill(linOut[c][b].begin(), linOut[c][b].end(), 0.f); }
                }
                float band[2][kMaxBands];
                for (int k = 0; k < os; ++k) {
                    osPhase = k;
                    for (int c = 0; c < 2; ++c) for (int b = 0; b < nb; ++b) band[c][b] = bup[c][b][k];
                    processBands(band, y[0][k], y[1][k]);
                }
            }
            float o[2] = { ovs[0].down(y[0], os), ovs[1].down(y[1], os) };
            if (ms) { const float l = o[0] + o[1], r = o[0] - o[1]; o[0] = l; o[1] = r; }
            sMix += (tMix - sMix) * smooth; sOut += (tOut - sOut) * smooth;
            for (int c = 0; c < 2; ++c) {
                float v = (dryS[c] + (o[c] - dryS[c]) * sMix) * sOut * fadeGain;
                if (!(std::fabs(v) < 64.f)) v = std::copysign(64.f, v);
                if (!std::isfinite(v)) v = 0;
                (c ? outR : outL)[i] = v;
            }
            // analyser feed
            const uint32_t w = anaWrite.load() % kAna;
            anaIn[w] = 0.5f * (inL[i] + inR[i]); anaOut[w] = 0.5f * (outL[i] + outR[i]); anaWrite.store(anaWrite.load() + 1);
            if (fadeState == 1) { fadeGain -= 1.f / 128.f; if (fadeGain <= 0) { fadeGain = 0; configure(); fadeState = 2; } }
            else if (fadeState == 2) { fadeGain += 1.f / 128.f; if (fadeGain >= 1) { fadeGain = 1; fadeState = 0; } }
        }
        for (int b = 0; b < kMaxBands; ++b) bandLevel[b] *= 0.92f;
    }

    // static band response (for the display): tone controls of one band at hz
    float toneDb(int b, float hz) const {
        float db = 0;
        auto bellDb = [](float f, float f0, float g, float q) { const float x = std::log2(f / f0) * q * 1.7f; return g / (1.f + x * x); };
        db += effv[bp(b, kBBass)] / (1.f + std::pow(hz / 120.f, 2.f));
        db += bellDb(hz, 1000.f, effv[bp(b, kBMid)], 0.8f);
        db += effv[bp(b, kBTreble)] / (1.f + std::pow(4500.f / hz, 2.f));
        const float pr = effv[bp(b, kBPresence)];
        if (pr >= 0) db += pr / (1.f + std::pow(10000.f / hz, 2.f));
        else { const float fc = presenceLp(pr); db += -10.f * std::log10(1.f + std::pow(hz / fc, 4.f)); }
        return db;
    }
    static float presenceLp(float pr) { return 20000.f * std::pow(1500.f / 20000.f, clampf(-pr / 12.f, 0.f, 1.f)); }
    int bands() const { return nb; }

private:
    struct BandChan { StyleState st; Svf tone[4], fb; Allpass ap[5]; float last = 0, dcx = 0, dcy = 0; };
    struct BandSmooth { float drive = 0, mix = 1, fb = 0, level = 1, panL = 1, panR = 1, dyn = 0, gate = 1; };

    void configure() {
        os = osFactor(p); linear = p[kXPhase] > 0.5f;
        fsOs = fs * os;
        smooth = 1.f - std::exp(-1.f / (0.01f * fs)); osSmooth = 1.f - std::exp(-1.f / (0.004f * fsOs));
        for (auto& o : ovs) o.reset();
        for (auto& c : bandUp) for (auto& o : c) o.reset();
        for (int c = 0; c < 2; ++c) { std::fill(linIn[c].begin(), linIn[c].end(), 0.f); for (int b = 0; b < kMaxBands; ++b) { conv[c][b].reset(); std::fill(linOut[c][b].begin(), linOut[c][b].end(), 0.f); } }
        linPos = 0; linAge = 0; linNb = -1;
        for (auto& c : split) for (auto& s : c) s.reset();
        for (auto& b : chan) for (auto& c : b) { c.st.reset(); for (auto& t : c.tone) t.reset(); c.fb.reset(); for (auto& a : c.ap) a.reset(); c.last = c.dcx = c.dcy = 0; }
        for (auto& d : dry) std::fill(d.begin(), d.end(), 0.f);
        dryPos = 0; ctl = 0;
        for (int b = 0; b < kMaxBands; ++b) dynEnv[b] = 0;
        mods.resetFollowers();
        control(); snapSmooth();
        if (linear) designLinear();
    }
    // (re)design the linear-phase band filters for the current crossovers
    void designLinear() {
        std::vector<std::vector<float>> h; designLinearBands(linXf, nb, linSlope, fs, linLen, h);
        for (int c = 0; c < 2; ++c) for (int b = 0; b < nb; ++b) conv[c][b].setKernel(h[b].data(), linLen);
        for (int i = 0; i < 5; ++i) linDesXf[i] = linXf[i];
        linNb = nb; linDesSlope = linSlope; linDirty = false;
    }
    void snapSmooth() {
        for (int b = 0; b < kMaxBands; ++b) { BandSmooth& s = cur[b]; const BandSmooth& t = tgt[b]; s = t; }
        sMix = tMix; sOut = tOut;
    }

    // ---- control rate: modulation sources, matrix, coefficients ------------------
    void control() {
        const float cr = fs / kCtl;
        mods.tick(p, effv, layout(), fs, cr, tValid, tPlaying, tBarPos, tBpm);
        std::copy(mods.val, mods.val + kNumSources, srcVal);
        matrix(p, layout(), srcVal, modOff);
        // effective values
        for (uint32_t i = 0; i < kParamCount; ++i) effv[i] = p[i];
        for (int d = 1; d < kDstSlotFirst; ++d) {
            if (modOff[d] == 0.f) continue;
            const int q = destParam(d); if (q < 0) continue;
            effv[q] = valueOf((uint32_t)q, normOf((uint32_t)q, p[q]) + modOff[d]);
            if (paramRange((uint32_t)q).integer) effv[q] = std::round(effv[q]);
        }
        updateCoefficients();
    }

    void updateCoefficients() {
        nb = (int)clampf(std::round(p[kNumBands]), 1, kMaxBands);
        msMode = p[kChannelMode] > 0.5f;
        const int slope = (int)clampf(std::round(p[kXSlope]), 0, 3);
        // crossovers, kept in order and at least a third of an octave apart
        float xf[5]; float lo = 20.f;
        for (int i = 0; i < nb - 1; ++i) { xf[i] = clampf(effv[kX1 + i], lo * 1.26f, 20000.f / std::pow(1.26f, (float)(nb - 2 - i))); lo = xf[i]; }
        for (int i = 0; i < nb - 1; ++i) xc[i].design(xf[i], fsOs, slope);
        if (linear) {           // linear phase: redesign when the crossovers have moved by more than 1%, or the bands or slope changed
            for (int i = 0; i < nb - 1; ++i) linXf[i] = xf[i];
            linSlope = slope;
            bool moved = nb != linNb || slope != linDesSlope;
            for (int i = 0; i < nb - 1 && !moved; ++i) moved = std::fabs(std::log(linXf[i] / std::max(linDesXf[i], 1.f))) > 0.01f;
            if (moved) linDirty = true;
            if (nb != linNb) linAge = 99;                 // a new band count can't wait
        }
        bool anySolo = false; for (int b = 0; b < nb; ++b) anySolo |= p[bp(b, kBSolo)] > 0.5f;
        for (int b = 0; b < nb; ++b) {
            BandSmooth& t = tgt[b];
            t.drive = dbToGain(effv[bp(b, kBDrive)] * 36.f);
            t.mix = clampf(effv[bp(b, kBMix)], 0.f, 1.f);
            const bool muted = p[bp(b, kBMute)] > 0.5f || (anySolo && p[bp(b, kBSolo)] < 0.5f);
            t.gate = muted ? 0.f : 1.f;
            t.level = dbToGain(effv[bp(b, kBLevel)]);
            const float pan = clampf(effv[bp(b, kBPan)], -1.f, 1.f);
            t.panL = pan > 0 ? 1.f - pan : 1.f; t.panR = pan < 0 ? 1.f + pan : 1.f;
            t.dyn = clampf(effv[bp(b, kBDynamics)], -1.f, 1.f);
            // feedback: off when auto-mute sees silence at the input for a while
            const bool mute = p[kAutoMute] > 0.5f && silent > (int)(0.25f * fs);
            t.fb = mute ? 0.f : clampf(effv[bp(b, kBFbAmt)], 0.f, 1.f);
            style[b] = (int)clampf(std::round(effv[bp(b, kBStyle)]), 0, kNumStyles - 1);
            styleCoef(b);
            // tone (after the distortion) and the feedback band-pass
            for (int k = 0; k < 3; ++k) toneOn[b][k] = std::fabs(effv[bp(b, kBBass + k)]) > 0.01f;
            toneOn[b][3] = std::fabs(effv[bp(b, kBPresence)]) > 0.01f;
            tc[b][0].design(SvfCoef::LowShelf, 120.f, 0.7f, effv[bp(b, kBBass)], fsOs);
            tc[b][1].design(SvfCoef::Bell, 1000.f, 0.8f, effv[bp(b, kBMid)], fsOs);
            tc[b][2].design(SvfCoef::HighShelf, 4500.f, 0.7f, effv[bp(b, kBTreble)], fsOs);
            const float pr = effv[bp(b, kBPresence)];
            if (pr >= 0) tc[b][3].design(SvfCoef::HighShelf, 10000.f, 0.7f, pr, fsOs);
            else tc[b][3].design(SvfCoef::Low, presenceLp(pr), 0.6f, 0, fsOs);
            fbc[b].design(SvfCoef::Band, effv[bp(b, kBFbFreq)], 4.f, 0, fsOs);
        }
        tMix = clampf(effv[kMix], 0.f, 1.f); tOut = dbToGain(effv[kOutGain]);
        dynA = 1.f - std::exp(-1.f / (0.004f * fsOs)); dynR = 1.f - std::exp(-1.f / (0.08f * fsOs));
    }
    void styleCoef(int b) {
        StyleCoef& c = sc[b]; const int s = style[b]; const float d = effv[bp(b, kBDrive)];
        auto onePole = [&](float fc) { return 1.f - std::exp(-2.f * kPi * clampf(fc, 10.f, 0.45f * fsOs) / fsOs); };
        c.preLp = 1; c.preHp = 0;
        const float g = dbToGain(d * 36.f);
        // makeup is only recomputed when style or drive moved noticeably
        if (s != c.calStyle || std::fabs(d - c.calDrive) > 0.004f) { c.makeup = styleMakeup(s, g, d); c.calStyle = s; c.calDrive = d; }
        switch (s) {
        case 2: case 3: c.preHp = onePole(30.f); break;
        case 4: c.preLp = onePole(16000.f); break;
        case 5: c.preLp = onePole(11000.f); break;
        case 6: c.preLp = onePole(8000.f); break;
        case 7: case 8: case 9:
            c.preHp = onePole(s == 7 ? 70.f : (s == 8 ? 130.f : 200.f));
            c.cab.design(SvfCoef::Low, s == 7 ? 6500.f : (s == 8 ? 5000.f : 4000.f), s == 9 ? 1.1f : 0.8f, 0, fsOs);
            c.pre.design(SvfCoef::Bell, 800.f, 0.7f, s == 9 ? 6.f : (s == 8 ? 3.f : 0.f), fsOs);
            break;
        case 13: c.bits = std::pow(2.f, 15.f - 13.f * d); c.holdN = std::max(1, (int)std::round((1.f + d * d * 30.f) * os)); break;
        case 14: c.grainLen = std::max(64, (int)(fsOs * (0.015f + 0.05f * d))); break;
        // 2.0
        case 17: c.preHp = onePole(30.f); c.envA = 1.f - std::exp(-1.f / (0.06f * fsOs)); c.envR = 1.f - std::exp(-1.f / (0.4f * fsOs)); break;
        case 18: c.preLp = onePole(6500.f); break;
        case 19:
            c.preHp = onePole(100.f); c.cab.design(SvfCoef::Low, 4500.f, 0.9f, 0, fsOs); c.pre.design(SvfCoef::Bell, 900.f, 0.7f, 4.f, fsOs);
            c.envA = 1.f - std::exp(-1.f / (0.01f * fsOs)); c.envR = 1.f - std::exp(-1.f / (0.15f * fsOs)); break;
        case 20: c.preHp = onePole(250.f); c.cab.design(SvfCoef::Low, 3800.f, 1.2f, 0, fsOs); c.pre.design(SvfCoef::Bell, 1200.f, 0.8f, 8.f, fsOs); break;
        case 22: c.lowK = onePole(180.f); break;
        case 23: c.holdN = std::max(1, (int)std::round((1.f + d * d * 40.f) * os)); break;
        case 25: c.ringInc = 30.f * std::exp2(7.f * clampf(d, 0.f, 1.f)) / fsOs; break;
        case 27: c.subK = onePole(600.f); c.envA = 1.f - std::exp(-1.f / (0.005f * fsOs)); c.envR = 1.f - std::exp(-1.f / (0.08f * fsOs)); break;
        }
        c.dc = std::exp(-2.f * kPi * 12.f / fsOs);
    }

    // one oversampled sample, both channels
    inline void processOs(float xl, float xr, float& yl, float& yr) {
        // split into bands
        float band[2][kMaxBands];
        for (int c = 0; c < 2; ++c) {
            float rest = c ? xr : xl;
            for (int i = 0; i < nb - 1; ++i) { float lo, hi; split[c][i].process(rest, xc[i], lo, hi); band[c][i] = lo; rest = hi; }
            band[c][nb - 1] = rest;
            for (int b = 0; b < nb - 1; ++b) for (int j = b + 1; j < nb - 1; ++j) band[c][b] = chan[b][c].ap[j].process(band[c][b], xc[j]);
        }
        processBands(band, yl, yr);
    }
    inline void processBands(float (&band)[2][kMaxBands], float& yl, float& yr) {
        float out[2] = { 0, 0 };
        for (int b = 0; b < nb; ++b) {
            BandSmooth& s = cur[b]; const BandSmooth& t = tgt[b];
            s.drive += (t.drive - s.drive) * osSmooth; s.mix += (t.mix - s.mix) * osSmooth; s.fb += (t.fb - s.fb) * osSmooth;
            s.level += (t.level - s.level) * osSmooth; s.panL += (t.panL - s.panL) * osSmooth; s.panR += (t.panR - s.panR) * osSmooth;
            s.dyn += (t.dyn - s.dyn) * osSmooth; s.gate += (t.gate - s.gate) * osSmooth;
            // dynamics: compress (+) or expand (-) around -18 dBFS, before the distortion
            const float a = std::max(std::fabs(band[0][b]), std::fabs(band[1][b]));
            dynEnv[b] += (a > dynEnv[b] ? dynA : dynR) * (a - dynEnv[b]);
            if (osPhase == 0) dynGain[b] = std::fabs(s.dyn) > 0.001f ? dbToGain(clampf(-s.dyn * 0.6f * (gainToDb(dynEnv[b] + 1e-6f) + 18.f), -24.f, 18.f)) : 1.f;
            float pk = 0;
            for (int c = 0; c < 2; ++c) {
                BandChan& ch = chan[b][c];
                const float dryB = band[c][b];
                float v = dryB * dynGain[b];
                // feedback through a resonant band-pass, softly limited so it cannot run away
                if (s.fb > 0.0005f) v += 3.f * ftanh(runSvf(ch.fb, fbc[b], ch.last) * s.fb * 1.6f / 3.f);
                float w = driveIsAmount(style[b]) ? shape(b, c, v) : shape(b, c, v * s.drive) * sc[b].makeup;   // Smear etc.: drive sets an amount, not gain
                // DC blocker (asymmetric styles create DC)
                const float dcOut = w - ch.dcx + sc[b].dc * ch.dcy; ch.dcx = w; ch.dcy = dcOut; w = dcOut;
                for (int k = 0; k < 4; ++k) if (toneOn[b][k]) w = runSvf(ch.tone[k], tc[b][k], w); else ch.tone[k].reset();
                ch.last = w;
                float y = dryB + (w - dryB) * s.mix;
                y *= s.level * s.gate * (c ? s.panR : s.panL);
                out[c] += y; pk = std::max(pk, std::fabs(y));
            }
            bandLevel[b] = std::max(bandLevel[b], pk);
        }
        yl = out[0]; yr = out[1];
    }

    inline float shape(int b, int c, float x) {
        StyleState& st = chan[b][c].st; const StyleCoef& k = sc[b];
        if (k.preHp > 0) { st.hp += (x - st.hp) * k.preHp; x -= st.hp; }
        if (k.preLp < 1) { st.lp += (x - st.lp) * k.preLp; x = st.lp; }
        switch (style[b]) {
        case 5: { const float y = staticCurve(5, x + 0.12f * (x - st.prev), 0); st.prev = x; return y; }     // a little "head" memory
        case 7: case 8: case 9: return runSvf(st.cab, k.cab, staticCurve(style[b], runSvf(st.pre, k.pre, x), 0));
        case 13: {                                                     // bit and sample-rate reduction
            if (--st.holdCount <= 0) { st.holdCount = k.holdN; st.hold = std::round(clampf(x, -1.f, 1.f) * k.bits) / k.bits; }
            return st.hold; }
        case 14: {                                                     // smear: overlapping grains replayed from the recent past, fed back
            std::vector<float>& buf = st.smear; const int N = (int)buf.size(); if (N < 4) return x;
            const int L = std::min(k.grainLen, N / 3);
            auto grain = [&](int& pos, int& age) {
                if (age >= L || age <= 0) { st.rng = st.rng * 1664525u + 1013904223u; pos = (st.sw - L - (int)((st.rng >> 9) % (unsigned)std::max(1, L)) + 4 * N) % N; age = 0; }
                const float t = (float)age / L, wv = 0.5f - 0.5f * std::cos(2.f * kPi * t);
                const float s = buf[(pos + age) % N] * wv; ++age; return s; };
            if (st.g2Age == 0 && st.g1Age == L / 2) st.g2Age = L;            // start the second grain half a grain later
            const float gsum = grain(st.g1Pos, st.g1Age) + (st.g2Age ? grain(st.g2Pos, st.g2Age) : 0.f);
            const float d = clampf(effv[bp(b, kBDrive)], 0.f, 1.f);
            buf[st.sw] = ftanh(x * 0.5f + gsum * 0.55f * d); st.sw = (st.sw + 1) % N;
            // overlapping grains partly cancel: measured level loss, made up here (dB at drive 0, .25, .5, .75, 1)
            static const float loss[5] = { 0.1f, 1.3f, 1.75f, 4.5f, 6.2f };
            const float pos = d * 4.f; const int i0 = std::min(3, (int)pos); const float comp = dbToGain(loss[i0] + (loss[i0 + 1] - loss[i0]) * (pos - i0));
            return (2.f * ftanh(x * 0.5f) * (1.f - 0.7f * d) + gsum * (0.4f + 0.6f * d)) * comp;
        }
        // 2.0
        case 17: {                                                     // the bias shifts with the level: the tube "sags" on loud passages
            const float a = std::fabs(x); st.env += (a > st.env ? k.envA : k.envR) * (a - st.env);
            const float e = std::min(st.env, 1.f);
            return tubeStage(1.6f * tubeStage(x * 1.3f / (1.f + 0.8f * e), 0.25f + 0.3f * e), 0.25f) * 0.7f / (1.f + 0.35f * e); }
        case 18: { const float y = staticCurve(18, x + 0.2f * (x - st.prev), 0); st.prev = x; return y; }
        case 19: {                                                     // power-supply sag: the gain drops while it works hard
            const float a = std::fabs(x); st.env += (a > st.env ? k.envA : k.envR) * (a - st.env);
            return runSvf(st.cab, k.cab, staticCurve(19, runSvf(st.pre, k.pre, x) / (1.f + 1.5f * st.env), 0)); }
        case 20: return runSvf(st.cab, k.cab, staticCurve(20, runSvf(st.pre, k.pre, x), 0));
        case 22: {                                                     // the lows saturate first, as in an iron core
            st.lowLp += (x - st.lowLp) * k.lowK; const float lo = st.lowLp, hi = x - lo;
            return ftanh(lo * 1.6f) / 1.6f + ftanh(hi * 0.5f) / 0.5f; }
        case 23: if (--st.holdCount <= 0) { st.holdCount = k.holdN; st.hold = x; } return st.hold;
        case 25: { st.ph += k.ringInc; if (st.ph >= 1.f) st.ph -= 1.f; return x * std::sin(2.f * kPi * st.ph) * 1.41421356f; }
        case 27: {                                                     // a flip-flop divider: a square an octave down, following the input's level
            st.subLp += (x - st.subLp) * k.subK;
            if (st.prev <= 0.f && st.subLp > 0.f) st.sub = -st.sub;
            st.prev = st.subLp;
            const float a = std::fabs(x); st.env += (a > st.env ? k.envA : k.envR) * (a - st.env);
            const float d = clampf(effv[bp(b, kBDrive)], 0.f, 1.f);
            return (ftanh(x) + 1.2f * d * st.sub * st.env) / (1.f + 0.5f * d); }
        default: return staticCurve(style[b], x, effv[bp(b, kBDrive)]);
        }
    }

    float fs = 48000.f, fsOs = 96000.f, smooth = 0.01f, osSmooth = 0.01f;
    int os = 2, nb = 1, ctl = 0, fadeState = 0; float fadeGain = 1; bool msMode = false;
    OversamplerN ovs[2];
    // linear-phase split
    static constexpr int kLinBlock = 256;
    bool linear = false, linDirty = false; int linLen = 4096, linPos = 0, linAge = 0, linNb = -1, linSlope = 1, linDesSlope = -1;
    float linXf[5] = { 120, 600, 2500, 6000, 12000 }, linDesXf[5] = {};
    epsilon::Convolver conv[2][kMaxBands]; std::vector<float> linIn[2], linOut[2][kMaxBands];
    OversamplerN bandUp[2][kMaxBands];
    Split split[2][kMaxBands - 1];
    XoverCoef xc[kMaxBands - 1];
    BandChan chan[kMaxBands][2];
    SvfCoef tc[kMaxBands][4], fbc[kMaxBands]; bool toneOn[kMaxBands][4] = {}; float dynGain[kMaxBands] = { 1, 1, 1, 1, 1, 1 }; int osPhase = 0;
    StyleCoef sc[kMaxBands]; int style[kMaxBands] = {};
    BandSmooth cur[kMaxBands], tgt[kMaxBands];
    float dynEnv[kMaxBands] = {}, dynA = 0.01f, dynR = 0.001f;
    float effv[kParamCount];
    std::vector<float> dry[2]; int dryPos = 0;
    float sMix = 1, tMix = 1, sOut = 1, tOut = 1;
    // sources
    Sources mods; int silent = 0;
    bool tValid = false, tPlaying = false; double tBarPos = 0, tBpm = 120;
};

} // namespace magma
