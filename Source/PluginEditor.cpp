#include "PluginEditor.h"

namespace {
constexpr juce::uint32 bg = 0xff111923, panel = 0xff1c2935, accent = 0xff65e6c5;
}
BassMaxEditor::BassMaxEditor(TechHouseBassLab& p) : AudioProcessorEditor(p), processor(p)
{
    setSize(780, 520);
    heading.setText("BASSMAX 2", juce::dontSendNotification);
    heading.setFont(juce::Font(juce::FontOptions(32.0f, juce::Font::bold)));
    heading.setColour(juce::Label::textColourId, juce::Colour(accent)); addAndMakeVisible(heading);
    subheading.setText("TECH HOUSE  /  BASS + MIDI LAB", juce::dontSendNotification);
    subheading.setColour(juce::Label::textColourId, juce::Colours::lightgrey); addAndMakeVisible(subheading);
    status.setText("Exporta un .mid y arrastralo al piano roll de Ableton", juce::dontSendNotification);
    status.setColour(juce::Label::textColourId, juce::Colours::lightgrey); addAndMakeVisible(status);
    auto& params = processor.parameters();
    const char* ids[] = { "root", "density", "swing", "length", "variation", "gain", "tone" };
    const char* names[] = { "ROOT", "DENSITY", "SWING", "GATE", "VARIATION", "GAIN", "TONE" };
    for (int i = 0; i < 7; ++i)
    {
        auto slider = std::make_unique<juce::Slider>();
        slider->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider->setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 20);
        slider->setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(accent));
        slider->setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff3b4d5c));
        slider->setColour(juce::Slider::thumbColourId, juce::Colour(accent));
        slider->setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);
        slider->setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        addAndMakeVisible(*slider);
        auto label = std::make_unique<juce::Label>();
        label->setText(names[i], juce::dontSendNotification);
        label->setJustificationType(juce::Justification::centred);
        label->setColour(juce::Label::textColourId, juce::Colours::lightgrey);
        addAndMakeVisible(*label);
        attachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(params, ids[i], *slider));
        knobs.push_back(std::move(slider)); knobLabels.push_back(std::move(label));
    }
    groove.addItem("ROLLING", 1); groove.addItem("OFFBEAT", 2);
    groove.addItem("SYNCOPATED", 3); groove.addItem("MINIMAL", 4);
    grooveAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(params, "groove", groove);
    addAndMakeVisible(groove);
    steps.addItem("16 STEPS", 1); steps.addItem("32 STEPS", 2);
    stepsAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(params, "steps", steps);
    addAndMakeVisible(steps);
    runAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(params, "run", run);
    addAndMakeVisible(run);
    for (auto* b : { &exportButton, &generateButton }) {
        b->setColour(juce::TextButton::buttonColourId, juce::Colour(accent));
        b->setColour(juce::TextButton::textColourOffId, juce::Colour(bg));
        addAndMakeVisible(*b);
    }
    generateButton.onClick = [this] {
        auto* parameter = processor.parameters().getParameter("seed");
        if (parameter == nullptr) return;
        const auto next = (static_cast<int>(processor.parameters().getRawParameterValue("seed")->load()) + 1) % 10000;
        parameter->setValueNotifyingHost(parameter->convertTo0to1(static_cast<float>(next)));
        status.setText("Nuevo patron generado. Exportalo para editarlo.", juce::dontSendNotification);
    };
    exportButton.onClick = [this] {
        chooser = std::make_shared<juce::FileChooser>("Guardar patron MIDI", juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("BassMax_Pattern.mid"), "*.mid");
        chooser->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::warnAboutOverwriting,
            [this, keepAlive = chooser](const juce::FileChooser& fc) {
                auto target = fc.getResult();
                if (target == juce::File()) return;
                target = target.withFileExtension("mid");
                const bool ok = processor.exportPatternMidi(target);
                status.setText(ok ? "MIDI guardado: " + target.getFullPathName() : "No se pudo guardar el archivo MIDI", juce::dontSendNotification);
            });
    };
}
void BassMaxEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(bg));
    g.setColour(juce::Colour(panel));
    g.fillRoundedRectangle(20.0f, 92.0f, 740.0f, 326.0f, 16.0f);
    g.setColour(juce::Colour(accent));
    g.drawLine(20.0f, 83.0f, 760.0f, 83.0f, 2.0f);
}
void BassMaxEditor::resized()
{
    heading.setBounds(25, 13, 350, 43); subheading.setBounds(27, 53, 460, 23);
    run.setBounds(665, 24, 85, 35);
    groove.setBounds(36, 108, 245, 34); steps.setBounds(294, 108, 150, 34);
    for (int i = 0; i < 7; ++i) {
        const int x = 30 + (i % 4) * 181 + (i / 4) * 90;
        const int y = 161 + (i / 4) * 127;
        knobs[static_cast<size_t>(i)]->setBounds(x, y, 110, 104);
        knobLabels[static_cast<size_t>(i)]->setBounds(x, y + 100, 110, 20);
    }
    generateButton.setBounds(38, 439, 175, 43);
    exportButton.setBounds(227, 439, 175, 43);
    status.setBounds(415, 441, 345, 42);
}
juce::AudioProcessorEditor* TechHouseBassLab::createEditor() { return new BassMaxEditor(*this); }
