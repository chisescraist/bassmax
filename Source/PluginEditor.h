#pragma once
#include "PluginProcessor.h"
#include <juce_gui_extra/juce_gui_extra.h>
#include <array>
#include <memory>

class BassMaxKnobEditor final : public juce::AudioProcessorEditor
{
public:
    explicit BassMaxKnobEditor(TechHouseBassLab&);
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    TechHouseBassLab& processor;
    using Attachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::array<juce::Slider, 11> knobs;
    std::array<juce::Label, 11> captions;
    std::array<std::unique_ptr<Attachment>, 11> attachments;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BassMaxKnobEditor)
};
