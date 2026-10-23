// Offline sanity test: builds all 50 presets, plays chords, checks output is finite,
// audible and not clipping wildly. Also checks preset keys against the param-id list.
#include "../Source/Engine.h"
#include "../Source/Presets.h"
#include <cstdio>
#include <set>

int main() {
    using namespace d50;
    auto presets = buildFactoryPresets();
    auto ids = allParamIds();
    std::set<std::string> idset(ids.begin(), ids.end());
    int bad = 0;
    if (presets.size() != 50) { std::printf("FAIL: %zu presets\n", presets.size()); ++bad; }

    const double sr = 48000.0;
    Engine eng; eng.prepare(sr);
    std::vector<float> L(512), R(512);

    for (auto& pr : presets) {
        for (auto& kv : pr.values) if (!idset.count(kv.first)) { std::printf("FAIL %s: unknown key %s\n", pr.name.c_str(), kv.first.c_str()); ++bad; }
        Patch p; int used = patchFromMap(pr.values, p);
        // revsize / revmix are handled by the plugin wrapper (juce::Reverb), not the engine Patch
        if (used != (int)pr.values.size() - 2) { std::printf("FAIL %s: consumed %d of %zu\n", pr.name.c_str(), used, pr.values.size()); ++bad; }
        eng.allNotesOff(); eng.setPatch(p);
        for (int i = 0; i < 300; ++i) eng.render(L.data(), R.data(), 512); // flush tails
        eng.setPatch(p);
        for (int n : { 48, 60, 64, 67 }) eng.noteOn(n, 0.8f);
        float peak = 0, energy = 0; bool finite = true;
        for (int b = 0; b < 200; ++b) {
            eng.render(L.data(), R.data(), 512);
            for (int i = 0; i < 512; ++i) {
                if (!std::isfinite(L[i]) || !std::isfinite(R[i])) finite = false;
                peak = std::max({ peak, std::fabs(L[i]), std::fabs(R[i]) });
                energy += L[i] * L[i];
            }
        }
        eng.allNotesOff();
        if (!finite || peak < 0.01f || peak > 1.0f) { std::printf("FAIL %-18s finite=%d peak=%.3f\n", pr.name.c_str(), finite, peak); ++bad; }
        else std::printf("ok   %-18s %-8s peak=%.3f rms=%.4f\n", pr.name.c_str(), pr.category.c_str(), peak, std::sqrt(energy / (200 * 512)));
    }
    std::printf(bad ? "\n%d problem(s)\n" : "\nALL PASSED\n", bad);
    return bad ? 1 : 0;
}
