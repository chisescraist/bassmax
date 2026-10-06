#pragma once
#include <JuceHeader.h>
#include <vector>
class StutterProcessor {
public:
 void prepare(double s,int c){sr=s;int n=(int)std::ceil(sr*2);cap.assign(c,std::vector<float>(n,0));}
 void reset(){for(auto&v:cap)std::fill(v.begin(),v.end(),0);was=false;capturing=false;pos=play=0;}
 void process(juce::AudioBuffer<float>&a,bool en,int idx,double bpm){
  if(cap.empty())return;double beats[]={1,.5,.25,.125};idx=juce::jlimit(0,3,idx);
  len=juce::jlimit(1,(int)cap[0].size(),(int)std::round(sr*(60.0/juce::jmax(40.0,bpm))*beats[idx]));
  if(en&&!was){pos=play=0;capturing=true;}
  for(int i=0;i<a.getNumSamples();++i)if(en){if(capturing){for(int ch=0;ch<juce::jmin(a.getNumChannels(),(int)cap.size());++ch)cap[ch][pos]=a.getSample(ch,i);if(++pos>=len){capturing=false;play=0;}}
  else{for(int ch=0;ch<juce::jmin(a.getNumChannels(),(int)cap.size());++ch)a.setSample(ch,i,cap[ch][play]);if(++play>=len)play=0;}}
  if(!en)capturing=false;was=en;}
private:double sr=44100;std::vector<std::vector<float>>cap;bool was=false,capturing=false;int pos=0,play=0,len=1;
};