#pragma once
#include <JuceHeader.h>
class DarkProcessor {
public:
 void prepare(const juce::dsp::ProcessSpec& s){sr=s.sampleRate;shelf.prepare(s);lp.prepare(s);update(0);}
 void reset(){shelf.reset();lp.reset();}
 void update(float a){a=juce::jlimit(0.f,1.f,a);
  *shelf.state=*juce::dsp::IIR::Coefficients<float>::makeLowShelf(sr,180.0,0.707,juce::Decibels::decibelsToGain(5.f*a));
  *lp.state=*juce::dsp::IIR::Coefficients<float>::makeLowPass(sr,juce::jmap(a,18000.f,4800.f));}
 void process(juce::dsp::ProcessContextReplacing<float>& c){shelf.process(c);lp.process(c);}
private:
 double sr=44100;
 juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>,juce::dsp::IIR::Coefficients<float>> shelf,lp;
};