#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "UI/LookAndFeel.h"
class ChisescraistVOXAudioProcessorEditor:public juce::AudioProcessorEditor,private juce::Timer{
public:explicit ChisescraistVOXAudioProcessorEditor(ChisescraistVOXAudioProcessor&);~ChisescraistVOXAudioProcessorEditor()override;void paint(juce::Graphics&)override;void resized()override;
private:
 using SA=juce::AudioProcessorValueTreeState::SliderAttachment;using BA=juce::AudioProcessorValueTreeState::ButtonAttachment;using CA=juce::AudioProcessorValueTreeState::ComboBoxAttachment;
 void timerCallback()override;void knob(juce::Slider&,juce::Label&,const juce::String&);void button(juce::TextButton&);
 ChisescraistVOXAudioProcessor&p;ChisescraistLookAndFeel lnf;
 juce::Slider input,drive,dark,comp,delay,reverb,output;juce::Label il,dl,dkl,cl,dell,rl,ol,bpm;
 juce::ComboBox character;juce::TextButton st{"STUTTER"},th{"THROW"},fi{"FILTER"},q4{"1/4"},q8{"1/8"},q16{"1/16"},q32{"1/32"};
 std::unique_ptr<SA>ia,da,dka,ca,dela,ra,oa;std::unique_ptr<BA>sta,tha,fia;std::unique_ptr<CA>cha;
};