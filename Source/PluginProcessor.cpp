#include "PluginProcessor.h"
#include <cmath>
#include <algorithm>
namespace {constexpr double pi=3.14159265358979323846;}
juce::AudioProcessorValueTreeState::ParameterLayout TechHouseBassLab::makeParams(){
 std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
 auto f=[&](const char* id,const char* name,float lo,float hi,float def,float step=1.0f){p.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{id,1},name,juce::NormalisableRange<float>(lo,hi,step),def));};
 f("run","RUN (0 off / 1 on)",0,1,1);f("groove","Groove: 0 Rolling 1 Offbeat 2 Syncopated 3 Minimal",0,3,0);
 f("root","Root MIDI note (F1 = 29)",24,60,29);f("density","Density percent",0,100,65);
 f("swing","Swing percent",0,70,15);f("length","Gate percent",10,95,48);f("seed","Generate: change seed",0,9999,1);
 f("steps","Pattern length (16 or 32)",16,32,16,16);f("variation","Pitch variation percent",0,100,35);
 f("gain","Internal bass gain",0,1,0.3f,0.01f);f("tone","Internal bass tone",0,1,0.35f,0.01f);
 return {p.begin(),p.end()};
}
TechHouseBassLab::TechHouseBassLab():AudioProcessor(BusesProperties().withOutput("Output",juce::AudioChannelSet::stereo(),true)),state(*this,nullptr,"PARAMS",makeParams()){}
void TechHouseBassLab::prepareToPlay(double sampleRate,int){sr=sampleRate;phase=env=0;activeNote=activeStep=-1;lastPpq=-1;previousSeed=-1;}
void TechHouseBassLab::regenerate(int seed){
 random.setSeed(seed);int groove=(int)state.getRawParameterValue("groove")->load();
 int root=(int)state.getRawParameterValue("root")->load();int density=(int)state.getRawParameterValue("density")->load();
 int variation=(int)state.getRawParameterValue("variation")->load();
 static const bool bases[4][16]={{1,0,0,1,0,0,1,0,1,0,1,0,0,0,1,0},{0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0},{1,0,0,1,0,0,0,1,0,0,1,0,0,1,0,1},{0,0,1,0,0,0,0,1,0,0,0,1,0,0,1,0}};
 static const int offsets[]={0,0,0,7,0,0,3,0,0,0,7,0,0,3,0,0};
 for(int i=0;i<32;++i){bool base=bases[groove][i%16];gates[i]=base?(random.nextInt(100)<std::min(100,density+35)):(random.nextInt(100)<std::max(0,density-55));
 int off=(random.nextInt(100)<variation)?offsets[i%16]:0;pitches[i]=root+off;velocities[i]=juce::jlimit(35,127,85+(i%4==0?22:0)+random.nextInt(17)-8);}
}
void TechHouseBassLab::noteOff(juce::MidiBuffer& midi,int offset){if(activeNote>=0){midi.addEvent(juce::MidiMessage::noteOff(1,activeNote),offset);activeNote=-1;}}
void TechHouseBassLab::processBlock(juce::AudioBuffer<float>& audio,juce::MidiBuffer& midi){
 juce::ScopedNoDenormals noDenormals;audio.clear();
 int n=audio.getNumSamples();if(n==0)return;
 juce::MidiBuffer out;for(const auto meta:midi)out.addEvent(meta.getMessage(),meta.samplePosition);
 const bool run=state.getRawParameterValue("run")->load()>0.5f;
 const int seed=(int)state.getRawParameterValue("seed")->load();if(seed!=previousSeed){regenerate(seed);previousSeed=seed;}
 juce::AudioPlayHead::CurrentPositionInfo pos;bool havePos=false;
 if(auto* playHead=getPlayHead()){if(auto position=playHead->getPosition()){
 if(auto ppq=position->getPpqPosition()){pos.ppqPosition=*ppq;havePos=true;}
 if(auto bpm=position->getBpm())pos.bpm=*bpm;
 if(auto playing=position->getIsPlaying())pos.isPlaying=*playing;
 }}
 if(!run||!havePos||!pos.isPlaying){noteOff(out,0);activeStep=-1;lastPpq=-1;midi.swapWith(out);env=0;return;}
 double bpm=pos.bpm>1?pos.bpm:126.0;
 const double ppqPerSample=bpm/(60.0*sr);
 const double swing=state.getRawParameterValue("swing")->load()/100.0;
 const double gateLength=state.getRawParameterValue("length")->load()/100.0;
 const int length=(int)state.getRawParameterValue("steps")->load();
 const double gain=state.getRawParameterValue("gain")->load();
 const double tone=state.getRawParameterValue("tone")->load();
 for(int i=0;i<n;++i){
  double ppq=pos.ppqPosition+i*ppqPerSample;
  if(lastPpq>=0&&(ppq<lastPpq-0.01||ppq-lastPpq>1.0)){noteOff(out,i);activeStep=-1;}
  lastPpq=ppq;
  // Odd 16th notes are delayed by up to 70% of a 16th note.
  double cycle=std::floor(ppq/4.0)*4.0;double local=ppq-cycle;
  int raw=(int)std::floor(local*4.0);int step=raw;
  if(raw%2==1&&local*4.0-raw<swing)step=raw-1;
  int globalStep=(int)(std::floor(ppq/4.0)*16.0)+step;
  int index=((globalStep%length)+length)%length;
  double stepStart=(double)globalStep/4.0+((globalStep%2)!=0?swing/4.0:0.0);
  if(index!=activeStep){noteOff(out,i);activeStep=index;
   if(gates[index]){activeNote=pitches[index];out.addEvent(juce::MidiMessage::noteOn(1,activeNote,(juce::uint8)velocities[index]),i);env=1.0;}
  }
  if(activeNote>=0&&ppq-stepStart>=gateLength*0.25){noteOff(out,i);}
  double hz=activeNote>=0?juce::MidiMessage::getMidiNoteInHertz(activeNote):0;
  if(activeNote>=0)phase+=hz/sr;
  phase-=std::floor(phase);
  env*=std::exp(-1.0/(sr*0.11));
  double saw=2.0*phase-1.0;double sine=std::sin(2.0*pi*phase);
  float sample=(float)((sine*(1.0-tone)+saw*tone)*env*gain);
  for(int ch=0;ch<audio.getNumChannels();++ch)audio.setSample(ch,i,sample);
 }
 midi.swapWith(out);
}
void TechHouseBassLab::getStateInformation(juce::MemoryBlock& dest){auto xml=state.copyState().createXml();copyXmlToBinary(*xml,dest);}
void TechHouseBassLab::setStateInformation(const void* data,int size){auto xml=getXmlFromBinary(data,size);if(xml&&xml->hasTagName(state.state.getType()))state.replaceState(juce::ValueTree::fromXml(*xml));previousSeed=-1;}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new TechHouseBassLab();}
