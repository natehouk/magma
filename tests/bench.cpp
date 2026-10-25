// CPU: 1, 3 and 6 bands with modulation, normal (2x), good (8x) and superb (32x), natural and linear phase.
#include "../plugins/Magma/ParamDefs.hpp"
#include <chrono>
#include <cstdio>
#include <memory>
using namespace magma;
int main() {
    const int SR = 48000; static float in[SR], L[SR], R[SR]; uint32_t r = 1; for (float& x : in) { r = r * 1664525u + 1013904223u; x = 0.3f * ((int32_t)r * (1.f / 2147483648.f)); }
    for (int lin = 0; lin < 2; ++lin) for (int q = 0; q < 3; ++q) for (int nb : { 1, 3, 6 }) {
        if (lin && nb == 1) continue;
        std::unique_ptr<Engine> e(new Engine()); e->p[kNumBands] = (float)nb; e->p[kQuality] = q >= 1 ? 1.f : 0.f; e->p[kSuperb] = q == 2 ? 1.f : 0.f; e->p[kXPhase] = (float)lin;
        for (int b = 0; b < nb; ++b) { e->p[bp(b, kBStyle)] = (float)(b * 5 % kNumStyles); e->p[bp(b, kBDrive)] = 0.5f; e->p[bp(b, kBBass)] = 2; e->p[bp(b, kBTreble)] = -2; }
        for (int s = 0; s < 8; ++s) e->setSlot(s, 1 + (s % 4), kDstBandFirst + (s % nb) * kNumBandDests, 0.2f);
        e->setSampleRate(SR);
        const int secs = q == 2 ? 1 : 4; auto t0 = std::chrono::steady_clock::now();
        for (int k = 0; k < secs; ++k) for (int b = 0; b < SR; b += 256) e->process(in + b, in + b, nullptr, nullptr, L + b, R + b, 256);
        const double el = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
        printf("%s %s, %d band%s: %.1f%% of one core\n", lin ? "linear " : "natural", q == 2 ? "superb 32x" : (q ? "good 8x   " : "normal 2x "), nb, nb == 1 ? " " : "s", 100 * el / secs);
    }
    return 0;
}
