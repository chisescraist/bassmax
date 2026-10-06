#include "VoxEngine.h"
void VoxEngine::prepare(double sr,int bs,int ch){juce::dsp::ProcessSpec s{sr,(juce::uint32)bs,(juce::uint32)ch};comp.prepare(s);comp.setAttack(4);comp.setRelease(75);dark.prepare(s);radio.prepare(s);filter.prepare(s);stutter.prepare(sr,ch);delay.prepare(sr,bs,ch);reverb.prepare(sr);}
void VoxEngine::reset(){comp.reset();dark.reset();radio.reset();filter.reset();stutter.reset();delay.reset();reverb.reset();}
void VoxEngine::process(juce::AudioBuffer<float>&a,juce::MidiBuffer&m,juce::AudioProcessorValueTreeState&s,double bpm){
 juce::ignoreUnused(m);auto v=[&](const char*id){return s.getRawParameterValue(id)->load();};
 a.applyGain(juce::Decibels::decibelsToGain(v("input")));
 float ca=v("comp")/100.f;comp.setThreshold(juce::jmap(ca,-6.f,-30.f));comp.setRatio(juce::jmap(ca,1.5f,6.f));
 {juce::dsp::AudioBlock<float>b(a);juce::dsp::ProcessContextReplacing<float>c(b);comp.process(c);}
 float dr=v("drive")/100.f;if(dr>.0001f){float g=1+dr*5,n=1/std::tanh(g);for(int ch=0;ch<a.getNumChannels();++ch){auto*d=a.getWritePointer(ch);for(int i=0;i<a.getNumSamples();++i)d[i]=std::tanh(d[i]*g)*n;}}
 int character=(int)v("character");float da=juce::jlimit(0.f,1.f,v("dark")/100.f);
 if(character==1||da>.001f){dark.update(character==1?juce::jmax(.65f,da):da);juce::dsp::AudioBlock<float>b(a);juce::dsp::ProcessContextReplacing<float>c(b);dark.process(c);}
 if(character==2){juce::dsp::AudioBlock<float>b(a);juce::dsp::ProcessContextReplacing<float>c(b);radio.process(c);}
 filter.setEnabled(v("filter")>.5f);{juce::dsp::AudioBlock<float>b(a);juce::dsp::ProcessContextReplacing<float>c(b);filter.process(c);}
 stutter.process(a,v("stutter")>.5f,(int)v("stutterRate"),bpm);delay.process(a,bpm,v("delay")/100.f,v("throw")>.5f);reverb.process(a,v("reverb")/100.f);
 a.applyGain(juce::Decibels::decibelsToGain(v("output")));
}