#pragma once
#include <JuceHeader.h>
#include "DSP/VoxEngine.h"

class ChisescraistVOXAudioProcessor : public juce::AudioProcessor
{
public:
    ChisescraistVOXAudioProcessor();
    ~ChisescraistVOXAudioProcessor() override = default;
    void prepareToPlay(double, int) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 4.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;
    double getHostBpm() const noexcept { return hostBpm.load(); }

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    VoxEngine voxEngine;
    std::atomic<double> hostBpm {126.0};
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChisescraistVOXAudioProcessor)
};
