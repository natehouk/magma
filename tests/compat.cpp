// 1.x compatibility: random 1.x settings (2.0 parameters at their defaults) including bands, styles and the
// modulation matrix, with changes mid-stream, through the frozen 1.0 engine and the 2.0 engine: outputs must match.
#include "../plugins/Magma/ParamDefs.hpp"
#include "legacy/MagmaEngine1.hpp"
#include <cstdio>
#include <memory>
#include <random>
#include <vector>
using namespace magma;
int main() {
    std::mt19937 g(21); std::uniform_real_distribution<float> u(0, 1);
    int bad = 0; float worstAll = -1000;
    for (int sr : { 44100, 48000, 96000 }) for (int trial = 0; trial < 10; ++trial) {
        std::unique_ptr<Engine> e(new Engine()); std::unique_ptr<magma1::Engine> o(new magma1::Engine());
        for (uint32_t i = 0; i < kOutLearn; ++i) {
            const Range r = paramRange(i); float lo = r.min, hi = r.max;
            if (i == kXSlope) hi = 2;                                         // 1.x ranges
            if ((i - kBandFirst) % kBandFields == kBStyle && i >= kBandFirst && i < kXlfoFirst) hi = kNumStyles1 - 1;
            float v = r.log ? lo * std::pow(hi / lo, u(g)) : lo + (hi - lo) * u(g);
            if (r.integer) v = std::round(v);
            if (i >= kSlotFirst && (i - kSlotFirst) % kSlotFields == kSlotSrc && u(g) < 0.8f) v = 0;     // most slots empty
            if (i == kOutGain) v = std::min(v, 6.f);
            e->p[i] = v; o->p[i] = v;
        }
        e->setSampleRate((float)sr); o->setSampleRate((float)sr);
        const int N = sr; std::vector<float> l(N), r(N), k(N), L1(N), R1(N), L2(N), R2(N);
        for (int i = 0; i < N; ++i) { const float env = (i / (sr / 5)) % 2 ? 0.6f : 0.05f; l[i] = env * (2 * u(g) - 1); r[i] = 0.7f * l[i] + 0.2f * env * (2 * u(g) - 1); k[i] = 0.3f * std::sin(0.01f * i); }
        for (int i = 0; i < N; i += 300) {
            const int n = std::min(300, N - i);
            if (i >= N / 2 && i < N / 2 + 300) { const float d = u(g); e->p[bp(0, kBDrive)] = o->p[magma1::bp(0, magma1::kBDrive)] = d; e->p[kX1] = o->p[magma1::kX1] = 300 + 1000 * u(g); }
            o->process(l.data() + i, r.data() + i, k.data() + i, k.data() + i, L1.data() + i, R1.data() + i, n);
            e->process(l.data() + i, r.data() + i, k.data() + i, k.data() + i, L2.data() + i, R2.data() + i, n);
        }
        float worst = 0; for (int i = 0; i < N; ++i) worst = std::max(worst, std::max(std::fabs(L1[i] - L2[i]), std::fabs(R1[i] - R2[i])));
        const float rel = 20 * std::log10(worst / 0.6f + 1e-12f);
        worstAll = std::max(worstAll, rel);
        if (rel > -120 || e->latency() != o->latency()) { ++bad; printf("FAIL sr %d trial %d bands %.0f q %.0f: difference %.1f dB below the input, latency %u/%u\n", sr, trial, e->p[kNumBands], e->p[kQuality], -rel, e->latency(), o->latency()); }
    }
    printf("1.x compatibility: largest difference %.1f dB below the input level, %d failures\n", -worstAll, bad);
    return bad;
}
