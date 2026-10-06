#include "PluginProcessor.h"
#include "PluginEditor.h"

ChisescraistVOXAudioProcessor::ChisescraistVOXAudioProcessor()
: AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                  .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
  apvts(*this, nullptr, "PARAMETERS", createParameterLayout()) {}

juce::AudioProcessorValueTreeState::ParameterLayout
ChisescraistVOXAudioProcessor::createParameterLayout()
{
    using APF=juce::AudioParameterFloat; using APB=juce::AudioParameterBool;
    using APC=juce::AudioParameterChoice; using R=juce::NormalisableRange<float>;
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
    p.push_back(std::make_unique<APF>("input","Input",R(-24,24,0.1f),0));
    p.push_back(std::make_unique<APF>("drive","Drive",R(0,100,0.1f),12));
    p.push_back(std::make_unique<APF>("dark","Dark",R(0,100,0.1f),0));
    p.push_back(std::make_unique<APF>("comp","Comp",R(0,100,0.1f),25));
    p.push_back(std::make_unique<APF>("delay","Delay",R(0,100,0.1f),0));
    p.push_back(std::make_unique<APF>("reverb","Reverb",R(0,100,0.1f),8));
    p.push_back(std::make_unique<APF>("output","Output",R(-24,12,0.1f),-3));
    p.push_back(std::make_unique<APB>("stutter","Stutter",false));
    p.push_back(std::make_unique<APB>("throw","Throw",false));
    p.push_back(std::make_unique<APB>("filter","Filter",false));
    p.push_back(std::make_unique<APC>("character","Character",juce::StringArray{"CLEAN","DARK","RADIO"},0));
    p.push_back(std::make_unique<APC>("stutterRate","Stutter Rate",juce::StringArray{"1/4","1/8","1/16","1/32"},2));
    return {p.begin(),p.end()};
}

void ChisescraistVOXAudioProcessor::prepareToPlay(double sr,int bs)
{ voxEngine.prepare(sr,bs,getTotalNumOutputChannels()); }

bool ChisescraistVOXAudioProcessor::isBusesLayoutSupported(const BusesLayout& l) const
{
    auto in=l.getMainInputChannelSet(), out=l.getMainOutputChannelSet();
    return in==out && (out==juce::AudioChannelSet::mono() || out==juce::AudioChannelSet::stereo());
}

void ChisescraistVOXAudioProcessor::processBlock(juce::AudioBuffer<float>& b,juce::MidiBuffer& m)
{
    juce::ScopedNoDenormals n;
    double bpm=126.0;
    if(auto* ph=getPlayHead()) if(auto pos=ph->getPosition()) if(auto v=pos->getBpm()) bpm=*v;
    hostBpm.store(bpm);
    voxEngine.process(b,m,apvts,bpm);
}

void ChisescraistVOXAudioProcessor::getStateInformation(juce::MemoryBlock& d)
{ if(auto x=apvts.copyState().createXml()) copyXmlToBinary(*x,d); }

void ChisescraistVOXAudioProcessor::setStateInformation(const void* d,int n)
{ if(auto x=getXmlFromBinary(d,n)) if(x->hasTagName(apvts.state.getType())) apvts.replaceState(juce::ValueTree::fromXml(*x)); }

juce::AudioProcessorEditor* ChisescraistVOXAudioProcessor::createEditor()
{ return new ChisescraistVOXAudioProcessorEditor(*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{ return new ChisescraistVOXAudioProcessor(); }
