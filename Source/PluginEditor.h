#pragma once
#include "PluginProcessor.h"
class BeatMaxEditor final : public juce::AudioProcessorEditor, private juce::Timer {
public:
 explicit BeatMaxEditor(BeatMaxProcessor&); ~BeatMaxEditor() override=default;
 void paint(juce::Graphics&) override; void resized() override; void mouseDown(const juce::MouseEvent&) override;
private:
 BeatMaxProcessor& processor;
 juce::ComboBox style,length;juce::Slider bpm,swing,variation;
 juce::TextButton generate{"GENERATE BEAT"},exportFull{"EXPORT FULL MIDI"},dragFull{"DRAG FULL MIDI  >>"},exportAll{"EXPORT ALL TRACKS"},preview{"PREVIEW ON"};
 struct LaneControls {juce::ToggleButton enable,lock;juce::ComboBox sound;juce::TextButton regenerate{"RANDOM"},drag{"DRAG MIDI"};juce::Slider level;};
 std::array<LaneControls,9> controls;
 juce::File dragFile(int lane); void exportMidi(int lane); void dragMidi(int lane);void refresh();void timerCallback() override;
 JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BeatMaxEditor)
};
