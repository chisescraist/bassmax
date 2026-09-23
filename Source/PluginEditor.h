#pragma once
#include "PluginProcessor.h"
#include <juce_gui_extra/juce_gui_extra.h>
#include <memory>
#include <vector>

class BassMaxEditor final : public juce::AudioProcessorEditor
{
public:
    explicit BassMaxEditor(TechHouseBassLab&);
    ~BassMaxEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    TechHouseBassLab& processor;
    juce::Label heading, subheading, status;
    juce::TextButton exportButton { "EXPORT MIDI" }, generateButton { "GENERATE" };
    std::vector<std::unique_ptr<juce::Slider>> knobs;
    std::vector<std::unique_ptr<juce::Label>> knobLabels;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> attachments;
    juce::ComboBox groove, steps;
    juce::ToggleButton run { "RUN" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> grooveAttachment, stepsAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> runAttachment;
    std::shared_ptr<juce::FileChooser> chooser;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BassMaxEditor)
};
