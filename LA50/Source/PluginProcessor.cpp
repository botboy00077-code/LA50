#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace juce;

namespace
{
using FRange = NormalisableRange<float>;
using Layout = AudioProcessorValueTreeState::ParameterLayout;

void addF (Layout& l, const String& id, const String& name, FRange r, float def)
{
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name, r, def));
}
void addI (Layout& l, const String& id, const String& name, int lo, int hi, int def)
{
    l.add (std::make_unique<AudioParameterInt> (ParameterID { id, 1 }, name, lo, hi, def));
}
void addC (Layout& l, const String& id, const String& name, const StringArray& choices, int def)
{
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { id, 1 }, name, choices, def));
}
} // namespace

AudioProcessorValueTreeState::ParameterLayout LA50AudioProcessor::createLayout()
{
    Layout l;
    const d50::Patch def;
    const FRange unit (0.f, 1.f), bipolar (-1.f, 1.f), timeR (0.001f, 10.f, 0.f, 0.3f);

    StringArray pcmNames;
    for (auto* n : d50::kPcmNames) pcmNames.add (n);

    for (int t = 0; t < 2; ++t)
    {
        const String id = t == 0 ? "a" : "b";
        const String nm = String ("Tone ") + id.toUpperCase() + " ";
        const auto& T = def.t[t];

        addC (l, id + "_struct", nm + "Structure", { "Mix", "Ring", "Sync" }, 0);
        addC (l, id + "_pcm", nm + "Attack Sample", pcmNames, 0);
        addF (l, id + "_pcmlvl", nm + "Attack Level", unit, T.pcmLevel);
        addF (l, id + "_lforate", nm + "LFO Rate", FRange (0.1f, 20.f, 0.f, 0.4f), T.lfoRate);
        addF (l, id + "_lfopitch", nm + "LFO Pitch", FRange (0.f, 2.f), T.lfoPitch);
        addF (l, id + "_lfofilt", nm + "LFO Filter", unit, T.lfoFilt);
        addF (l, id + "_lfodelay", nm + "LFO Delay", FRange (0.f, 5.f), T.lfoDelay);
        addF (l, id + "_level", nm + "Level", unit, t == 0 ? 1.f : 0.f);
        addF (l, id + "_pan", nm + "Pan", bipolar, 0.f);

        for (int p = 1; p <= 2; ++p)
        {
            const String q = id + String (p) + "_";
            const String pn = nm + "P" + String (p) + " ";
            const auto& P = T.p[p - 1];
            addC (l, q + "wave", pn + "Wave", { "Saw", "Square" }, 0);
            addF (l, q + "pw", pn + "Pulse Width", FRange (0.05f, 0.95f), 0.5f);
            addF (l, q + "pwlfo", pn + "PWM Depth", unit, 0.f);
            addI (l, q + "coarse", pn + "Coarse", -24, 24, 0);
            addF (l, q + "fine", pn + "Fine", FRange (-50.f, 50.f), 0.f);
            addF (l, q + "level", pn + "Level", unit, p == 1 ? 1.f : 0.f);
            addF (l, q + "cutoff", pn + "Cutoff", unit, P.cutoff);
            addF (l, q + "res", pn + "Resonance", unit, P.res);
            addF (l, q + "fenv", pn + "Filter Env", bipolar, 0.f);
            addF (l, q + "kf", pn + "Key Follow", unit, P.kf);
            addF (l, q + "fa", pn + "TVF Attack", timeR, 0.01f);
            addF (l, q + "fd", pn + "TVF Decay", timeR, 0.5f);
            addF (l, q + "fs", pn + "TVF Sustain", unit, 0.7f);
            addF (l, q + "fr", pn + "TVF Release", timeR, 0.3f);
            addF (l, q + "aa", pn + "TVA Attack", timeR, 0.01f);
            addF (l, q + "ad", pn + "TVA Decay", timeR, 0.5f);
            addF (l, q + "as", pn + "TVA Sustain", unit, 0.8f);
            addF (l, q + "ar", pn + "TVA Release", timeR, 0.3f);
        }
    }

    addC (l, "mode", "Key Mode", { "Single", "Dual", "Split" }, 0);
    addI (l, "split", "Split Point", 24, 96, 60);
    addF (l, "chorusrate", "Chorus Rate", FRange (0.1f, 5.f, 0.f, 0.5f), 0.8f);
    addF (l, "chorusdepth", "Chorus Depth", unit, 0.5f);
    addF (l, "chorusmix", "Chorus Mix", unit, 0.3f);
    addF (l, "revsize", "Reverb Size", unit, 0.6f);
    addF (l, "revmix", "Reverb Mix", unit, 0.25f);
    addF (l, "master", "Master", unit, 0.7f);
    addI (l, "bend", "Bend FRange", 0, 12, 2);
    return l;
}

LA50AudioProcessor::LA50AudioProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "LA50", createLayout()),
      presetList (d50::buildFactoryPresets())
{
    auto bf = [this] (const String& id, float& d) { floatBinds.push_back ({ apvts.getRawParameterValue (id), &d }); };
    auto bi = [this] (const String& id, int& d) { intBinds.push_back ({ apvts.getRawParameterValue (id), &d }); };

    for (int t = 0; t < 2; ++t)
    {
        const String id = t == 0 ? "a" : "b";
        auto& T = patch.t[t];
        bi (id + "_struct", T.structure);   bi (id + "_pcm", T.pcm);
        bf (id + "_pcmlvl", T.pcmLevel);    bf (id + "_lforate", T.lfoRate);
        bf (id + "_lfopitch", T.lfoPitch);  bf (id + "_lfofilt", T.lfoFilt);
        bf (id + "_lfodelay", T.lfoDelay);  bf (id + "_level", T.level);
        bf (id + "_pan", T.pan);
        for (int p = 1; p <= 2; ++p)
        {
            const String q = id + String (p) + "_";
            auto& P = T.p[p - 1];
            bi (q + "wave", P.wave);   bf (q + "pw", P.pw);       bf (q + "pwlfo", P.pwLfo);
            bi (q + "coarse", P.coarse); bf (q + "fine", P.fine);  bf (q + "level", P.level);
            bf (q + "cutoff", P.cutoff); bf (q + "res", P.res);    bf (q + "fenv", P.fenv);
            bf (q + "kf", P.kf);
            bf (q + "fa", P.fe.a); bf (q + "fd", P.fe.d); bf (q + "fs", P.fe.s); bf (q + "fr", P.fe.r);
            bf (q + "aa", P.ae.a); bf (q + "ad", P.ae.d); bf (q + "as", P.ae.s); bf (q + "ar", P.ae.r);
        }
    }
    bi ("mode", patch.mode);  bi ("split", patch.split);  bi ("bend", patch.bend);
    bf ("chorusrate", patch.chorusRate);  bf ("chorusdepth", patch.chorusDepth);
    bf ("chorusmix", patch.chorusMix);    bf ("master", patch.master);
    revSizeParam = apvts.getRawParameterValue ("revsize");
    revMixParam  = apvts.getRawParameterValue ("revmix");

    // Start on the first factory preset so the plugin makes sound immediately.
    setCurrentProgram (0);
}

void LA50AudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.prepare (sampleRate);
    reverb.setSampleRate (sampleRate);
    reverb.reset();
    scratchR.assign ((size_t) std::max (samplesPerBlock, 1), 0.f);
}

bool LA50AudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return layouts.getMainInputChannelSet().isDisabled()
        && (out == AudioChannelSet::stereo() || out == AudioChannelSet::mono());
}

void LA50AudioProcessor::handleMidi (const MidiMessage& m)
{
    if (m.isNoteOn())                  engine.noteOn (m.getNoteNumber(), m.getFloatVelocity());
    else if (m.isNoteOff())            engine.noteOff (m.getNoteNumber());
    else if (m.isPitchWheel())         engine.setBend ((float) (m.getPitchWheelValue() - 8192) / 8192.f);
    else if (m.isAllNotesOff() || m.isAllSoundOff()) engine.allNotesOff();
    else if (m.isController())
    {
        const int cc = m.getControllerNumber(), v = m.getControllerValue();
        if (cc == 1)       engine.setMod ((float) v / 127.f);
        else if (cc == 64) engine.setSustain (v >= 64);
    }
}

void LA50AudioProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midi)
{
    ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    const int numCh = buffer.getNumChannels();
    buffer.clear();

    for (auto& b : floatBinds) *b.second = b.first->load();
    for (auto& b : intBinds)   *b.second = (int) std::lround (b.first->load());
    engine.setPatch (patch);

    float* L = buffer.getWritePointer (0);
    float* R = nullptr;
    if (numCh > 1) R = buffer.getWritePointer (1);
    else { if ((int) scratchR.size() < n) scratchR.assign ((size_t) n, 0.f); R = scratchR.data(); }

    int pos = 0;
    for (const auto meta : midi)
    {
        const int t = std::clamp (meta.samplePosition, 0, n);
        if (t > pos) { engine.render (L + pos, R + pos, t - pos); pos = t; }
        handleMidi (meta.getMessage());
    }
    if (pos < n) engine.render (L + pos, R + pos, n - pos);

    Reverb::Parameters rp;
    const float mix = revMixParam->load();
    rp.roomSize = revSizeParam->load();
    rp.damping = 0.5f;
    rp.wetLevel = mix * 0.4f;
    rp.dryLevel = 1.f - 0.2f * mix;
    rp.width = 1.f;
    reverb.setParameters (rp);
    reverb.processStereo (L, R, n);

    if (numCh == 1)
        for (int i = 0; i < n; ++i) L[i] = 0.5f * (L[i] + R[i]);
}

void LA50AudioProcessor::setCurrentProgram (int index)
{
    if (index < 0 || index >= (int) presetList.size()) return;
    currentProgram = index;
    const auto& vals = presetList[(size_t) index].values;

    for (auto* param : getParameters())
    {
        if (auto* r = dynamic_cast<RangedAudioParameter*> (param))
        {
            auto it = vals.find (r->paramID.toStdString());
            const float norm = it != vals.end() ? r->convertTo0to1 (it->second) : r->getDefaultValue();
            r->setValueNotifyingHost (norm);
        }
    }
    apvts.state.setProperty ("program", currentProgram, nullptr);
}

const String LA50AudioProcessor::getProgramName (int index)
{
    if (index < 0 || index >= (int) presetList.size()) return {};
    return presetList[(size_t) index].name;
}

void LA50AudioProcessor::getStateInformation (MemoryBlock& destData)
{
    apvts.state.setProperty ("program", currentProgram, nullptr);
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void LA50AudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            apvts.replaceState (ValueTree::fromXml (*xml));
            currentProgram = (int) apvts.state.getProperty ("program", 0);
        }
}

AudioProcessorEditor* LA50AudioProcessor::createEditor() { return new LA50Editor (*this); }

AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new LA50AudioProcessor(); }
