// LA-50 factory bank: 50 ORIGINAL presets in the spirit of classic LA-synth
// categories (pads, strings, brass, mallets, plucks, basses, leads, voices, FX).
// Pure C++ so it can be tested offline. Names and settings are original.
#pragma once
#include <map>
#include <string>
#include <vector>
#include <algorithm>
#include "Engine.h"

namespace d50 {

using ParamMap = std::map<std::string, float>;
struct PresetData { std::string name, category; ParamMap values; };

inline std::vector<std::string> allParamIds() {
    std::vector<std::string> ids;
    for (std::string t : { "a", "b" }) {
        for (auto s : { "struct", "pcm", "pcmlvl", "lforate", "lfopitch", "lfofilt", "lfodelay", "level", "pan" })
            ids.push_back(t + "_" + s);
        for (int p = 1; p <= 2; ++p)
            for (auto s : { "wave", "pw", "pwlfo", "coarse", "fine", "level", "cutoff", "res", "fenv", "kf",
                            "fa", "fd", "fs", "fr", "aa", "ad", "as", "ar" })
                ids.push_back(t + std::to_string(p) + "_" + s);
    }
    for (auto s : { "mode", "split", "chorusrate", "chorusdepth", "chorusmix", "revsize", "revmix", "master", "bend" })
        ids.push_back(s);
    return ids;
}

// Maps a preset's parameter map onto an engine Patch (used by tests / offline renders).
// Returns the number of keys consumed.
inline int patchFromMap(const ParamMap& m, Patch& p) {
    int n = 0;
    auto gf = [&](const std::string& k, float& d) { auto it = m.find(k); if (it != m.end()) { d = it->second; ++n; } };
    auto gi = [&](const std::string& k, int& d) { auto it = m.find(k); if (it != m.end()) { d = (int)std::lround(it->second); ++n; } };
    const char* tn[2] = { "a", "b" };
    for (int t = 0; t < 2; ++t) {
        std::string pre = tn[t]; ToneP& T = p.t[t];
        gi(pre + "_struct", T.structure); gi(pre + "_pcm", T.pcm); gf(pre + "_pcmlvl", T.pcmLevel);
        gf(pre + "_lforate", T.lfoRate); gf(pre + "_lfopitch", T.lfoPitch); gf(pre + "_lfofilt", T.lfoFilt);
        gf(pre + "_lfodelay", T.lfoDelay); gf(pre + "_level", T.level); gf(pre + "_pan", T.pan);
        for (int i = 0; i < 2; ++i) {
            std::string q = pre + std::to_string(i + 1) + "_"; PartialP& P = T.p[i];
            gi(q + "wave", P.wave); gf(q + "pw", P.pw); gf(q + "pwlfo", P.pwLfo); gi(q + "coarse", P.coarse);
            gf(q + "fine", P.fine); gf(q + "level", P.level); gf(q + "cutoff", P.cutoff); gf(q + "res", P.res);
            gf(q + "fenv", P.fenv); gf(q + "kf", P.kf);
            gf(q + "fa", P.fe.a); gf(q + "fd", P.fe.d); gf(q + "fs", P.fe.s); gf(q + "fr", P.fe.r);
            gf(q + "aa", P.ae.a); gf(q + "ad", P.ae.d); gf(q + "as", P.ae.s); gf(q + "ar", P.ae.r);
        }
    }
    gi("mode", p.mode); gi("split", p.split); gf("chorusrate", p.chorusRate); gf("chorusdepth", p.chorusDepth);
    gf("chorusmix", p.chorusMix); gf("master", p.master); gi("bend", p.bend);
    return n;
}

// One tone's worth of settings, built with chained setters.
struct Recipe {
    int w1 = 0, w2 = 0, c2 = 0, pcm = 0, st = 0;
    float pw = .5f, pwlfo = 0, det = 10, cut = .6f, res = .1f, fenv = 0, kf = .5f;
    float fa = .01f, fd = .5f, fs = .7f, fr = .3f, aa = .01f, ad = .5f, as_ = .8f, ar = .3f;
    float pcmLvl = .5f, lv = 1, pan = 0, lfoRate = 5, lfoP = 0, lfoF = 0, lfoD = 0, p2 = 1;
    Recipe& W(int a, int b) { w1 = a; w2 = b; return *this; }
    Recipe& C2(int s) { c2 = s; return *this; }
    Recipe& Det(float c) { det = c; return *this; }
    Recipe& Cut(float x) { cut = x; return *this; }
    Recipe& Res(float x) { res = x; return *this; }
    Recipe& FEnv(float x) { fenv = x; return *this; }
    Recipe& KF(float x) { kf = x; return *this; }
    Recipe& F(float a, float d, float s, float r) { fa = a; fd = d; fs = s; fr = r; return *this; }
    Recipe& A(float a, float d, float s, float r) { aa = a; ad = d; as_ = s; ar = r; return *this; }
    Recipe& Pcm(int t, float l) { pcm = t; pcmLvl = l; return *this; }
    Recipe& St(int s) { st = s; return *this; }
    Recipe& Lv(float x) { lv = x; return *this; }
    Recipe& Pan(float x) { pan = x; return *this; }
    Recipe& Lfo(float r, float p, float f, float d) { lfoRate = r; lfoP = p; lfoF = f; lfoD = d; return *this; }
    Recipe& P2(float x) { p2 = x; return *this; }
    Recipe& PW(float w, float m) { pw = w; pwlfo = m; return *this; }
    Recipe& Off() { lv = 0; pcmLvl = 0; pcm = 0; return *this; }
};

inline void applyTone(ParamMap& m, const std::string& t, const Recipe& r) {
    m[t + "_struct"] = (float)r.st; m[t + "_pcm"] = (float)r.pcm; m[t + "_pcmlvl"] = r.pcmLvl;
    m[t + "_lforate"] = r.lfoRate; m[t + "_lfopitch"] = r.lfoP; m[t + "_lfofilt"] = r.lfoF;
    m[t + "_lfodelay"] = r.lfoD; m[t + "_level"] = r.lv; m[t + "_pan"] = r.pan;
    for (int p = 0; p < 2; ++p) {
        std::string q = t + std::to_string(p + 1) + "_";
        m[q + "wave"] = (float)(p == 0 ? r.w1 : r.w2); m[q + "pw"] = r.pw; m[q + "pwlfo"] = r.pwlfo;
        m[q + "coarse"] = (float)(p == 0 ? 0 : r.c2);
        m[q + "fine"] = std::clamp(p == 0 ? -r.det * .5f : r.det * .5f, -50.f, 50.f);
        m[q + "level"] = p == 0 ? 1.f : r.p2;
        m[q + "cutoff"] = r.cut; m[q + "res"] = r.res; m[q + "fenv"] = r.fenv; m[q + "kf"] = r.kf;
        m[q + "fa"] = r.fa; m[q + "fd"] = r.fd; m[q + "fs"] = r.fs; m[q + "fr"] = r.fr;
        m[q + "aa"] = r.aa; m[q + "ad"] = r.ad; m[q + "as"] = r.as_; m[q + "ar"] = r.ar;
    }
}

// ---- base recipes -------------------------------------------------------
inline Recipe rPad()     { return Recipe().W(0,0).Det(16).Cut(.5f).Res(.12f).FEnv(.25f).F(1.2f,2,.55f,1.8f).A(1.0f,1,.85f,2.0f).Lfo(.35f,0,.06f,0); }
inline Recipe rStrings() { return Recipe().W(0,0).Det(20).Cut(.58f).Res(.08f).FEnv(.2f).F(.4f,1,.7f,.5f).A(.45f,1,.9f,.7f).Lfo(5.3f,.05f,0,.6f).Pcm(PcmChiff,.2f); }
inline Recipe rBrass()   { return Recipe().W(0,0).Det(8).Cut(.35f).Res(.15f).FEnv(.55f).F(.12f,.5f,.45f,.25f).A(.06f,.4f,.85f,.25f).Lfo(5,.04f,0,.8f).Pcm(PcmBrass,.4f); }
inline Recipe rBell()    { return Recipe().W(1,0).C2(12).Det(6).Cut(.7f).Res(.1f).FEnv(-.2f).F(.005f,1.5f,.15f,1).A(.003f,2.5f,0,2.5f).Pcm(PcmBell,.7f); }
inline Recipe rPluck()   { return Recipe().W(0,1).Det(8).Cut(.45f).Res(.1f).FEnv(.6f).F(.003f,.35f,.05f,.2f).A(.002f,.6f,0,.3f).Pcm(PcmPluck,.6f); }
inline Recipe rBass()    { return Recipe().W(0,1).C2(-12).Det(6).Cut(.28f).Res(.3f).FEnv(.5f).KF(.6f).F(.003f,.3f,.2f,.15f).A(.003f,.3f,.8f,.12f).Pcm(PcmThump,.5f); }
inline Recipe rLead()    { return Recipe().W(0,1).Det(10).Cut(.62f).Res(.2f).FEnv(.2f).F(.02f,.4f,.7f,.3f).A(.01f,.3f,.9f,.25f).Lfo(5.5f,.06f,0,.6f).PW(.5f,.3f); }
inline Recipe rChoir()   { return Recipe().W(0,1).Det(14).Cut(.45f).Res(.1f).FEnv(.2f).F(.5f,1,.7f,.6f).A(.45f,1,.9f,1.0f).Pcm(PcmAh,.5f).Lfo(5,.03f,.05f,.6f); }
inline Recipe rFx()      { return Recipe().W(0,0).Det(30).Cut(.5f).Res(.3f).FEnv(.4f).F(1.5f,2,.5f,2).A(1,1,.8f,2).Pcm(PcmBreath,.6f).Lfo(.3f,0,.3f,0); }

inline std::vector<PresetData> buildFactoryPresets() {
    std::vector<PresetData> out;
    auto add = [&](const char* name, const char* cat, const Recipe& A, const Recipe& B,
                   float chorMix, float revMix, float chorDepth = .5f, float revSize = .6f, float chorRate = .8f) {
        PresetData d; d.name = name; d.category = cat;
        applyTone(d.values, "a", A); applyTone(d.values, "b", B);
        d.values["mode"] = 1; d.values["split"] = 60; d.values["master"] = .7f; d.values["bend"] = 2;
        d.values["chorusmix"] = chorMix; d.values["chorusdepth"] = chorDepth; d.values["chorusrate"] = chorRate;
        d.values["revmix"] = revMix; d.values["revsize"] = revSize;
        out.push_back(std::move(d));
    };
    // --- Pads (8)
    add("Glass Harbor",    "Pad", rPad().W(0,1).Cut(.55f).Pcm(PcmGlass,.35f).Lfo(.4f,0,.1f,0), rBell().Lv(.3f).Pcm(PcmNone,0), .5f, .5f);
    add("Velvet Dusk",     "Pad", rPad().Cut(.38f).Det(22).F(2,3,.5f,2.5f).A(1.6f,1,.85f,2.5f), rStrings().Cut(.4f).Lv(.5f).A(1.4f,1,.9f,2.2f), .6f, .5f);
    add("Aurora Drift",    "Pad", rPad().W(1,0).PW(.4f,.6f).Cut(.6f).FEnv(.4f).F(2.5f,4,.4f,2).Lfo(.2f,0,.15f,0), rPad().Lv(.5f).C2(12).Pcm(PcmBreath,.3f).Cut(.7f), .6f, .6f);
    add("Moonlit Tide",    "Pad", rPad().Cut(.45f).FEnv(.35f).F(3,5,.3f,3).A(1.5f,1,.8f,3).Lfo(.15f,0,.2f,0), rPad().Lv(.6f).C2(7).Cut(.4f).Det(30).A(2,1,.8f,3), .7f, .6f);
    add("Crystal Orchard", "Pad", rPad().W(0,1).C2(12).Cut(.65f).Pcm(PcmGlass,.3f), rBell().Lv(.35f).C2(19), .5f, .55f);
    add("Slow Horizon",    "Pad", rPad().Cut(.3f).FEnv(.6f).F(4,6,.4f,3).A(2.5f,1,.9f,3.5f).Det(26), Recipe().Off(), .7f, .6f);
    add("Silk Weather",    "Pad", rStrings().Cut(.5f).A(.9f,1,.9f,1.5f).Det(24).Lfo(.5f,.03f,.05f,.5f), rPad().Lv(.4f).Cut(.45f), .6f, .5f);
    add("Amber Hall",      "Pad", rPad().W(1,0).Cut(.4f).Res(.2f).FEnv(.3f).F(1,2,.6f,2).A(.7f,1,.9f,2), rChoir().Lv(.4f).Pcm(PcmAh,.4f), .5f, .8f);
    // --- Strings (4)
    add("Chamber Silk",    "Strings", rStrings().Cut(.52f), rStrings().Lv(.5f).Det(10).A(.6f,1,.9f,.8f), .4f, .35f);
    add("Northern Strings","Strings", rStrings().Cut(.6f).Det(26).A(.35f,1,.9f,.8f), rBrass().Lv(.25f).Cut(.3f).Pcm(PcmNone,0), .5f, .4f);
    add("Ballroom Bows",   "Strings", rStrings().Cut(.48f).A(.7f,1,.9f,1.2f).Lfo(5,.06f,0,.8f), rStrings().Lv(.6f).C2(12).Cut(.55f), .5f, .6f);
    add("Cinema Swell",    "Strings", rStrings().Cut(.3f).FEnv(.55f).F(2.2f,2,.7f,1.5f).A(1.8f,1,.9f,2), rPad().Lv(.5f).Cut(.35f), .6f, .65f);
    // --- Brass (4)
    add("Brass Cathedral", "Brass", rBrass(), rBrass().Lv(.6f).C2(-12).Cut(.3f).Pcm(PcmNone,0), .3f, .6f);
    add("Sunrise Horns",   "Brass", rBrass().Cut(.4f).FEnv(.5f).F(.2f,.6f,.5f,.3f).A(.1f,.4f,.85f,.3f), rStrings().Lv(.35f).Cut(.45f), .3f, .4f);
    add("Bright Fanfare",  "Brass", rBrass().Cut(.5f).FEnv(.4f).F(.05f,.4f,.6f,.2f).Pcm(PcmBrass,.6f), rBrass().Lv(.7f).C2(12).Cut(.45f), .25f, .4f);
    add("Soft Trombones",  "Brass", rBrass().Cut(.28f).FEnv(.4f).F(.25f,.7f,.5f,.3f).A(.12f,.5f,.85f,.35f).Pcm(PcmBrass,.25f), Recipe().Off(), .3f, .5f);
    // --- Mallets / Bells (5)
    add("Ice Chimes",      "Mallet", rBell().Pcm(PcmBell,.7f), rBell().Lv(.5f).Pcm(PcmGlass,.5f).C2(19), .5f, .7f);
    add("Temple Glass",    "Mallet", rBell().C2(7).Pcm(PcmGlass,.8f).A(.003f,3,0,3), rPad().Lv(.25f).Cut(.5f), .6f, .8f);
    add("Kalimba Rain",    "Mallet", rBell().W(0,1).C2(12).Cut(.55f).Pcm(PcmMarimba,.7f).A(.002f,.9f,0,.5f), rPluck().Lv(.4f), .3f, .55f);
    add("Marimba Dusk",    "Mallet", rPluck().W(1,1).Cut(.3f).Pcm(PcmMarimba,.9f).A(.002f,.5f,0,.3f), rBell().Lv(.2f).C2(24), .2f, .4f);
    add("Music Box Attic", "Mallet", rBell().W(1,0).C2(24).Cut(.6f).Pcm(PcmClick,.4f).A(.002f,1.2f,0,1.2f), rBell().Lv(.3f).Pcm(PcmBell,.5f), .3f, .6f);
    // --- Keys / Plucks (6)
    add("Nylon Echo",      "Pluck", rPluck().Pcm(PcmPluck,.8f), rPluck().Lv(.4f).C2(12).Cut(.5f), .5f, .5f);
    add("Digital Harp",    "Pluck", rPluck().W(0,0).Cut(.5f).Pcm(PcmPluck,.7f).A(.002f,1.4f,0,.8f), rBell().Lv(.3f).C2(12).Pcm(PcmNone,0), .45f, .6f);
    add("Soft Piano Tine", "Pluck", rPluck().W(1,0).Cut(.35f).FEnv(.5f).Pcm(PcmHammer,.7f).A(.002f,2,.1f,.8f).F(.003f,.5f,.1f,.3f), rPluck().Lv(.5f).C2(12).Pcm(PcmNone,0).Cut(.4f), .3f, .35f);
    add("Pluck Garden",    "Pluck", rPluck().Cut(.4f).FEnv(.7f).F(.002f,.25f,0,.2f), rPad().Lv(.3f).Cut(.4f).A(.3f,1,.5f,1), .4f, .5f);
    add("Glass Pizz",      "Pluck", rPluck().W(1,1).Cut(.55f).Pcm(PcmClick,.6f).A(.002f,.35f,0,.15f), rBell().Lv(.3f).Pcm(PcmGlass,.4f), .3f, .4f);
    add("Staccato Pearls", "Pluck", rPluck().Cut(.5f).Pcm(PcmStrike,.5f).A(.002f,.2f,0,.1f).F(.002f,.15f,0,.1f), rPluck().Lv(.5f).C2(7).Cut(.5f).Pcm(PcmNone,0), .3f, .3f);
    // --- Bass (5)
    add("Sub Pulse",       "Bass", rBass().W(1,0).Cut(.22f).Pcm(PcmThump,.6f), Recipe().Off(), .1f, .1f);
    add("Rubber Bass",     "Bass", rBass().Cut(.3f).FEnv(.7f).Res(.35f).Pcm(PcmThump,.4f).F(.002f,.2f,.1f,.1f), rBass().Lv(.4f).C2(12).Cut(.3f).Pcm(PcmNone,0), .15f, .15f);
    add("Digital Thump",   "Bass", rBass().W(1,1).Cut(.35f).FEnv(.4f).Pcm(PcmThump,.9f).A(.002f,.25f,.6f,.12f), rPluck().Lv(.25f).Cut(.4f).Pcm(PcmClick,.4f), .1f, .15f);
    add("Warm Fretless",   "Bass", rBass().W(0,0).Cut(.3f).FEnv(.35f).Pcm(PcmPluck,.3f).Lfo(4.5f,.04f,0,.5f), rBass().Lv(.35f).C2(12).Cut(.3f).Pcm(PcmNone,0).A(.01f,.3f,.8f,.2f), .2f, .25f);
    add("Growl Seq",       "Bass", rBass().W(0,1).Cut(.3f).Res(.5f).FEnv(.8f).F(.002f,.18f,.1f,.1f).C2(-12).St(2), Recipe().Off(), .2f, .2f);
    // --- Leads (5)
    add("Silver Lead",     "Lead", rLead(), rLead().Lv(.5f).C2(12).Pcm(PcmNone,0).Cut(.55f), .35f, .35f);
    add("Sync Blade",      "Lead", rLead().St(2).C2(7).Cut(.7f).FEnv(.4f).Res(.25f), Recipe().Off(), .3f, .3f);
    add("Whistle Wire",    "Lead", rLead().W(1,1).Cut(.85f).Res(.05f).A(.04f,.3f,.9f,.3f).Lfo(5.8f,.1f,0,.9f).Pcm(PcmBreath,.2f), Recipe().Off(), .25f, .35f);
    add("Flutey Lead",     "Lead", rLead().W(1,0).Cut(.5f).Pcm(PcmChiff,.5f).Lfo(5.5f,.08f,0,.5f), rLead().Lv(.4f).C2(12).Cut(.5f).Pcm(PcmNone,0), .3f, .4f);
    add("Ring Comet",      "Lead", rLead().St(1).C2(7).Cut(.65f).Det(0), rLead().Lv(.3f).Pcm(PcmNone,0), .35f, .4f);
    // --- Voices (4)
    add("Choir Loft",      "Voice", rChoir(), rChoir().Lv(.6f).C2(12).Pcm(PcmNone,0), .5f, .75f);
    add("Breathy Ah",      "Voice", rChoir().Cut(.45f).Pcm(PcmAh,.7f), rPad().Lv(.3f).Pcm(PcmBreath,.5f), .4f, .6f);
    add("Angel Air",       "Voice", rChoir().C2(12).Cut(.6f).A(.8f,1,.9f,2), rPad().Lv(.4f).W(0,1).Cut(.6f).C2(12).Pcm(PcmGlass,.3f), .6f, .8f);
    add("Monk Hum",        "Voice", rChoir().Cut(.25f).Pcm(PcmAh,.4f).A(.5f,1,.9f,1.2f), rPad().Lv(.5f).C2(-12).Cut(.25f), .3f, .7f);
    // --- FX / Atmospheres (5)
    add("Wind Chasm",      "FX", rFx().Pcm(PcmBreath,.9f).Cut(.55f).Res(.45f).FEnv(.5f).F(3,3,.5f,3).A(2,1,.7f,3).Lfo(.3f,0,.5f,0), Recipe().Off(), .5f, .8f);
    add("Tidal Noise",     "FX", rFx().Pcm(PcmBreath,.8f).Lfo(.15f,0,.6f,0).Cut(.4f), rPad().Lv(.4f).Cut(.3f), .6f, .8f);
    add("Sci-Fi Drip",     "FX", rPluck().St(1).C2(5).Cut(.6f).Res(.3f).FEnv(.6f).F(.002f,.3f,0,.2f).A(.002f,.5f,0,.4f).Pcm(PcmStrike,.4f), Recipe().Off(), .4f, .6f);
    add("Rising Tension",  "FX", rFx().Cut(.15f).FEnv(.9f).F(6,2,.8f,2).A(3,1,.9f,3).Res(.4f).Pcm(PcmNone,0).Det(40), rStrings().Lv(.4f).Cut(.3f).A(3,1,.9f,3), .5f, .7f);
    add("Deep Space Hum",  "FX", rPad().Cut(.2f).Res(.35f).Det(30).A(3.5f,1,.9f,5).F(3,3,.5f,4).Lfo(.1f,0,.3f,0), rFx().Lv(.4f).Pcm(PcmBreath,.5f).Cut(.3f), .7f, .9f);
    // --- Extra (4)
    add("Pizzicato Frost", "Pluck", rPluck().W(0,0).Cut(.45f).Pcm(PcmPluck,.9f).A(.002f,.25f,0,.15f), rBell().Lv(.2f).C2(24).Pcm(PcmNone,0), .3f, .45f);
    add("Harp Cascade",    "Pluck", rPluck().Cut(.6f).Pcm(PcmPluck,.6f).A(.002f,2.2f,0,1.2f).F(.003f,1,.1f,.5f).Det(8), rBell().Lv(.3f).C2(12).Pcm(PcmGlass,.3f), .5f, .75f);
    add("Dream Flute",     "Lead",  rLead().W(1,0).Cut(.4f).Pcm(PcmChiff,.6f).A(.12f,.4f,.85f,.4f).Lfo(4.8f,.05f,0,.9f), rPad().Lv(.3f).Cut(.4f).Pcm(PcmBreath,.3f), .4f, .6f);
    add("Hollow Organ",    "Keys",  Recipe().W(1,1).C2(12).Cut(.45f).Res(.05f).Det(4).KF(.7f).F(.01f,.1f,1,.1f).A(.01f,.1f,1,.08f).Lfo(6,.02f,0,0).Pcm(PcmNone,0),
                                    Recipe().W(1,1).C2(19).Cut(.4f).Res(.05f).Det(4).Lv(.5f).A(.01f,.1f,1,.08f).F(.01f,.1f,1,.1f).Pcm(PcmNone,0), .3f, .4f);
    return out;
}

} // namespace d50
