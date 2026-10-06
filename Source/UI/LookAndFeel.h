#pragma once
#include <JuceHeader.h>
class ChisescraistLookAndFeel:public juce::LookAndFeel_V4{
public:ChisescraistLookAndFeel();void drawRotarySlider(juce::Graphics&,int,int,int,int,float,float,float,juce::Slider&)override;};