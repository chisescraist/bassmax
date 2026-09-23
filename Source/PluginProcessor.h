#pragma once
#include <JuceHeader.h>
#include <array>
#include <vector>
struct DrumHit { int step=0, note=36, velocity=100; };
struct DrumLane { juce::String name; int midiNote=36; bool enabled=true, locked=false; int sound=0; float level=0.65f; std::vector<DrumHit> hits; };
class BeatMaxProcessor final : public juce::AudioProcessor {
public:
 BeatMaxProcessor();
 const juce::String getName() const override { return "ChisesCraist BeatMax"; }
 void prepareToPlay(double,int) override; void releaseResources() override {}
 void processBlock(juce::AudioBuffer<float>&,juce::MidiBuffer&) override;
 juce::AudioProcessorEditor* createEditor() override; bool hasEditor() const override {return true;}
 bool acceptsMidi() const override {return true;} bool producesMidi() const override {return true;}
 bool isMidiEffect() const override {return false;} double getTailLengthSeconds() const override {return 0;}
 int getNumPrograms() override {return 1;} int getCurrentProgram() override {return 0;}
 void setCurrentProgram(int) override {} const juce::String getProgramName(int) override {return {};}
 void changeProgramName(int,const juce::String&) override {}
 void getStateInformation(juce::MemoryBlock&) override; void setStateInformation(const void*,int) override;
 void generate(int onlyLane=-1); bool writeMidi(const juce::File&,int lane=-1) const;
 std::array<DrumLane,9> lanes; int style=0,bars=4,bpm=126,seed=1234; float swing=0.12f, variation=0.35f; bool preview=true;
 juce::CriticalSection lock;
private:
 double sr=44100, phase=0, lastPpq=-1; std::array<double,9> env{}; std::array<double,9> tonePhase{};
 JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BeatMaxProcessor)
};
