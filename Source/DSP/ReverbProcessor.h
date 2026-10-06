#pragma once
#include <JuceHeader.h>
class ReverbProcessor {
public:
 void prepare(double sr){r.setSampleRate(sr);r.reset();}
 void reset(){r.reset();}
 void process(juce::AudioBuffer<float>&b,float wet){juce::Reverb::Parameters p;p.roomSize=.42f;p.damping=.58f;p.wetLevel=juce::jlimit(0.f,1.f,wet)*.38f;p.dryLevel=1;p.width=.85f;r.setParameters(p);
  if(b.getNumChannels()>=2)r.processStereo(b.getWritePointer(0),b.getWritePointer(1),b.getNumSamples());else if(b.getNumChannels()==1)r.processMono(b.getWritePointer(0),b.getNumSamples());}
private: juce::Reverb r;
};