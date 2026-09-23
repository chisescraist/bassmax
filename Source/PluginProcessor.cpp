#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <random>
#include <cmath>
namespace { constexpr int stepsPerBar=16; int clampVel(int v){return juce::jlimit(1,127,v);} }
BeatMaxProcessor::BeatMaxProcessor():juce::AudioProcessor(juce::AudioProcessor::BusesProperties().withOutput("Output",juce::AudioChannelSet::stereo(),true)) {
 const char* names[]={"KICK","CLAP","SNARE","CLOSED HAT","OPEN HAT","SHAKER","RIM","PERC 1","PERC 2"};
 const int notes[]={36,39,38,42,46,70,37,64,65};
 for(int i=0;i<9;++i){lanes[i].name=names[i];lanes[i].midiNote=notes[i];lanes[i].enabled=i<6;}
 generate();
}
void BeatMaxProcessor::prepareToPlay(double rate,int){sr=rate;phase=0;lastPpq=-1;env.fill(0);tonePhase.fill(0);}
void BeatMaxProcessor::generate(int onlyLane){
 const juce::ScopedLock guard(lock);
 std::mt19937 rng((unsigned)++seed); std::uniform_real_distribution<float> random(0.f,1.f);
 const int count=bars*stepsPerBar;
 for(int l=0;l<9;++l){auto& lane=lanes[(size_t)l]; if((onlyLane>=0 && l!=onlyLane)||lane.locked||!lane.enabled)continue;
 lane.hits.clear();
 for(int s=0;s<count;++s){int p=s%16,b=s/16; float probability=0;
 if(l==0) probability=(p%4==0)?0.99f:((style==0 && p==15)?0.10f:0.015f);
 if(l==1) probability=(p==4||p==12)?0.97f:0.005f;
 if(l==2) probability=(p==12 && b%4==3)?0.38f:0.012f;
 if(l==3) probability=(p%4==2)?(style==1?0.85f:0.96f):(style==2?0.25f:0.09f);
 if(l==4) probability=(p==2||p==10)?(style==0?0.48f:0.66f):0.018f;
 if(l==5) probability=(style==2?((p%2)?0.83f:0.48f):((p%2)?0.57f:0.16f));
 if(l==6) probability=(style==2?(p==3||p==11?0.42f:0.08f):(p==7||p==15?0.26f:0.03f));
 if(l==7) probability=(style==2?(p==3||p==10||p==15?0.73f:0.08f):(p==6||p==14?0.28f:0.04f));
 if(l==8) probability=(style==2?(p==5||p==13?0.58f:0.07f):(p==11?0.30f:0.035f));
 if(style==1 && b%4==3 && l>=3)probability=juce::jlimit(0.f,1.f,probability+variation*0.17f);
 if(random(rng)<probability){int vel=clampVel((int)(72+random(rng)*38 + ((p%4==0)?10:0)));lane.hits.push_back({s,lane.midiNote,vel});}
 }
 }
}
bool BeatMaxProcessor::writeMidi(const juce::File& file,int onlyLane) const {
 juce::MidiMessageSequence sequence; constexpr int ppq=960; const int count=bars*16;
 auto add=[&](int l,const DrumHit& h){double tick=(double)h.step*ppq/4.0 + ((h.step%2)?swing*ppq/8.0:0.0);int note=lanes[(size_t)l].midiNote;
 auto on=juce::MidiMessage::noteOn(10,note,(juce::uint8)clampVel(h.velocity));on.setTimeStamp(tick);sequence.addEvent(on);
 auto off=juce::MidiMessage::noteOff(10,note);off.setTimeStamp(tick+ppq/8.0);sequence.addEvent(off);};
 const juce::ScopedLock guard(const_cast<juce::CriticalSection&>(lock));
 for(int l=0;l<9;++l)if((onlyLane<0||onlyLane==l)&&lanes[(size_t)l].enabled)for(const auto& h:lanes[(size_t)l].hits)add(l,h);
 auto end=juce::MidiMessage::allNotesOff(10);end.setTimeStamp((double)count*ppq/4.0);sequence.addEvent(end);sequence.updateMatchedPairs();
 juce::MidiFile midi;midi.setTicksPerQuarterNote(ppq);juce::MidiMessageSequence meta;
 auto tempo=juce::MidiMessage::tempoMetaEvent((int)std::round(60000000.0/bpm));tempo.setTimeStamp(0);meta.addEvent(tempo);
 auto time=juce::MidiMessage::timeSignatureMetaEvent(4,4);time.setTimeStamp(0);meta.addEvent(time);midi.addTrack(meta);midi.addTrack(sequence);
 auto stream=file.createOutputStream();return stream!=nullptr && midi.writeTo(*stream);
}
void BeatMaxProcessor::processBlock(juce::AudioBuffer<float>& buffer,juce::MidiBuffer& midi){
 juce::ScopedNoDenormals noDenormals;buffer.clear();midi.clear();
 const juce::ScopedTryLock guard(lock);if(!guard.isLocked())return;
 auto pos=getPlayHead()?getPlayHead()->getPosition():juce::Optional<juce::AudioPlayHead::PositionInfo>();
 const bool playing=pos && pos->getIsPlaying(); const double tempo=(pos && pos->getBpm())?*pos->getBpm():bpm;
 const double initialPpq=(pos && pos->getPpqPosition())?*pos->getPpqPosition():phase;
 const int total=bars*16; const double samplesPerStep=sr*60.0/tempo/4.0;
 for(int sample=0;sample<buffer.getNumSamples();++sample){
  const double ppq=playing?initialPpq+(sample/sr)*tempo/60.0:phase+(sample/sr)*tempo/60.0;
  const double stepFloat=ppq*4.0;const int step=(int)std::floor(stepFloat);const double fraction=stepFloat-step;
  const bool trigger=(sample==0 || (int)std::floor((playing?initialPpq+(sample-1)/sr*tempo/60.0:phase+(sample-1)/sr*tempo/60.0)*4.0)!=step);
  if(trigger){const int wrapped=((step%total)+total)%total;for(int l=0;l<9;++l){auto& lane=lanes[(size_t)l];if(!lane.enabled)continue;for(const auto& hit:lane.hits)if(hit.step==wrapped){auto on=juce::MidiMessage::noteOn(10,lane.midiNote,(juce::uint8)hit.velocity);midi.addEvent(on,sample);env[(size_t)l]=hit.velocity/127.0*lane.level;auto off=juce::MidiMessage::noteOff(10,lane.midiNote);midi.addEvent(off,juce::jmin(buffer.getNumSamples()-1,sample+(int)(samplesPerStep*0.45)));}}}
  if(preview){double mix=0;for(int l=0;l<9;++l){auto& e=env[(size_t)l];if(e<0.0001)continue;double freq=(l==0?55.0:l==1?650.0:l==2?220.0:l==3?3800.0:l==4?2400.0:l==5?3100.0:l==6?900.0:l==7?210.0:290.0);tonePhase[(size_t)l]+=freq/sr; if(tonePhase[(size_t)l]>=1)tonePhase[(size_t)l]-=1;
  double wave=(l==0||l>=7)?std::sin(juce::MathConstants<double>::twoPi*tonePhase[(size_t)l]):(tonePhase[(size_t)l]<0.5?1.0:-1.0);
  if(l>=1&&l<=6)wave*=std::sin(juce::MathConstants<double>::twoPi*tonePhase[(size_t)l]*7.13);
  mix+=e*wave*0.11;e*=std::exp(-1.0/(sr*(l==0?0.12:l==4?0.18:0.065)));}
  const float output=(float)std::tanh(mix);for(int c=0;c<buffer.getNumChannels();++c)buffer.setSample(c,sample,output);
  }
 }
 if(!playing)phase+=buffer.getNumSamples()/sr*tempo/60.0;
}
void BeatMaxProcessor::getStateInformation(juce::MemoryBlock& dest){juce::XmlElement root("BeatMax");const juce::ScopedLock guard(lock);root.setAttribute("style",style);root.setAttribute("bars",bars);root.setAttribute("bpm",bpm);root.setAttribute("seed",seed);root.setAttribute("swing",swing);root.setAttribute("variation",variation);root.setAttribute("preview",preview);for(int i=0;i<9;++i){auto* lane=root.createNewChildElement("lane");lane->setAttribute("id",i);lane->setAttribute("enabled",lanes[(size_t)i].enabled);lane->setAttribute("locked",lanes[(size_t)i].locked);lane->setAttribute("sound",lanes[(size_t)i].sound);lane->setAttribute("level",lanes[(size_t)i].level);for(auto h:lanes[(size_t)i].hits){auto* hit=lane->createNewChildElement("hit");hit->setAttribute("step",h.step);hit->setAttribute("vel",h.velocity);}}copyXmlToBinary(root,dest);}
void BeatMaxProcessor::setStateInformation(const void* data,int size){auto xml=getXmlFromBinary(data,size);if(!xml||!xml->hasTagName("BeatMax"))return;const juce::ScopedLock guard(lock);style=xml->getIntAttribute("style",0);bars=xml->getIntAttribute("bars",4);bpm=xml->getIntAttribute("bpm",126);seed=xml->getIntAttribute("seed",1234);swing=(float)xml->getDoubleAttribute("swing",0.12);variation=(float)xml->getDoubleAttribute("variation",0.35);preview=xml->getBoolAttribute("preview",true);for(auto* el:xml->getChildIterator()){int i=el->getIntAttribute("id",-1);if(i<0||i>=9)continue;auto& lane=lanes[(size_t)i];lane.enabled=el->getBoolAttribute("enabled",true);lane.locked=el->getBoolAttribute("locked",false);lane.sound=el->getIntAttribute("sound",0);lane.level=(float)el->getDoubleAttribute("level",0.65);lane.hits.clear();for(auto* h:el->getChildIterator())if(h->hasTagName("hit"))lane.hits.push_back({h->getIntAttribute("step"),lane.midiNote,h->getIntAttribute("vel",100)});}}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new BeatMaxProcessor();}
