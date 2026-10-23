// LA-50 engine: framework-free C++17 (no JUCE) so it can be unit-tested offline.
// Linear-Arithmetic-style architecture: per tone, a short one-shot "attack"
// transient (synthesised at start-up, standing in for PCM samples) is layered
// over two resonant-filtered synth partials (TVF + TVA envelopes, LFO, PWM),
// with Mix / Ring / Sync partial structures. Two tones, Single / Dual / Split.
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace d50 {

constexpr float kPi = 3.14159265358979f;

enum Pcm { PcmNone = 0, PcmPluck, PcmBell, PcmChiff, PcmThump, PcmMarimba, PcmStrike,
           PcmAh, PcmHammer, PcmBrass, PcmClick, PcmGlass, PcmBreath, PcmCount };

inline constexpr const char* kPcmNames[PcmCount] = {
    "None", "Pluck", "Bell", "Chiff", "Thump", "Marimba", "Strike",
    "Ah Voice", "Hammer", "Brass Blat", "Click", "Glass", "Breath" };

struct EnvP { float a = 0.01f, d = 0.5f, s = 0.7f, r = 0.3f; };

struct PartialP {
    int wave = 0;                 // 0 saw, 1 square
    float pw = 0.5f, pwLfo = 0.f;
    int coarse = 0;               // semitones
    float fine = 0.f;             // cents
    float level = 1.f;
    float cutoff = 0.6f, res = 0.1f, fenv = 0.f, kf = 0.5f;
    EnvP fe, ae;
};

struct ToneP {
    int structure = 0;            // 0 mix, 1 ring, 2 sync
    int pcm = 0;
    float pcmLevel = 0.5f;
    float lfoRate = 5.f, lfoPitch = 0.f, lfoFilt = 0.f, lfoDelay = 0.f;
    float level = 1.f, pan = 0.f;
    PartialP p[2];
};

struct Patch {
    ToneP t[2];
    int mode = 1;                 // 0 single (A), 1 dual, 2 split
    int split = 60;
    float chorusRate = 0.8f, chorusDepth = 0.5f, chorusMix = 0.3f;
    float master = 0.7f;
    int bend = 2;
};

// ---------------------------------------------------------------------------
class AttackBank {
public:
    void build(double sr) {
        const int N = (int)(sr * 0.6);
        banks.assign(PcmCount, std::vector<float>());
        for (int t = 1; t < PcmCount; ++t) { banks[t].assign(N, 0.f); gen(t, banks[t], sr); }
    }
    const std::vector<float>& get(int i) const { return banks[(size_t)std::clamp(i, 0, PcmCount - 1)]; }

private:
    std::vector<std::vector<float>> banks;

    static void gen(int type, std::vector<float>& o, double sr) {
        const int N = (int)o.size();
        const float f0 = 261.63f;
        const float twoPi = 2.f * kPi;
        uint32_t rng = 12345u + (uint32_t)type * 77u;
        auto rnd = [&]() { rng = rng * 1664525u + 1013904223u; return (float)(rng >> 8) * (1.0f / 8388608.0f) - 1.0f; };
        auto T = [&](int i) { return (float)(i / sr); };
        auto lpCoef = [&](float fc) { return 1.f - std::exp(-twoPi * fc / (float)sr); };
        auto addPartial = [&](float base, float ratio, float amp, float decay) {
            for (int i = 0; i < N; ++i) { float t = T(i); o[(size_t)i] += amp * std::sin(twoPi * base * ratio * t) * std::exp(-decay * t); }
        };
        switch (type) {
        case PcmPluck: {
            const int P = std::max(2, (int)(sr / f0));
            std::vector<float> b((size_t)P);
            for (auto& x : b) x = rnd();
            int idx = 0;
            for (int i = 0; i < N; ++i) {
                int nx = (idx + 1) % P; float y = b[(size_t)idx];
                b[(size_t)idx] = 0.5f * (b[(size_t)idx] + b[(size_t)nx]) * 0.996f; o[(size_t)i] = y; idx = nx;
            }
        } break;
        case PcmBell: {
            const float r[4] = { 1.f, 2.76f, 5.4f, 8.93f }, a[4] = { 1.f, .6f, .4f, .25f }, d[4] = { 3.f, 5.f, 8.f, 12.f };
            for (int k = 0; k < 4; ++k) addPartial(f0, r[k], a[k], d[k]);
        } break;
        case PcmChiff: {
            float l1 = 0, l2 = 0; const float c1 = lpCoef(4000.f), c2 = lpCoef(800.f);
            for (int i = 0; i < N; ++i) {
                float t = T(i), n = rnd(); l1 += c1 * (n - l1); l2 += c2 * (n - l2);
                o[(size_t)i] = (l1 - l2) * (1.f - std::exp(-t * 200.f)) * std::exp(-t * 14.f);
            }
        } break;
        case PcmThump: {
            float ph = 0;
            for (int i = 0; i < N; ++i) {
                float t = T(i); ph += twoPi * (50.f + 200.f * std::exp(-t * 30.f)) / (float)sr;
                o[(size_t)i] = std::sin(ph) * std::exp(-t * 10.f);
            }
        } break;
        case PcmMarimba: {
            const float r[3] = { 1.f, 4.f, 9.9f }, a[3] = { 1.f, .5f, .2f }, d[3] = { 7.f, 30.f, 60.f };
            for (int k = 0; k < 3; ++k) addPartial(f0, r[k], a[k], d[k]);
        } break;
        case PcmStrike: {
            float l = 0; const float c = lpCoef(5000.f);
            for (int i = 0; i < N; ++i) {
                float t = T(i); l += c * (rnd() - l);
                o[(size_t)i] = l * std::exp(-t * 70.f) + 0.6f * std::sin(twoPi * 1100.f * t) * std::exp(-t * 35.f);
            }
        } break;
        case PcmAh: {
            auto gauss = [](float f, float fc, float bw) { float x = (f - fc) / bw; return std::exp(-x * x); };
            for (int h = 1; h <= 45; ++h) {
                float f = (float)h * f0;
                float amp = (1.f / std::pow((float)h, 0.8f)) *
                            (gauss(f, 800.f, 150.f) + 0.6f * gauss(f, 1150.f, 200.f) + 0.3f * gauss(f, 2800.f, 300.f) + 0.05f);
                for (int i = 0; i < N; ++i) {
                    float t = T(i);
                    o[(size_t)i] += amp * std::sin(twoPi * f * t) * (1.f - std::exp(-t * 40.f)) * std::exp(-t * 2.5f);
                }
            }
        } break;
        case PcmHammer: {
            float l = 0; const float c = lpCoef(3000.f);
            for (int i = 0; i < N; ++i) { float t = T(i); l += c * (rnd() - l); o[(size_t)i] = 0.8f * l * std::exp(-t * 50.f); }
            for (int h = 1; h <= 6; ++h) addPartial(f0, (float)h, 1.f / (float)h, 6.f + 3.f * (float)h);
        } break;
        case PcmBrass: {
            for (int i = 0; i < N; ++i) {
                float t = T(i), bright = std::exp(-t * 6.f), s = 0;
                for (int h = 1; h <= 24; ++h) s += std::sin(twoPi * f0 * (float)h * t) / (float)h * (h < 4 ? 1.f : bright);
                o[(size_t)i] = s * std::min(1.f, t * 80.f) * std::exp(-t * 3.5f);
            }
        } break;
        case PcmClick: {
            float l = 0; const float c = lpCoef(6000.f);
            for (int i = 0; i < N; ++i) {
                float t = T(i); l += c * (rnd() - l);
                o[(size_t)i] = l * std::exp(-t * 500.f) + 0.5f * std::sin(twoPi * 2200.f * t) * std::exp(-t * 600.f);
            }
        } break;
        case PcmGlass: {
            const float r[5] = { 1.f, 2.32f, 4.25f, 6.63f, 9.38f }, a[5] = { 1.f, .7f, .5f, .3f, .2f }, d[5] = { 2.5f, 4.f, 6.f, 9.f, 14.f };
            for (int k = 0; k < 5; ++k) addPartial(f0 * 2.f, r[k], a[k], d[k]);
        } break;
        case PcmBreath: {
            float l1 = 0, l2 = 0; const float c1 = lpCoef(3000.f), c2 = lpCoef(300.f);
            for (int i = 0; i < N; ++i) {
                float t = T(i), n = rnd(); l1 += c1 * (n - l1); l2 += c2 * (n - l2);
                o[(size_t)i] = (l1 - l2) * (1.f - std::exp(-t * 12.f)) * std::exp(-t * 3.f);
            }
        } break;
        default: break;
        }
        float peak = 1e-6f;
        for (float x : o) peak = std::max(peak, std::fabs(x));
        const int fade = N / 20;
        for (int i = 0; i < N; ++i) {
            float g = 0.9f / peak;
            if (i > N - fade) g *= (float)(N - i) / (float)fade;
            o[(size_t)i] *= g;
        }
    }
};

// ---------------------------------------------------------------------------
class Chorus {
public:
    void prepare(double sr) { sampleRate = (float)sr; bl.assign(8192, 0.f); br.assign(8192, 0.f); w = 0; ph = 0; }
    void process(float& l, float& r, float rate, float depth, float mix) {
        if (bl.empty()) return;
        bl[(size_t)w] = l; br[(size_t)w] = r;
        ph += rate / sampleRate; if (ph >= 1.f) ph -= 1.f;
        const float lfo = std::sin(2.f * kPi * ph);
        const float dl = (9.f + depth * 4.f * lfo) * 0.001f * sampleRate;
        const float dr = (9.f - depth * 4.f * lfo) * 0.001f * sampleRate;
        const float wl = read(bl, dl), wr = read(br, dr);
        w = (w + 1) & 8191;
        l = l * (1.f - 0.35f * mix) + wl * mix * 0.7f;
        r = r * (1.f - 0.35f * mix) + wr * mix * 0.7f;
    }
private:
    float read(const std::vector<float>& b, float d) const {
        float pos = (float)w - d; while (pos < 0.f) pos += 8192.f;
        int i0 = (int)pos; float fr = pos - (float)i0;
        int i1 = (i0 + 1) & 8191; i0 &= 8191;
        return b[(size_t)i0] * (1.f - fr) + b[(size_t)i1] * fr;
    }
    std::vector<float> bl, br; int w = 0; float ph = 0, sampleRate = 44100.f;
};

// ---------------------------------------------------------------------------
struct Env {
    enum { Idle, Att, Dec, Sus, Rel } st = Idle;
    float v = 0.f;
    void on() { st = Att; }
    void off() { if (st != Idle) st = Rel; }
    float tick(const EnvP& p, float sr, int n) {
        switch (st) {
        case Att:
            v += (float)n / (std::max(p.a, 0.0005f) * sr);
            if (v >= 1.f) { v = 1.f; st = Dec; }
            break;
        case Dec: {
            float c = 1.f - std::exp(-4.6f * (float)n / (std::max(p.d, 0.001f) * sr));
            v += (p.s - v) * c;
            if (std::fabs(v - p.s) < 1e-3f) { v = p.s; st = Sus; }
        } break;
        case Sus:
            v = p.s;
            if (v < 1e-4f) { v = 0.f; st = Idle; }
            break;
        case Rel: {
            float c = 1.f - std::exp(-4.6f * (float)n / (std::max(p.r, 0.001f) * sr));
            v -= v * c;
            if (v < 1e-4f) { v = 0.f; st = Idle; }
        } break;
        default: break;
        }
        return v;
    }
};

struct PartState {
    float phase = 0, ic1 = 0, ic2 = 0, amp = 0, ampStep = 0, inc = 0.01f;
    float a1 = 0, a2 = 0, a3 = 0, pwNow = 0.5f;
    Env fe, ae;
};
struct ToneState {
    bool active = false, wrap1 = false, pcmOn = false;
    PartState p[2];
    float lfoPhase = 0, lfoTime = 0, pcmPos = 0, pcmRate = 1.f, pcmGain = 0.f;
    int pcmType = 0;
};
struct VoiceState {
    bool on = false, held = false, sustained = false;
    int note = -1, ctl = 0;
    float vel = 0;
    uint64_t age = 0;
    ToneState t[2];
};

class Engine {
public:
    static constexpr int kVoices = 16;
    static constexpr int kCtl = 8;

    void prepare(double sr) {
        sampleRate = (float)sr; bank.build(sr); chorus.prepare(sr);
        for (auto& v : voices) v = VoiceState();
    }
    void setPatch(const Patch& p) { patch = p; }
    void setBend(float b) { bend = b; }
    void setMod(float m) { mod = m; }

    void noteOn(int note, float vel) {
        VoiceState* vp = nullptr;
        for (auto& v : voices) if (v.on && v.note == note) { vp = &v; break; }
        if (!vp) for (auto& v : voices) if (!v.on) { vp = &v; break; }
        if (!vp) {
            uint64_t best = UINT64_MAX;
            for (auto& v : voices) {
                uint64_t score = v.age + (v.held ? (1ull << 40) : 0ull);
                if (score < best) { best = score; vp = &v; }
            }
        }
        *vp = VoiceState();
        vp->on = true; vp->held = true; vp->note = note; vp->vel = std::clamp(vel, 0.f, 1.f);
        vp->age = ++ageCounter;
        for (int t = 0; t < 2; ++t) {
            bool act = patch.mode == 1 || (patch.mode == 0 && t == 0) || (patch.mode == 2 && ((note < patch.split) == (t == 0)));
            ToneState& ts = vp->t[t]; ts.active = act;
            if (!act) continue;
            ts.pcmType = patch.t[t].pcm;
            ts.pcmOn = ts.pcmType > 0 && patch.t[t].pcmLevel > 0.f;
            ts.pcmGain = patch.t[t].pcmLevel * (0.4f + 0.6f * vp->vel);
            for (auto& ps : ts.p) { ps.fe.on(); ps.ae.on(); }
        }
    }
    void noteOff(int note) {
        for (auto& v : voices) if (v.on && v.held && v.note == note) {
            v.held = false;
            if (sustain) v.sustained = true; else release(v);
        }
    }
    void setSustain(bool on) {
        sustain = on;
        if (!on) for (auto& v : voices) if (v.on && v.sustained) { v.sustained = false; release(v); }
    }
    void allNotesOff() { for (auto& v : voices) if (v.on) { v.held = false; v.sustained = false; release(v); } }

    void render(float* L, float* R, int n) {
        std::fill(L, L + n, 0.f); std::fill(R, R + n, 0.f);
        for (auto& v : voices) if (v.on) renderVoice(v, L, R, n);
        for (int i = 0; i < n; ++i) {
            float l = L[i] * patch.master, r = R[i] * patch.master;
            chorus.process(l, r, patch.chorusRate, patch.chorusDepth, patch.chorusMix);
            L[i] = std::tanh(l); R[i] = std::tanh(r);
        }
    }
    int activeVoices() const { int c = 0; for (auto& v : voices) c += v.on ? 1 : 0; return c; }

private:
    Patch patch;
    VoiceState voices[kVoices];
    AttackBank bank;
    Chorus chorus;
    float sampleRate = 44100.f, bend = 0.f, mod = 0.f;
    bool sustain = false;
    uint64_t ageCounter = 0;

    void release(VoiceState& v) {
        for (auto& ts : v.t) if (ts.active) for (auto& ps : ts.p) { ps.ae.off(); ps.fe.off(); }
    }
    static float polyblep(float t, float dt) {
        if (t < dt) { t /= dt; return t + t - t * t - 1.f; }
        if (t > 1.f - dt) { t = (t - 1.f) / dt; return t * t + t + t + 1.f; }
        return 0.f;
    }

    void control(VoiceState& v) {
        bool alive = false;
        for (int t = 0; t < 2; ++t) {
            ToneState& ts = v.t[t]; if (!ts.active) continue;
            const ToneP& tp = patch.t[t];
            ts.lfoTime += (float)kCtl / sampleRate;
            ts.lfoPhase += tp.lfoRate * (float)kCtl / sampleRate; if (ts.lfoPhase >= 1.f) ts.lfoPhase -= 1.f;
            const float fade = tp.lfoDelay < 0.001f ? 1.f : std::clamp(ts.lfoTime / tp.lfoDelay, 0.f, 1.f);
            const float lfo = std::sin(2.f * kPi * ts.lfoPhase) * fade;
            const float semis = (float)(v.note - 69) + bend * (float)patch.bend + lfo * (tp.lfoPitch + mod * 0.6f);
            const float baseFreq = 440.f * std::exp2(semis / 12.f);
            ts.pcmRate = baseFreq / 261.63f;
            for (int p = 0; p < 2; ++p) {
                PartState& ps = ts.p[p]; const PartialP& pp = tp.p[p];
                const float fe = ps.fe.tick(pp.fe, sampleRate, kCtl);
                const float ae = ps.ae.tick(pp.ae, sampleRate, kCtl);
                if (ps.ae.st != Env::Idle) alive = true;
                const float f = baseFreq * std::exp2(((float)pp.coarse + pp.fine * 0.01f) / 12.f);
                ps.inc = std::clamp(f / sampleRate, 0.00001f, 0.45f);
                ps.pwNow = std::clamp(pp.pw + pp.pwLfo * 0.4f * lfo, 0.05f, 0.95f);
                const float oct = pp.cutoff * 9.5f + pp.fenv * 5.f * fe * (0.5f + 0.5f * v.vel)
                                + pp.kf * (float)(v.note - 60) / 12.f + tp.lfoFilt * 2.f * lfo;
                const float fc = std::clamp(30.f * std::exp2(oct), 20.f, 0.45f * sampleRate);
                const float g = std::tan(kPi * fc / sampleRate);
                const float k = 2.f - 1.96f * std::clamp(pp.res, 0.f, 1.f);
                ps.a1 = 1.f / (1.f + g * (g + k)); ps.a2 = g * ps.a1; ps.a3 = g * ps.a2;
                const float target = ae * pp.level * (0.35f + 0.65f * v.vel);
                ps.ampStep = (target - ps.amp) / (float)kCtl;
            }
        }
        if (!alive) v.on = false;
    }

    void renderVoice(VoiceState& v, float* L, float* R, int n) {
        for (int i = 0; i < n; ++i) {
            if (v.ctl == 0) { control(v); if (!v.on) return; }
            v.ctl = (v.ctl + 1) & (kCtl - 1);
            float outL = 0.f, outR = 0.f;
            for (int t = 0; t < 2; ++t) {
                ToneState& ts = v.t[t]; if (!ts.active) continue;
                const ToneP& tp = patch.t[t];
                float s[2];
                for (int p = 0; p < 2; ++p) {
                    PartState& ps = ts.p[p]; const PartialP& pp = tp.p[p];
                    ps.phase += ps.inc;
                    bool wrap = false;
                    if (ps.phase >= 1.f) { ps.phase -= 1.f; wrap = true; }
                    if (p == 0) ts.wrap1 = wrap;
                    else if (tp.structure == 2 && ts.wrap1) ps.phase = 0.f;
                    const float ph = ps.phase, dt = ps.inc;
                    float osc;
                    if (pp.wave == 0) osc = 2.f * ph - 1.f - polyblep(ph, dt);
                    else {
                        osc = ph < ps.pwNow ? 1.f : -1.f;
                        osc += polyblep(ph, dt);
                        float p2 = ph - ps.pwNow; if (p2 < 0.f) p2 += 1.f;
                        osc -= polyblep(p2, dt);
                    }
                    const float x = osc * 0.7f;
                    const float v3 = x - ps.ic2;
                    const float v1 = ps.a1 * ps.ic1 + ps.a2 * v3;
                    const float v2 = ps.ic2 + ps.a2 * ps.ic1 + ps.a3 * v3;
                    ps.ic1 = 2.f * v1 - ps.ic1; ps.ic2 = 2.f * v2 - ps.ic2;
                    ps.amp += ps.ampStep;
                    s[p] = v2 * ps.amp;
                }
                float mix = tp.structure == 0 ? s[0] + s[1] : (tp.structure == 1 ? s[0] * s[1] * 3.f : s[1] * 1.2f);
                if (ts.pcmOn) {
                    const auto& smp = bank.get(ts.pcmType);
                    const int ip = (int)ts.pcmPos;
                    if (ip + 1 < (int)smp.size()) {
                        const float fr = ts.pcmPos - (float)ip;
                        mix += (smp[(size_t)ip] * (1.f - fr) + smp[(size_t)ip + 1] * fr) * ts.pcmGain;
                        ts.pcmPos += ts.pcmRate;
                    } else ts.pcmOn = false;
                }
                mix *= tp.level;
                outL += mix * (tp.pan > 0.f ? 1.f - tp.pan : 1.f);
                outR += mix * (tp.pan < 0.f ? 1.f + tp.pan : 1.f);
            }
            L[i] += outL * 0.3f; R[i] += outR * 0.3f;
        }
    }
};

} // namespace d50
