#pragma once
#include <JuceHeader.h>
class RadioProcessor {
public:
 void prepare(const juce::dsp::ProcessSpec&s){sr=s.sampleRate;hp.prepare(s);lp.prepare(s);
  *hp.state=*juce::dsp::IIR::Coefficients<float>::makeHighPass(sr,280.0);
  *lp.state=*juce::dsp::IIR::Coefficients<float>::makeLowPass(sr,4200.0);}
 void reset(){hp.reset();lp.reset();}
 void process(juce::dsp::ProcessContextReplacing<float>&c){hp.process(c);lp.process(c);
  auto&b=c.getOutputBlock();for(size_t ch=0;ch<b.getNumChannels();++ch){auto*d=b.getChannelPointer(ch);
  for(size_t i=0;i<b.getNumSamples();++i)d[i]=std::tanh(d[i]*2.2f)*.72f;}}
private:
 double sr=44100;
 juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>,juce::dsp::IIR::Coefficients<float>> hp,lp;
};