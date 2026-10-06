#pragma once
#include <JuceHeader.h>
#include <vector>
class DelayProcessor {
public:
 void prepare(double s,int,int c){sr=s;int n=(int)std::ceil(sr*4.0);buf.assign(c,std::vector<float>(n,0));wp.assign(c,0);}
 void reset(){for(auto&b:buf)std::fill(b.begin(),b.end(),0);std::fill(wp.begin(),wp.end(),0);}
 void process(juce::AudioBuffer<float>&a,double bpm,float wet,bool thr){
  if(buf.empty())return;wet=juce::jlimit(0.f,1.f,wet);if(thr)wet=juce::jmax(wet,.55f);
  int ds=juce::jlimit(1,(int)buf[0].size()-1,(int)std::round(sr*(60.0/juce::jmax(40.0,bpm))*.75));
  float fb=thr?.48f:.32f;
  for(int ch=0;ch<juce::jmin(a.getNumChannels(),(int)buf.size());++ch){auto*d=a.getWritePointer(ch);auto&r=buf[ch];int&p=wp[ch];
   for(int i=0;i<a.getNumSamples();++i){int rp=p-ds;if(rp<0)rp+=(int)r.size();float y=r[rp],x=d[i];r[p]=x+y*fb;d[i]=x+y*wet*.75f;if(++p>=(int)r.size())p=0;}}}
private: double sr=44100;std::vector<std::vector<float>>buf;std::vector<int>wp;
};