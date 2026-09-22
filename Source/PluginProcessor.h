#pragma once
#include <JuceHeader.h>
#include <array>
class TechHouseBassLab final : public juce::AudioProcessor {
public:
 TechHouseBassLab();
 const juce::String getName() const override { return "Tech House Bass Lab"; }
 void prepareToPlay(double sampleRate, int) override;
 void releaseResources() override {}
 bool isBusesLayoutSupported(const BusesLayout& l) const override {return l.getMainOutputChannelSet()==juce::AudioChannelSet::stereo();}
 void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
 juce::AudioProcessorEditor* createEditor() override {return new juce::GenericAudioProcessorEditor(*this);}
 bool hasEditor() const override {return true;}
 double getTailLengthSeconds() const override {return 0.2;}
 bool acceptsMidi() const override {return true;}
 bool producesMidi() const override {return true;}
 bool isMidiEffect() const override {return false;}
 int getNumPrograms() override {return 1;}
 int getCurrentProgram() override {return 0;}
 void setCurrentProgram(int) override {}
 const juce::String getProgramName(int) override {return {};}
 void changeProgramName(int,const juce::String&) override {}
 void getStateInformation(juce::MemoryBlock&) override;
 void setStateInformation(const void*,int) override;
private:
 juce::AudioProcessorValueTreeState state;
 static juce::AudioProcessorValueTreeState::ParameterLayout makeParams();
 double sr=44100.0, phase=0.0, env=0.0, lastPpq=-1.0;
 int activeNote=-1, activeStep=-1, previousSeed=-1;
 std::array<int,32> pitches{};
 std::array<bool,32> gates{};
 std::array<int,32> velocities{};
 juce::Random random;
 void regenerate(int seed);
 void noteOff(juce::MidiBuffer&,int);
 JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TechHouseBassLab)
};
