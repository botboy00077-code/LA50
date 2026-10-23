#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "Engine.h"
#include "Presets.h"

class LA50AudioProcessor : public juce::AudioProcessor
{
public:
    LA50AudioProcessor();
    ~LA50AudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 6.0; }

    int getNumPrograms() override { return (int) presetList.size(); }
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    juce::AudioProcessorValueTreeState apvts;
    const std::vector<d50::PresetData>& presets() const { return presetList; }

private:
    void handleMidi (const juce::MidiMessage& m);

    d50::Engine engine;
    d50::Patch patch;
    juce::Reverb reverb;
    std::vector<d50::PresetData> presetList;
    int currentProgram = 0;

    std::vector<std::pair<std::atomic<float>*, float*>> floatBinds;
    std::vector<std::pair<std::atomic<float>*, int*>> intBinds;
    std::atomic<float>* revSizeParam = nullptr;
    std::atomic<float>* revMixParam = nullptr;
    std::vector<float> scratchR;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LA50AudioProcessor)
};
