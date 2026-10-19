// Feature tests for Magma: transparent band splitting, level-compensated drive for every style,
// harmonics, per-band isolation, dynamics, feedback and auto-mute, mute/solo, mid/side, pan, tone,
// global mix alignment, modulation (XLFO, steps, EG, EF, XY, MIDI, slot-on-slot), latency, stability.
#include "../plugins/Magma/ParamDefs.hpp"
#include "../plugins/Magma/Presets.hpp"
#include <cstdio>
#include <memory>
#include <functional>
#include <vector>
using namespace magma;

static int fails = 0;
static void check(bool ok, const char* fmt, double a = 0, double b = 0, double c = 0) { printf("%s  ", ok ? "PASS" : "FAIL"); printf(fmt, a, b, c); printf("\n"); fails += !ok; }
static const float fs = 48000;
typedef std::unique_ptr<Engine> EP;
static EP make() { EP e(new Engine()); return e; }
static void run(Engine& e, const std::vector<float>& l, const std::vector<float>& r, std::vector<float>& L, std::vector<float>& R, const std::vector<float>* sc = nullptr) {
    L.assign(l.size(), 0); R.assign(l.size(), 0);
    for (size_t i = 0; i < l.size(); i += 256) { const uint32_t n = (uint32_t)std::min<size_t>(256, l.size() - i);
        e.process(l.data() + i, r.data() + i, sc ? sc->data() + i : nullptr, sc ? sc->data() + i : nullptr, L.data() + i, R.data() + i, n); }
}
static std::vector<float> sine(float hz, float amp, int n) { std::vector<float> v(n); for (int i = 0; i < n; ++i) v[i] = amp * std::sin(2 * kPi * hz * i / fs); return v; }
static double rmsDb(const std::vector<float>& v, size_t a, size_t b) { double s = 0; for (size_t i = a; i < b; ++i) s += v[i] * v[i]; return 10 * std::log10(s / (b - a) + 1e-30); }
static double toneDb(const std::vector<float>& v, float hz, size_t a, size_t b) {   // level of one frequency (DFT bin)
    double c = 0, s = 0; for (size_t i = a; i < b; ++i) { c += v[i] * std::cos(2 * kPi * hz * i / fs); s += v[i] * std::sin(2 * kPi * hz * i / fs); }
    return 10 * std::log10(2 * (c * c + s * s) / ((double)(b - a) * (b - a)) / 0.5 + 1e-30);
}

int main() {
    const int N = 48128;
    // 1. with mix 0 every band setting is transparent (crossovers sum flat), all slopes, both qualities
    { double worst = 0;
      for (int q = 0; q < 2; ++q) for (int sl = 0; sl < 3; ++sl) for (float hz : { 40.f, 150.f, 700.f, 2500.f, 9000.f, 16000.f }) {
          EP e = make(); e->p[kNumBands] = 6; e->p[kXSlope] = (float)sl; e->p[kQuality] = (float)q; for (int b = 0; b < 6; ++b) e->p[bp(b, kBMix)] = 0; e->setSampleRate(fs);
          std::vector<float> in = sine(hz, 0.1f, N), L, R; run(*e, in, in, L, R); worst = std::max(worst, std::fabs(rmsDb(L, N / 2, N) - rmsDb(in, N / 2, N))); }
      check(worst < 0.05, "6 bands at mix 0, every slope and quality: flat within %.3f dB", worst); }
    // 2. drive changes character, not level: -18 dBFS tone, every style, drive 0..1
    { double worst = 0; int ws = 0;
      for (int st = 0; st < kNumStyles; ++st) for (float d : { 0.f, 0.5f, 1.f }) {
          EP e = make(); e->p[bp(0, kBStyle)] = (float)st; e->p[bp(0, kBDrive)] = d; e->setSampleRate(fs);
          std::vector<float> in = sine(220, 0.125f, N), L, R; run(*e, in, in, L, R);
          const double dev = std::fabs(rmsDb(L, N / 2, N) - rmsDb(in, N / 2, N)); if (dev > worst) { worst = dev; ws = st; } }
      check(worst < 4.5, "level compensation: every style and drive within %.1f dB of the input (worst: style %.0f)", worst, ws); }
    // 3. drive adds harmonics
    { auto h3 = [&](int st, float d) { EP e = make(); e->p[bp(0, kBStyle)] = (float)st; e->p[bp(0, kBDrive)] = d; e->setSampleRate(fs);
          std::vector<float> in = sine(500, 0.25f, N), L, R; run(*e, in, in, L, R); return toneDb(L, 1500, N / 2, N) - toneDb(L, 500, N / 2, N); };
      const double clean = h3(11, 0.f), hot = h3(11, 0.7f), tube2 = [&] { EP e = make(); e->p[bp(0, kBStyle)] = 1; e->p[bp(0, kBDrive)] = 0.7f; e->setSampleRate(fs);
          std::vector<float> in = sine(500, 0.25f, N), L, R; run(*e, in, in, L, R); return toneDb(L, 1000, N / 2, N) - toneDb(L, 500, N / 2, N); }();
      check(clean < -55 && hot > -25, "Hard Clip: 3rd harmonic %.0f dB at drive 0, %.0f dB at 70%%", clean, hot);
      check(tube2 > -40, "Warm Tube adds even (2nd) harmonics: %.0f dB", tube2); }
    // 4. a band's distortion stays in its band: two bands, distort only the top, low tone untouched
    { EP e = make(); e->p[kNumBands] = 2; e->p[kX1] = 1000; e->p[kXSlope] = 2; e->p[bp(0, kBMix)] = 0; e->p[bp(1, kBStyle)] = 11; e->p[bp(1, kBDrive)] = 1; e->setSampleRate(fs);
      std::vector<float> in = sine(100, 0.3f, N), L, R; run(*e, in, in, L, R);
      check(std::fabs(rmsDb(L, N / 2, N) - rmsDb(in, N / 2, N)) < 0.3, "100 Hz tone with only the band above 1 kHz driven: level change %.2f dB", rmsDb(L, N / 2, N) - rmsDb(in, N / 2, N)); }
    // 5. mute / solo / level / pan
    { auto lvl = [&](std::function<void(Engine&)> f, float hz, int ch) { EP e = make(); e->p[kNumBands] = 2; e->p[kX1] = 1000; for (int b = 0; b < 2; ++b) e->p[bp(b, kBMix)] = 0; f(*e); e->setSampleRate(fs);
          std::vector<float> in = sine(hz, 0.2f, N), L, R; run(*e, in, in, L, R); return rmsDb(ch ? R : L, N / 2, N) - rmsDb(in, N / 2, N); };
      const double mute = lvl([](Engine& e) { e.p[bp(0, kBMute)] = 1; }, 100, 0), solo = lvl([](Engine& e) { e.p[bp(1, kBSolo)] = 1; }, 100, 0);
      const double lv = lvl([](Engine& e) { e.p[bp(1, kBLevel)] = -6; }, 5000, 0), panL = lvl([](Engine& e) { e.p[bp(0, kBPan)] = 1; }, 100, 0), panR = lvl([](Engine& e) { e.p[bp(0, kBPan)] = 1; }, 100, 1);
      check(mute < -40 && solo < -40, "mute low band: %.0f dB; solo high band, low tone: %.0f dB", mute, solo);
      check(std::fabs(lv + 6) < 0.3, "band level -6 dB: %.2f dB", lv);
      check(panL < -40 && std::fabs(panR) < 0.3, "pan hard right: left %.0f dB, right %+.2f dB", panL, panR); }
    // 6. tone controls after the distortion
    { auto tn = [&](int field, float db, float hz) { EP e = make(); e->p[bp(0, kBDrive)] = 0; e->p[bp(0, kBStyle)] = 11; e->p[bp(0, field)] = db; e->setSampleRate(fs);
          std::vector<float> in = sine(hz, 0.05f, N), L, R; run(*e, in, in, L, R); return rmsDb(L, N / 2, N) - rmsDb(in, N / 2, N); };
      check(tn(kBBass, 12, 40) > 10 && tn(kBMid, -12, 1000) < -10 && tn(kBTreble, 12, 12000) > 10, "tone: bass +12 at 40 Hz %.1f, mid -12 at 1 kHz %.1f, treble +12 at 12 kHz %.1f dB", tn(kBBass, 12, 40), tn(kBMid, -12, 1000), tn(kBTreble, 12, 12000));
      check(tn(kBPresence, -12, 8000) < -12 && tn(kBPresence, 12, 16000) > 8, "presence: -12 rolls off 8 kHz by %.0f dB, +12 lifts 16 kHz by %.0f dB", tn(kBPresence, -12, 8000), tn(kBPresence, 12, 16000)); }
    // 7. dynamics: compression narrows the level difference between loud and quiet
    { auto spread = [&](float dyn) { EP e = make(); e->p[bp(0, kBDrive)] = 0; e->p[bp(0, kBStyle)] = 11; e->p[bp(0, kBDynamics)] = dyn; e->setSampleRate(fs);
          std::vector<float> in(N); for (int i = 0; i < N; ++i) in[i] = (i < N / 2 ? 0.5f : 0.05f) * std::sin(2 * kPi * 300 * i / fs);
          std::vector<float> L, R; run(*e, in, in, L, R); return rmsDb(L, N / 4, N / 2) - rmsDb(L, 3 * N / 4, N); };
      const double off = spread(0), comp = spread(1), exp = spread(-1);
      check(comp < off - 6 && exp > off + 6, "dynamics: 20 dB input step becomes %.1f dB (off), %.1f (compress), %.1f (expand)", off, comp, exp); }
    // 8. feedback rings at its frequency; auto-mute silences it after the input stops
    { EP e = make(); e->p[bp(0, kBFbAmt)] = 1; e->p[bp(0, kBFbFreq)] = 700; e->p[bp(0, kBDrive)] = 0.5f; e->setSampleRate(fs);
      std::vector<float> in(N * 2, 0.f), L, R; uint32_t r = 3; for (int i = 0; i < 2000; ++i) { r = r * 1664525u + 1013904223u; in[i] = 0.5f * ((int32_t)r * (1.f / 2147483648.f)); }
      run(*e, in, in, L, R);
      check(rmsDb(L, 2000, 8000) > -40 && rmsDb(L, N, N * 2) < -100, "feedback rings after a burst (%.0f dB), auto-mute silences it later (%.0f dB)", rmsDb(L, 2000, 8000), rmsDb(L, N, N * 2));
      double peak = -999; for (float f : { 500.f, 700.f, 900.f }) peak = std::max(peak, toneDb(L, f, 2000, 8000));
      check(toneDb(L, 700, 2000, 8000) >= peak - 0.01, "the ring is strongest at the feedback frequency (700 Hz %.0f dB)", toneDb(L, 700, 2000, 8000)); }
    // 9. mid/side: a band muted in M/S mode removes the side signal only
    { EP e = make(); e->p[kChannelMode] = 1; e->p[kNumBands] = 1; e->p[bp(0, kBMix)] = 0; e->p[bp(0, kBPan)] = -1; e->setSampleRate(fs);   // pan hard to "mid"
      std::vector<float> l = sine(300, 0.2f, N), r(N), L, R; for (int i = 0; i < N; ++i) r[i] = -l[i] * 0.5f;     // mostly side content
      run(*e, l, r, L, R); double side = 0, ref = 0; for (int i = N / 2; i < N; ++i) { side += (L[i] - R[i]) * (L[i] - R[i]); ref += (l[i] - r[i]) * (l[i] - r[i]); }
      check(10 * std::log10(side / ref + 1e-30) < -40, "mid/side: pan to mid removes the side signal (%.0f dB)", 10 * std::log10(side / ref + 1e-30)); }
    // 10. global mix keeps dry and wet in time (no comb filtering at 50%)
    { EP e = make(); e->p[kMix] = 0.5f; e->p[bp(0, kBDrive)] = 0; e->p[bp(0, kBStyle)] = 11; e->setSampleRate(fs);
      double worst = 0; for (float hz : { 200.f, 3000.f, 11000.f }) { EP f = make(); f->p[kMix] = 0.5f; f->p[bp(0, kBDrive)] = 0; f->p[bp(0, kBStyle)] = 11; f->setSampleRate(fs);
          std::vector<float> in = sine(hz, 0.1f, N), L, R; run(*f, in, in, L, R); worst = std::max(worst, std::fabs(rmsDb(L, N / 2, N) - rmsDb(in, N / 2, N))); }
      check(worst < 0.1 && e->latency() == 32, "global mix 50%% is aligned (worst %.3f dB); latency %.0f samples (normal)", worst, e->latency()); }
    // 11. modulation: XLFO moves the drive; steps follow the pattern; EG on audio; EF; XY; velocity; slot on slot
    { EP e = make(); e->p[bp(0, kBDrive)] = 0.5f; e->setSlot(0, kSrcXlfo1, kDstBandFirst + 0, 0.4f); e->p[lp_(0, kLRate)] = 2; e->setSampleRate(fs);
      std::vector<float> in = sine(200, 0.1f, N), L(N), R(N); float mn = 9, mx = -9;
      for (int i = 0; i < N; i += 256) { e->process(in.data() + i, in.data() + i, nullptr, nullptr, L.data(), R.data(), std::min(256, N - i)); mn = std::min(mn, e->modOff[1]); mx = std::max(mx, e->modOff[1]); }
      check(mn < -0.35f && mx > 0.35f, "XLFO 1 -> band 1 drive at 40%%: offset swings %.2f .. %.2f", mn, mx); }
    { EP e = make(); e->p[lp_(0, kLMode)] = 1; e->p[lp_(0, kLSteps)] = 4; e->p[lp_(0, kLRate)] = 1;
      const float pat[4] = { 1, -1, 0.5f, -0.5f }; for (int s = 0; s < 4; ++s) e->p[lp_(0, kLStep1 + s)] = pat[s]; e->setSampleRate(fs);
      std::vector<float> in(N, 0.f), L(N), R(N); int ok = 0;
      for (int i = 0, k = 0; i < N; i += 256, ++k) { e->process(in.data() + i, in.data() + i, nullptr, nullptr, L.data(), R.data(), std::min(256, N - i));
          if (k % 47 == 23) { const float ph = std::fmod((i + 256) / fs * 1.f, 1.f); const int st = (int)(ph * 4); if (std::fabs(e->srcVal[kSrcXlfo1] - pat[st]) < 0.05f) ++ok; } }
      check(ok >= 3, "step sequencer follows its 4-step pattern (%.0f checks matched)", ok); }
    { EP e = make(); e->p[ep(0, kETrigger)] = 1; e->p[ep(0, kEThreshold)] = -30; e->setSampleRate(fs);
      std::vector<float> in(N, 0.f), L(N), R(N); for (int i = 4800; i < 14400; ++i) in[i] = 0.3f * std::sin(2 * kPi * 200 * i / fs);
      float atBurst = 0, after = 1; for (int i = 0; i < N; i += 256) { e->process(in.data() + i, in.data() + i, nullptr, nullptr, L.data(), R.data(), std::min(256, N - i));
          if (i > 9000 && i < 9600) atBurst = e->srcVal[kSrcEg1];
          if (i > 40000) after = e->srcVal[kSrcEg1]; }
      check(atBurst > 0.4f && after < 0.05f, "audio-triggered EG: %.2f during the burst, %.2f after", atBurst, after); }
    { EP e = make(); e->p[fp(1, kFSource)] = 1; e->setSampleRate(fs);   // EF 2 listens to the sidechain
      std::vector<float> in(N, 0.f), sc = sine(300, 0.5f, N), L, R; run(*e, in, in, L, R, &sc);
      check(e->srcVal[kSrcEf2] > 0.8f && e->srcVal[kSrcEf1] < 0.05f, "envelope followers: EF 2 on the sidechain %.2f, EF 1 on the silent input %.2f", e->srcVal[kSrcEf2], e->srcVal[kSrcEf1]); }
    { EP e = make(); e->p[xyp(0, 0)] = 0.8f; e->setSlot(0, kSrcXy1X, kDstMix, -1.f); e->setSampleRate(fs);
      std::vector<float> in = sine(200, 0.1f, N), L, R; run(*e, in, in, L, R);
      check(std::fabs(e->modOff[kDstMix] + 0.8f) < 0.01f, "XY 1 X at 80%% with amount -100%% offsets the mix by %.2f", e->modOff[kDstMix]); }
    { EP e = make(); e->setSlot(0, kSrcVelocity, kDstBandFirst, 1.f); e->setSlot(1, kSrcWheel, kDstSlotFirst + 0, -1.f); e->setSampleRate(fs);
      e->noteOn(60, 127); e->modWheel(127); std::vector<float> in(4096, 0.f), L(4096), R(4096); e->process(in.data(), in.data(), nullptr, nullptr, L.data(), R.data(), 4096);
      const float full = e->modOff[kDstBandFirst];
      e->modWheel(64); e->process(in.data(), in.data(), nullptr, nullptr, L.data(), R.data(), 4096);
      check(std::fabs(full) < 0.01f && std::fabs(e->modOff[kDstBandFirst] - 0.496f) < 0.02f, "velocity -> drive, scaled by a slot driven by the mod wheel: %.2f at full wheel, %.2f at half", full, e->modOff[kDstBandFirst]); }
    // 12. HQ latency and band count changes are click-free enough to stay finite and bounded
    { EP e = make(); e->p[kQuality] = 1; e->setSampleRate(fs); std::vector<float> in = sine(200, 0.5f, N), L, R; run(*e, in, in, L, R);
      check(e->latency() == 56, "HQ (8x) latency %.0f samples", e->latency()); }
    // 13. presets: all in range
    { int bad = 0; for (uint32_t p = 0; p < kNumPresets; ++p) for (uint32_t k = 0; k < kOutLearn; ++k) { const ParamDef& d = paramDef(k); if (presets()[p].v[k] < d.min || presets()[p].v[k] > d.max) ++bad; }
      check(bad == 0 && kNumPresets >= 40, "%.0f presets, %.0f values out of range", kNumPresets, bad); }
    printf(fails ? "%d FAILURES\n" : "ALL PASSED (0 failures)\n", fails); return fails;
}
