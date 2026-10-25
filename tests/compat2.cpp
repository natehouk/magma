// 2.0 refactor guard: random 2.0 settings (all styles, slopes, phases, qualities, every modulation source,
// MIDI and the sidechain) through the frozen 2.0.0 engine and the current one: outputs must be identical.
#include "../plugins/Magma/ParamDefs.hpp"
#include "legacy/MagmaEngine2.hpp"
#include <cstdio>
#include <memory>
#include <random>
#include <vector>
using namespace magma;
int main() {
    std::mt19937 g(77); std::uniform_real_distribution<float> u(0, 1);
    int bad = 0; float worst = -1000;
    for (int trial = 0; trial < 24; ++trial) {
        const int sr = trial % 3 == 0 ? 44100 : (trial % 3 == 1 ? 48000 : 96000);
        std::unique_ptr<Engine> e(new Engine()); std::unique_ptr<magma2::Engine> o(new magma2::Engine());
        for (uint32_t i = 0; i < kParamCount; ++i) {
            if (!isSetting(i)) continue;
            const Range r = paramRange(i); float v = r.log ? r.min * std::pow(r.max / r.min, u(g)) : r.min + (r.max - r.min) * u(g);
            if (r.integer) v = std::round(v);
            if (i >= kSlotFirst && i < kOutLearn && (i - kSlotFirst) % kSlotFields == kSlotSrc && u(g) < 0.3f) v = 0;
            if (i == kOutGain) v = std::min(v, 6.f);
            if (i == kSuperb) v = trial % 6 == 5 ? 1.f : 0.f;           // keep the run time sensible
            if (i == kNumBands && trial % 6 == 5) v = std::min(v, 3.f);
            e->p[i] = v; o->p[i] = v;
        }
        e->setSampleRate((float)sr); o->setSampleRate((float)sr);
        const int N = sr / 2; std::vector<float> l(N), r(N), k(N), L1(N), R1(N), L2(N), R2(N);
        for (int i = 0; i < N; ++i) { const float env = (i / (sr / 7)) % 2 ? 0.6f : 0.02f; l[i] = env * (2 * u(g) - 1); r[i] = 0.7f * l[i] + 0.2f * env * (2 * u(g) - 1); k[i] = 0.4f * std::sin(0.003f * i) * ((i / 3000) % 2); }
        for (int i = 0; i < N; i += 256) {
            const int n = std::min(256, N - i);
            const float ev = u(g);
            if (ev < 0.05f) { const int nt = 36 + (int)(u(g) * 48), vv = 1 + (int)(u(g) * 126); e->noteOn(nt, vv); o->noteOn(nt, vv); }
            else if (ev < 0.08f) { e->allNotesOff(); o->allNotesOff(); }
            else if (ev < 0.11f) { const int w = (int)(u(g) * 127); e->modWheel(w); o->modWheel(w); }
            else if (ev < 0.14f) { const int b = (int)(u(g) * 16383); e->pitchBend(b); o->pitchBend(b); }
            else if (ev < 0.16f) { const int a = (int)(u(g) * 127); e->channelPressure(a); o->channelPressure(a); }
            const bool play = (i / 4096) % 2 == 0;
            e->setTransport(true, play, i / (double)sr * 0.5, 120); o->setTransport(true, play, i / (double)sr * 0.5, 120);
            if (i == N / 2) { const float d = u(g); e->p[bp(0, kBDrive)] = o->p[bp(0, kBDrive)] = d; }
            e->process(l.data() + i, r.data() + i, k.data() + i, k.data() + i, L1.data() + i, R1.data() + i, n);
            o->process(l.data() + i, r.data() + i, k.data() + i, k.data() + i, L2.data() + i, R2.data() + i, n);
        }
        double d = 0, s = 1e-30; for (int i = 0; i < N; ++i) { d += (L1[i] - L2[i]) * (L1[i] - L2[i]) + (R1[i] - R2[i]) * (R1[i] - R2[i]); s += l[i] * l[i] + r[i] * r[i]; }
        const float db = d == 0 ? -240.f : (float)(10 * std::log10(d / s));
        worst = std::max(worst, db); if (db > -120) { ++bad; printf("trial %d (%d Hz): difference %.1f dB\n", trial, sr, db); }
    }
    printf("2.0 refactor guard: largest difference %.1f dB below the input level, %d failures\n", -worst, bad);
    return bad;
}
