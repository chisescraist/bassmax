#pragma once
#include <JuceHeader.h>
#include "DarkProcessor.h"
#include "RadioProcessor.h"
#include "StutterProcessor.h"
#include "DelayProcessor.h"
#include "ReverbProcessor.h"
#include "FilterProcessor.h"
class VoxEngine {
public:
 void prepare(double,int,int); void reset();
 void process(juce::AudioBuffer<float>&,juce::MidiBuffer&,juce::AudioProcessorValueTreeState&,double);
private:
 juce::dsp::Compressor<float> comp; DarkProcessor dark; RadioProcessor radio;
 StutterProcessor stutter; DelayProcessor delay; ReverbProcessor reverb; FilterProcessor filter;
};