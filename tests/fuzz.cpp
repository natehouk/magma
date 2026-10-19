// Random parameters, modulation, MIDI and band changes at 44.1-192 kHz with loud noise:
// output always finite and bounded.
#include "../plugins/Magma/ParamDefs.hpp"
#include <cstdio>
#include <memory>
#include <random>
using namespace magma;
int main() {
    std::mt19937 g(29); std::uniform_real_distribution<float> u(0, 1); int bad = 0; float pk = 0;
    for (int sr : { 44100, 48000, 96000, 192000 }) {
        std::unique_ptr<Engine> e(new Engine()); e->setSampleRate((float)sr);
        for (int it = 0; it < 400; ++it) {
            for (uint32_t i = 0; i < kOutLearn; ++i) {
                if (u(g) < 0.85f) continue;
                const ParamDef& d = paramDef(i);
                float v = (u(g) < .3f) ? (u(g) < .5f ? d.min : d.max) : d.min + (d.max - d.min) * u(g);
                if (d.hints & kHintInteger) v = std::round(v);
                e->p[i] = v;
            }
            if (u(g) < 0.1f) e->noteOn(36 + (int)(u(g) * 60), 1 + (int)(u(g) * 126));
            if (u(g) < 0.1f) e->allNotesOff();
            if (u(g) < 0.1f) e->modWheel((int)(u(g) * 127));
            float in[512], sc[512], L[512], R[512]; const float amp = u(g) < .2f ? 0 : (u(g) < .5f ? 4.f : 0.5f);
            for (int k = 0; k < 512; ++k) { in[k] = amp * (2 * u(g) - 1); sc[k] = 2 * u(g) - 1; }
            e->process(in, in, sc, sc, L, R, 512);
            for (int k = 0; k < 512; ++k) { if (!std::isfinite(L[k]) || !std::isfinite(R[k])) bad++; pk = std::max(pk, std::max(std::fabs(L[k]), std::fabs(R[k]))); }
        }
    }
    printf("fuzz nonfinite=%d peak=%.2f\n", bad, pk); return bad || pk > 64.f;
}
