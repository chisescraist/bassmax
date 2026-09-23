#include "PluginEditor.h"

namespace
{
    constexpr const char* ids[] = { "run", "groove", "root", "density", "swing", "length", "seed", "steps", "variation", "gain", "tone" };
    constexpr const char* names[] = { "RUN", "GROOVE", "ROOT", "DENSITY", "SWING", "GATE", "SEED", "STEPS", "VARIATION", "GAIN", "TONE" };
    static_assert(std::size(ids) == 11);
}

BassMaxKnobEditor::BassMaxKnobEditor(TechHouseBassLab& p)
    : juce::AudioProcessorEditor(p), processor(p)
{
    setSize(740, 410);
    for (size_t i = 0; i < knobs.size(); ++i)
    {
        auto& slider = knobs[i];
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 65, 19);
        slider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xff55dbc2));
        slider.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff3b4755));
        slider.setColour(juce::Slider::thumbColourId, juce::Colours::white);
        slider.setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);
        slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        addAndMakeVisible(slider);
        captions[i].setText(names[i], juce::dontSendNotification);
        captions[i].setJustificationType(juce::Justification::centred);
        captions[i].setColour(juce::Label::textColourId, juce::Colour(0xffb5c8d3));
        addAndMakeVisible(captions[i]);
        attachments[i] = std::make_unique<Attachment>(processor.getParameters(), ids[i], slider);
    }
}

void BassMaxKnobEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff151d28));
    g.setColour(juce::Colour(0xff55dbc2));
    g.setFont(juce::Font(juce::FontOptions(15.0f, juce::Font::bold)));
    g.drawText("CHISESCRAIST AUDIO", 24, 12, getWidth()-48, 24, juce::Justification::centredLeft);
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(juce::FontOptions(27.0f, juce::Font::bold)));
    g.drawText("BASSMAX", 24, 38, getWidth()-48, 40, juce::Justification::centredLeft);
    g.setColour(juce::Colour(0xff9cabb8));
    g.setFont(juce::Font(juce::FontOptions(12.0f)));
    g.drawText("DIAGNOSTICO DE INTERFAZ  |  MOTOR V1 SIN CAMBIOS", 24, 77, getWidth()-48, 20, juce::Justification::centredLeft);
}

void BassMaxKnobEditor::resized()
{
    constexpr int columns = 6;
    constexpr int cellW = 116;
    constexpr int cellH = 137;
    for (size_t i = 0; i < knobs.size(); ++i)
    {
        const int x = 22 + static_cast<int>(i % columns) * cellW;
        const int y = 110 + static_cast<int>(i / columns) * cellH;
        knobs[i].setBounds(x, y, cellW-8, 105);
        captions[i].setBounds(x, y+105, cellW-8, 24);
    }
}
