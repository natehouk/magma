// Magma 2.0 behaviour: linear-phase crossovers (exact reconstruction, band isolation per slope), 6 dB/oct
// crossovers, Superb (32x) quality and aliasing, the twelve new styles, latency of every configuration.
#include "../plugins/Magma/ParamDefs.hpp"
#include "../plugins/Magma/Presets.hpp"
#include <cstdio>
#include <memory>
#include <vector>
using namespace magma;

static int fails = 0;
#define CHECK(c, ...) do { bool ok_ = (c); char m_[400]; snprintf(m_, sizeof m_, __VA_ARGS__); printf("%s  %s\n", ok_ ? "PASS" : "FAIL", m_); if (!ok_) fails++; } while (0)
static const float fs = 48000;
typedef std::unique_ptr<Engine> EP;
static EP make() { EP e(new Engine()); return e; }
static void run(Engine& e, const std::vector<float>& l, const std::vector<float>& r, std::vector<float>& L, std::vector<float>& R, int block = 256) {
    L.assign(l.size(), 0); R.assign(l.size(), 0);
    for (size_t i = 0; i < l.size(); i += block) { const uint32_t n = (uint32_t)std::min<size_t>(block, l.size() - i);
        e.process(l.data() + i, r.data() + i, nullptr, nullptr, L.data() + i, R.data() + i, n); }
}
static std::vector<float> sine(float hz, float amp, int n) { std::vector<float> v(n); for (int i = 0; i < n; ++i) v[i] = amp * std::sin(2 * kPi * hz * i / fs); return v; }
static double rmsDb(const std::vector<float>& v, size_t a, size_t b) { double s = 0; for (size_t i = a; i < b; ++i) s += v[i] * v[i]; return 10 * std::log10(s / (b - a) + 1e-30); }
static double toneDb(const std::vector<float>& v, float hz, size_t a, size_t b) {
    double c = 0, s = 0; for (size_t i = a; i < b; ++i) { c += v[i] * std::cos(2 * kPi * hz * i / fs); s += v[i] * std::sin(2 * kPi * hz * i / fs); }
    return 10 * std::log10(2 * (c * c + s * s) / ((double)(b - a) * (b - a)) / 0.5 + 1e-30);
}
static void bands6(Engine& e) { e.p[kNumBands] = 6; const float x[5] = { 100, 400, 1500, 5000, 12000 }; for (int i = 0; i < 5; ++i) e.p[kX1 + i] = x[i]; }

int main() {
    const int N = 48000;
    // noise, band-limited (8 poles at 7 kHz) (the oversampling filters are not flat right up to Nyquist, in 1.x either)
    std::vector<float> noise(N * 2); { uint32_t r = 5; float z[8] = {}; const float k = 1 - std::exp(-2 * kPi * 7000 / fs);
        for (float& v : noise) { r = r * 1664525u + 1013904223u; float x = 0.6f * ((int32_t)r * (1.f / 2147483648.f)); for (float& q : z) { q += (x - q) * k; x = q; } v = x; } }

    // 1. linear phase: with every band at mix 0 the output is the input, delayed by exactly the reported latency
    {   double worst = -999; int bad = 0;
        for (int sl = 0; sl < 4; ++sl) for (int q = 0; q < 2; ++q) {
            EP e = make(); bands6(*e); e->p[kXSlope] = (float)sl; e->p[kXPhase] = 1; e->p[kQuality] = (float)q; for (int b = 0; b < 6; ++b) e->p[bp(b, kBMix)] = 0; e->setSampleRate(fs);
            std::vector<float> L, R; run(*e, noise, noise, L, R); const int lat = (int)e->latency();
            double err = 0, ref = 0; for (int i = N; i < 2 * N; ++i) { err += (L[i] - noise[i - lat]) * (L[i] - noise[i - lat]); ref += noise[i - lat] * noise[i - lat]; }
            const double db = 10 * std::log10(err / ref + 1e-30); worst = std::max(worst, db);
            if (lat != OversamplerN::latency(q ? 8 : 2) + 2048 + 256) ++bad; }
        CHECK(worst < -60 && bad == 0, "linear phase, 6 bands at mix 0, every slope, 2x and 8x: the input delayed by the latency (%d samples at 2x), error %.0f dB",
              OversamplerN::latency(2) + 2048 + 256, worst); }

    // 2. linear phase band isolation follows the slope: band 2 of 3 (400 Hz - 1.5 kHz), soloed
    {   double at[4]; for (int sl = 0; sl < 4; ++sl) {
            EP e = make(); e->p[kNumBands] = 3; e->p[kX1] = 400; e->p[kX2] = 1500; e->p[kXSlope] = (float)sl; e->p[kXPhase] = 1; for (int b = 0; b < 3; ++b) e->p[bp(b, kBMix)] = 0;
            e->p[bp(1, kBSolo)] = 1; e->setSampleRate(fs);
            std::vector<float> in = sine(6000, 0.2f, N), L, R; run(*e, in, in, L, R); at[sl] = rmsDb(L, N / 2, N) - rmsDb(in, N / 2, N); }
        std::vector<float> in = sine(775, 0.2f, N), L, R; EP e = make(); e->p[kNumBands] = 3; e->p[kX1] = 400; e->p[kX2] = 1500; e->p[kXSlope] = kSlope48; e->p[kXPhase] = 1;
        for (int b = 0; b < 3; ++b) { e->p[bp(b, kBMix)] = 0; } e->p[bp(1, kBSolo)] = 1; e->setSampleRate(fs); run(*e, in, in, L, R); const double pass = rmsDb(L, N / 2, N) - rmsDb(in, N / 2, N);
        CHECK(at[kSlope6] > at[kSlope12] && at[kSlope12] > at[kSlope24] && at[kSlope24] > at[kSlope48] && at[kSlope48] < -60 && std::fabs(pass) < 0.5,
              "linear phase, band 400-1500 Hz soloed: 6 kHz is %.1f / %.1f / %.1f / %.1f dB at 6/12/24/48 dB/oct; 775 Hz passes at %+.2f dB",
              at[kSlope6], at[kSlope12], at[kSlope24], at[kSlope48], pass); }

    // 3. natural phase 6 dB/oct: still sums flat, and is gentler than 12 dB/oct
    {   double worst = 0; for (float hz : { 40.f, 150.f, 700.f, 2500.f, 9000.f, 16000.f }) {
            EP e = make(); bands6(*e); e->p[kXSlope] = kSlope6; for (int b = 0; b < 6; ++b) e->p[bp(b, kBMix)] = 0; e->setSampleRate(fs);
            std::vector<float> in = sine(hz, 0.1f, N), L, R; run(*e, in, in, L, R); worst = std::max(worst, std::fabs(rmsDb(L, N / 2, N) - rmsDb(in, N / 2, N))); }
        auto iso = [&](int sl) { EP e = make(); e->p[kNumBands] = 2; e->p[kX1] = 1000; e->p[kXSlope] = (float)sl; for (int b = 0; b < 2; ++b) e->p[bp(b, kBMix)] = 0; e->p[bp(0, kBSolo)] = 1; e->setSampleRate(fs);
            std::vector<float> in = sine(4000, 0.2f, N), L, R; run(*e, in, in, L, R); return rmsDb(L, N / 2, N) - rmsDb(in, N / 2, N); };
        const double i6 = iso(kSlope6), i12 = iso(kSlope12);
        CHECK(worst < 0.05 && i6 > i12 + 3, "natural phase 6 dB/oct: 6 bands sum flat within %.3f dB; low band at 4 kHz (2 oct above 1 kHz) %.1f dB vs %.1f dB at 12 dB/oct", worst, i6, i12); }

    // 4. Superb (32x): latency, and less aliasing than Good (8x) and Normal (2x)
    {   auto alias = [&](int q, uint32_t& lat) { EP e = make(); e->p[bp(0, kBStyle)] = 11; e->p[bp(0, kBDrive)] = 1; e->p[kQuality] = q >= 1 ? 1.f : 0.f; e->p[kSuperb] = q == 2 ? 1.f : 0.f; e->setSampleRate(fs);
            std::vector<float> in = sine(9300, 0.25f, N), L, R; run(*e, in, in, L, R); lat = e->latency();
            // energy away from the harmonics of 9.3 kHz (which alias above 24 kHz) = aliasing; measured at a few non-harmonic frequencies
            double a = -999; for (float hz : { 1100.f, 3200.f, 5300.f, 7400.f, 10900.f, 14600.f }) a = std::max(a, toneDb(L, hz, N / 2, N));
            return a - toneDb(L, 9300, N / 2, N); };
        uint32_t l2, l8, l32; const double a2 = alias(0, l2), a8 = alias(1, l8), a32 = alias(2, l32);
        CHECK(l32 == 62 && l8 == 56 && l2 == 32 && a32 < a8 && a8 < a2, "hard clip of a 9.3 kHz tone: worst alias %.0f dB (2x), %.0f dB (8x), %.0f dB (32x); latency %u / %u / %u", a2, a8, a32, l2, l8, l32); }

    // 5. the new styles: level-compensated where drive is a gain, and each does its job
    {   double worst = 0; int ws = -1;
        for (int st = kNumStyles1; st < kNumStyles; ++st) { if (driveIsAmount(st)) continue;
            for (float d : { 0.f, 0.5f, 1.f }) { EP e = make(); e->p[bp(0, kBStyle)] = (float)st; e->p[bp(0, kBDrive)] = d; e->setSampleRate(fs);
                std::vector<float> in = sine(220, 0.125f, N), L, R; run(*e, in, in, L, R); const double dev = std::fabs(rmsDb(L, N / 2, N) - rmsDb(in, N / 2, N)); if (dev > worst) { worst = dev; ws = st; } } }
        CHECK(worst < 4.5, "new styles: level within %.1f dB of the input at every drive (worst: %s)", worst, styleName(ws));
        auto render = [&](int st, float d, float hz, float amp, std::vector<float>& L) { EP e = make(); e->p[bp(0, kBStyle)] = (float)st; e->p[bp(0, kBDrive)] = d; e->setSampleRate(fs);
            std::vector<float> in = sine(hz, amp, N), R; run(*e, in, in, L, R); };
        std::vector<float> L;
        // ring mod at drive 0.5: 30 * 2^3.5 = 339 Hz; 440 Hz in -> 101 and 779 Hz out, 440 gone
        render(25, 0.5f, 440, 0.25f, L); const double rm1 = toneDb(L, 779.4f, N / 2, N), rm0 = toneDb(L, 440, N / 2, N);
        CHECK(rm1 > -20 && rm0 < rm1 - 30, "Ring Mod: 440 Hz becomes sidebands (779 Hz at %.0f dB), the input frequency is %.0f dB lower", rm1, rm1 - rm0);
        render(26, 0.5f, 300, 0.25f, L); const double ou2 = toneDb(L, 600, N / 2, N), ou1 = toneDb(L, 300, N / 2, N);
        CHECK(ou2 > ou1 + 10, "Octave Up: 600 Hz is %.0f dB above 300 Hz", ou2 - ou1);
        render(27, 1.f, 440, 0.25f, L); const double so = toneDb(L, 220, N / 2, N); render(27, 0.f, 440, 0.25f, L); const double so0 = toneDb(L, 220, N / 2, N);
        CHECK(so > -25 && so > so0 + 30, "Sub Octave: 220 Hz under a 440 Hz tone at %.0f dB with drive 100%% (%.0f dB at 0%%)", so, so0);
        render(23, 1.f, 1000, 0.25f, L); double img = -999; for (float hz : { 170.f, 1340.f, 2340.f }) img = std::max(img, toneDb(L, hz, N / 2, N));
        render(23, 0.f, 1000, 0.25f, L); double img0 = -999; for (float hz : { 170.f, 1340.f, 2340.f }) img0 = std::max(img0, toneDb(L, hz, N / 2, N));
        CHECK(img > img0 + 20, "Downsample at 100%%: aliasing images %.0f dB (%.0f dB at 0%%)", img, img0);
        render(24, 0.5f, 200, 0.25f, L); double pk = 0; for (int i = N / 2; i < N; ++i) pk = std::max(pk, (double)std::fabs(L[i]));
        const double crest = 20 * std::log10(pk) - rmsDb(L, N / 2, N);
        CHECK(crest < 1.5, "Destroy: almost square (crest factor %.1f dB; a sine is 3.0)", crest);
        // the sagging styles compress: a 20 dB step in comes out smaller than the static curve alone would make it
        // sag: a loud tone starts louder than it settles (the gain sags while it works), static styles don't
        auto sag = [&](int st) { EP e = make(); e->p[bp(0, kBStyle)] = (float)st; e->p[bp(0, kBDrive)] = 0.4f; e->setSampleRate(fs);
            std::vector<float> in(N, 0.f); for (int i = N / 4; i < N; ++i) in[i] = 0.5f * std::sin(2 * kPi * 300 * i / fs);
            std::vector<float> L2, R2; run(*e, in, in, L2, R2); return rmsDb(L2, N / 4 + 200, N / 4 + 680) - rmsDb(L2, N - 4800, N); };
        const double s17 = sag(17), s19 = sag(19), s2 = sag(2), s8 = sag(8);
        CHECK(s17 > 1 && s19 > 1 && std::fabs(s2) < 0.5 && std::fabs(s8) < 0.5, "sag: a loud tone starts %.1f dB (Sagging Tube) and %.1f dB (Breakup Amp) above where it settles; %.1f dB (Hot Tube), %.1f dB (Crunch Amp)", s17, s19, s2, s8);
        render(22, 0.3f, 60, 0.25f, L); const double tLo = toneDb(L, 180, N / 2, N) - toneDb(L, 60, N / 2, N);
        render(22, 0.3f, 2000, 0.25f, L); const double tHi = toneDb(L, 6000, N / 2, N) - toneDb(L, 2000, N / 2, N);
        CHECK(tLo > tHi + 6, "Transformer: the lows distort more (3rd harmonic of 60 Hz %.0f dB, of 2 kHz %.0f dB)", tLo, tHi); }

    // 6. linear phase with a crossover swept by an LFO and 32x: finite and bounded, latency follows
    {   EP e = make(); bands6(*e); e->p[kXPhase] = 1; e->p[kSuperb] = 1; for (int b = 0; b < 6; ++b) { e->p[bp(b, kBStyle)] = (float)(b * 5 % kNumStyles); e->p[bp(b, kBDrive)] = 0.6f; }
        e->setSlot(0, kSrcXlfo1, kDstXFirst + 1, 0.5f); e->p[lp_(0, kLRate)] = 3; e->setSampleRate(fs);
        std::vector<float> L, R; run(*e, noise, noise, L, R); float pk = 0; bool fin = true; for (float v : L) { pk = std::max(pk, std::fabs(v)); if (!std::isfinite(v)) fin = false; }
        CHECK(fin && pk < 8 && e->latency() == (uint32_t)(62 + 2048 + 256), "6 bands, linear phase, 32x, crossover 2 swept by an LFO: finite, peak %.2f, latency %u", pk, e->latency()); }

    // 7. switching phase and quality while playing stays finite
    {   EP e = make(); bands6(*e); e->setSampleRate(fs); std::vector<float> L(noise.size()), R(noise.size()); bool fin = true;
        for (size_t i = 0; i < noise.size(); i += 256) {
            if (i == 256 * 30) e->p[kXPhase] = 1;
            if (i == 256 * 120) e->p[kSuperb] = 1;
            if (i == 256 * 250) { e->p[kXPhase] = 0; e->p[kNumBands] = 3; }
            e->process(noise.data() + i, noise.data() + i, nullptr, nullptr, L.data() + i, R.data() + i, (uint32_t)std::min<size_t>(256, noise.size() - i)); }
        for (float v : L) if (!std::isfinite(v) || std::fabs(v) > 8) fin = false;
        CHECK(fin && e->latency() == 62, "switching linear phase, Superb and the band count while playing: finite and bounded, latency %u", e->latency()); }

    // 8. presets
    {   int bad = 0; for (uint32_t p = 0; p < kNumPresets; ++p) for (uint32_t k = 0; k < kParamCount; ++k) { if (!isSetting(k)) continue; const ParamDef& d = paramDef(k); if (presets()[p].v[k] < d.min || presets()[p].v[k] > d.max) ++bad; }
        CHECK(bad == 0 && kNumPresets >= 55, "%u presets, %d values out of range", kNumPresets, bad); }

    printf("%s (%d failures)\n", fails ? "FAILED" : "ALL PASSED", fails);
    return fails;
}
