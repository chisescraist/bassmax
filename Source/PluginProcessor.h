
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>

#include <array>

class TechHouseBassLab final : public juce::AudioProcessor
{
public:
    TechHouseBassLab();

    const juce::String getName() const override
    {
        return "ChisesCraist BassMax MIDI OUT 1.2";
    }

    void prepareToPlay(double sampleRate, int) override;

    void releaseResources() override {}

    bool isBusesLayoutSupported(const BusesLayout& layout) const override
    {
        return layout.getMainOutputChannelSet()
            == juce::AudioChannelSet::stereo();
    }

    void processBlock(
        juce::AudioBuffer<float>&,
        juce::MidiBuffer&
    ) override;

    juce::AudioProcessorEditor* createEditor() override;

    juce::AudioProcessorValueTreeState& getParameters() noexcept { return state; }
    void generateNewPattern();
    bool exportMidi(const juce::File& file);
    int getPatternSeed() const noexcept { return static_cast<int>(state.getRawParameterValue("seed")->load()); }


    bool hasEditor() const override
    {
        return true;
    }

    double getTailLengthSeconds() const override
    {
        return 0.2;
    }

    bool acceptsMidi() const override
    {
        return true;
    }

    bool producesMidi() const override
    {
        return true;
    }

    bool isMidiEffect() const override
    {
        return false;
    }

    int getNumPrograms() override
    {
        return 1;
    }

    int getCurrentProgram() override
    {
        return 0;
    }

    void setCurrentProgram(int) override {}

    const juce::String getProgramName(int) override
    {
        return {};
    }

    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock&) override;

    void setStateInformation(const void*, int) override;

private:
    juce::AudioProcessorValueTreeState state;

    static juce::AudioProcessorValueTreeState::ParameterLayout
        makeParams();

    double sr = 44100.0;
    double phase = 0.0;
    double env = 0.0;
    double lastPpq = -1.0;

    int activeNote = -1;
    int activeStep = -1;
    int previousSeed = -1;
    juce::CriticalSection patternLock;

    std::array<int, 32> pitches{};
    std::array<bool, 32> gates{};
    std::array<int, 32> velocities{};

    juce::Random random;

    void regenerate(int seed);

    void noteOff(juce::MidiBuffer&, int);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TechHouseBassLab)
};
