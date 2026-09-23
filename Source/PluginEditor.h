#pragma once
#include "PluginProcessor.h"
#include <juce_gui_extra/juce_gui_extra.h>
#include <array>
#include <memory>

class BassMaxDragMidiButton final : public juce::TextButton
{
public:
    BassMaxDragMidiButton() : juce::TextButton("DRAG MIDI  >>") {}
    std::function<void()> onDragMidi;
    void mouseDown(const juce::MouseEvent& e) override
    {
        startedDrag = false;
        juce::TextButton::mouseDown(e);
    }
    void mouseDrag(const juce::MouseEvent& e) override
    {
        if (!startedDrag && e.getDistanceFromDragStart() > 7)
        {
            startedDrag = true;
            if (onDragMidi) onDragMidi();
        }
        if (!startedDrag) juce::TextButton::mouseDrag(e);
    }
private:
    bool startedDrag = false;
};

class BassMaxKnobEditor final : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit BassMaxKnobEditor(TechHouseBassLab&);
    ~BassMaxKnobEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;
private:
    void timerCallback() override;
    int editSource = -1, editTarget = -1, editPitch = 0;
    int stepAt(juce::Point<float>) const;
    int pitchAt(juce::Point<float>) const;
    std::unique_ptr<juce::LookAndFeel_V4> vintageLook;
    TechHouseBassLab& processor;
    using Attachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::array<juce::Slider, 19> knobs;
    std::array<juce::Label, 19> captions;
    std::array<std::unique_ptr<Attachment>, 19> attachments;
    juce::TextButton generateButton { "GENERATE BASS" };
    juce::TextButton exportButton { "EXPORT MIDI" };
    juce::TextButton responseButton { "RESPONSE" };
    juce::ComboBox phraseChoice;
    juce::TextButton undoButton { "UNDO GENERATE" };
    juce::TextButton redoButton { "REDO" };
    BassMaxDragMidiButton dragButton;
    juce::Label dragStatus;
    void startMidiDrag();
    juce::TextButton previewButton { "PREVIEW: ON" };
    juce::ComboBox barsChoice;
    juce::ComboBox styleChoice;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> styleAttachment;
    juce::ComboBox pageChoice;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> barsAttachment;
    std::shared_ptr<juce::FileChooser> exportChooser;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BassMaxKnobEditor)
};
