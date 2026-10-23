#include "PluginEditor.h"

using namespace juce;

LA50Editor::LA50Editor (LA50AudioProcessor& p) : AudioProcessorEditor (&p), proc (p)
{
    laf.setColourScheme (LookAndFeel_V4::getMidnightColourScheme());
    laf.setColour (Slider::rotarySliderFillColourId, Colour (0xff7fd1c4));
    laf.setColour (Slider::thumbColourId, Colour (0xffe8f7f4));
    setLookAndFeel (&laf);

    const auto& list = proc.presets();
    for (size_t i = 0; i < list.size(); ++i)
        presetBox.addItem (String ((int) i + 1).paddedLeft ('0', 2) + "  " + list[i].name + "   [" + list[i].category + "]", (int) i + 1);
    presetBox.setSelectedId (proc.getCurrentProgram() + 1, dontSendNotification);
    presetBox.onChange = [this] { selectPreset (presetBox.getSelectedId() - 1); };
    addAndMakeVisible (presetBox);

    prevButton.onClick = [this] { selectPreset ((proc.getCurrentProgram() + proc.getNumPrograms() - 1) % proc.getNumPrograms()); };
    nextButton.onClick = [this] { selectPreset ((proc.getCurrentProgram() + 1) % proc.getNumPrograms()); };
    addAndMakeVisible (prevButton);
    addAndMakeVisible (nextButton);

    auto bg = Colour (0xff181b21);
    tabs.addTab ("Tone A", bg, new ToneTab (proc.apvts, "a", "Tone A"), true);
    tabs.addTab ("Tone B", bg, new ToneTab (proc.apvts, "b", "Tone B"), true);
    tabs.addTab ("Global / FX", bg, new GlobalTab (proc.apvts), true);
    addAndMakeVisible (tabs);

    setSize (1060, 720);
}

LA50Editor::~LA50Editor() { setLookAndFeel (nullptr); }

void LA50Editor::selectPreset (int index)
{
    proc.setCurrentProgram (index);
    presetBox.setSelectedId (index + 1, dontSendNotification);
    proc.updateHostDisplay();
}

void LA50Editor::paint (Graphics& g)
{
    g.fillAll (Colour (0xff12141a));
    g.setColour (Colour (0xff7fd1c4));
    g.setFont (FontOptions (26.f, Font::bold));
    g.drawText ("LA-50", 18, 8, 160, 36, Justification::centredLeft);
    g.setColour (Colours::grey);
    g.setFont (FontOptions (11.f));
    g.drawText ("Linear Arithmetic Synthesizer", 18, 38, 200, 14, Justification::centredLeft);
}

void LA50Editor::resized()
{
    auto r = getLocalBounds();
    auto header = r.removeFromTop (60).reduced (14, 12);
    header.removeFromLeft (220);
    nextButton.setBounds (header.removeFromRight (40));
    header.removeFromRight (6);
    prevButton.setBounds (header.removeFromRight (40));
    header.removeFromRight (10);
    presetBox.setBounds (header);
    tabs.setBounds (r.reduced (8, 0).withTrimmedBottom (8));
}
