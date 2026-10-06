#pragma once
#include <JuceHeader.h>
class FilterProcessor {
public:
 void prepare(const juce::dsp::ProcessSpec&s){sr=s.sampleRate;f.prepare(s);setEnabled(false);}
 void reset(){f.reset();}
 void setEnabled(bool e){*f.state=*juce::dsp::IIR::Coefficients<float>::makeLowPass(sr,e?1250.0:20000.0);}
 void process(juce::dsp::ProcessContextReplacing<float>&c){f.process(c);}
private:
 double sr=44100;
 juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>,juce::dsp::IIR::Coefficients<float>> f;
};