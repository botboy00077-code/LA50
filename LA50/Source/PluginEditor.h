#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"

// A titled group of auto-generated controls (rotary sliders, or combo boxes for choice params).
class ParamPanel : public juce::Component
{
public:
    using IdLabel = std::pair<juce::String, juce::String>;
    using SliderAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAtt  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    ParamPanel (juce::AudioProcessorValueTreeState& state, const juce::String& panelTitle,
                const std::vector<IdLabel>& ids)
        : title (panelTitle)
    {
        for (auto& [id, label] : ids)
        {
            auto c = std::make_unique<Ctl>();
            c->label = std::make_unique<juce::Label>();
            c->label->setText (label, juce::dontSendNotification);
            c->label->setJustificationType (juce::Justification::centred);
            c->label->setFont (juce::FontOptions (11.f));
            addAndMakeVisible (*c->label);

            if (auto* cp = dynamic_cast<juce::AudioParameterChoice*> (state.getParameter (id)))
            {
                c->combo = std::make_unique<juce::ComboBox>();
                c->combo->addItemList (cp->choices, 1);
                addAndMakeVisible (*c->combo);
                c->ca = std::make_unique<ComboAtt> (state, id, *c->combo);
            }
            else
            {
                c->slider = std::make_unique<juce::Slider> (juce::Slider::RotaryHorizontalVerticalDrag,
                                                            juce::Slider::TextBoxBelow);
                c->slider->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 14);
                addAndMakeVisible (*c->slider);
                c->sa = std::make_unique<SliderAtt> (state, id, *c->slider);
            }
            ctls.push_back (std::move (c));
        }
    }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat().reduced (2.f);
        g.setColour (juce::Colour (0xff22262e));
        g.fillRoundedRectangle (r, 8.f);
        g.setColour (juce::Colour (0xff3b4350));
        g.drawRoundedRectangle (r, 8.f, 1.f);
        g.setColour (juce::Colour (0xff7fd1c4));
        g.setFont (juce::FontOptions (13.f, juce::Font::bold));
        g.drawText (title, 14, 6, getWidth() - 28, 18, juce::Justification::centredLeft);
    }

    void resized() override
    {
        const int cellW = 68, cellH = 92, top = 26, margin = 8;
        const int cols = std::max (1, (getWidth() - 2 * margin) / cellW);
        int i = 0;
        for (auto& c : ctls)
        {
            const int col = i % cols, row = i / cols;
            juce::Rectangle<int> cell (margin + col * cellW, top + row * cellH, cellW, cellH);
            c->label->setBounds (cell.removeFromTop (14));
            if (c->slider) c->slider->setBounds (cell);
            if (c->combo)  c->combo->setBounds (cell.withSizeKeepingCentre (cellW - 6, 24));
            ++i;
        }
    }

private:
    struct Ctl
    {
        std::unique_ptr<juce::Label> label;
        std::unique_ptr<juce::Slider> slider;
        std::unique_ptr<juce::ComboBox> combo;
        std::unique_ptr<SliderAtt> sa;
        std::unique_ptr<ComboAtt> ca;
    };
    juce::String title;
    std::vector<std::unique_ptr<Ctl>> ctls;
};

class ToneTab : public juce::Component
{
public:
    ToneTab (juce::AudioProcessorValueTreeState& s, const juce::String& id, const juce::String& name)
        : tone (s, name, toneIds (id)),
          p1 (s, name + " - Partial 1", partialIds (id, 1)),
          p2 (s, name + " - Partial 2", partialIds (id, 2))
    {
        addAndMakeVisible (tone); addAndMakeVisible (p1); addAndMakeVisible (p2);
    }
    void resized() override
    {
        auto r = getLocalBounds();
        tone.setBounds (r.removeFromTop (124));
        p1.setBounds (r.removeFromTop (r.getHeight() / 2));
        p2.setBounds (r);
    }
private:
    using V = std::vector<ParamPanel::IdLabel>;
    static V toneIds (const juce::String& t)
    {
        return { { t + "_struct", "Structure" }, { t + "_pcm", "Attack" }, { t + "_pcmlvl", "Atk Level" },
                 { t + "_level", "Level" }, { t + "_pan", "Pan" }, { t + "_lforate", "LFO Rate" },
                 { t + "_lfopitch", "LFO Pitch" }, { t + "_lfofilt", "LFO Filter" }, { t + "_lfodelay", "LFO Delay" } };
    }
    static V partialIds (const juce::String& t, int p)
    {
        const juce::String q = t + juce::String (p) + "_";
        return { { q + "wave", "Wave" }, { q + "pw", "PW" }, { q + "pwlfo", "PWM" }, { q + "coarse", "Coarse" },
                 { q + "fine", "Fine" }, { q + "level", "Level" }, { q + "cutoff", "Cutoff" }, { q + "res", "Reso" },
                 { q + "fenv", "Filt Env" }, { q + "kf", "Key Follow" },
                 { q + "fa", "F Attack" }, { q + "fd", "F Decay" }, { q + "fs", "F Sustain" }, { q + "fr", "F Release" },
                 { q + "aa", "A Attack" }, { q + "ad", "A Decay" }, { q + "as", "A Sustain" }, { q + "ar", "A Release" } };
    }
    ParamPanel tone, p1, p2;
};

class GlobalTab : public juce::Component
{
public:
    explicit GlobalTab (juce::AudioProcessorValueTreeState& s)
        : panel (s, "Global / Effects",
                 { { "mode", "Key Mode" }, { "split", "Split Pt" }, { "bend", "Bend" }, { "master", "Master" },
                   { "chorusrate", "Chorus Rate" }, { "chorusdepth", "Chorus Depth" }, { "chorusmix", "Chorus Mix" },
                   { "revsize", "Reverb Size" }, { "revmix", "Reverb Mix" } })
    { addAndMakeVisible (panel); }
    void resized() override { panel.setBounds (getLocalBounds().removeFromTop (240)); }
private:
    ParamPanel panel;
};

class LA50Editor : public juce::AudioProcessorEditor
{
public:
    explicit LA50Editor (LA50AudioProcessor&);
    ~LA50Editor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void selectPreset (int index);

    LA50AudioProcessor& proc;
    juce::ComboBox presetBox;
    juce::TextButton prevButton { "<" }, nextButton { ">" };
    juce::TabbedComponent tabs { juce::TabbedButtonBar::TabsAtTop };
    juce::LookAndFeel_V4 laf;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LA50Editor)
};
