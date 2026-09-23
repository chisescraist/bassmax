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
    setSize(740, 660);
    startTimerHz(12);
    for (size_t i = 0; i < knobs.size(); ++i)
    {
        if (i == 7) continue; // STEPS uses a 16/32 selector. 
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
    stepsChoice.addItem("16 STEPS", 1);
    stepsChoice.addItem("32 STEPS", 2);
    addAndMakeVisible(stepsChoice);
    stepsAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processor.getParameters(), "steps", stepsChoice);
    addAndMakeVisible(generateButton);
    addAndMakeVisible(exportButton);
    addAndMakeVisible(previewButton);
    previewButton.onClick = [this]
    {
        processor.setPreviewEnabled(!processor.isPreviewEnabled());
        previewButton.setButtonText(processor.isPreviewEnabled() ? "PREVIEW: ON" : "PREVIEW: OFF");
    };
    generateButton.onClick = [this] { processor.generateNewPattern(); };
    exportButton.onClick = [this]
    {
        exportChooser = std::make_shared<juce::FileChooser>("Guardar patrón MIDI", juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("ChisesCraist_BassMax.mid"), "*.mid");
        auto chooser = exportChooser;
        juce::Component::SafePointer<BassMaxKnobEditor> safeThis(this);
        chooser->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::warnAboutOverwriting,
            [safeThis, chooser](const juce::FileChooser& selected)
            {
                if (safeThis == nullptr) return;
                auto file = selected.getResult();
                if (file == juce::File()) return;
                if (!file.hasFileExtension("mid")) file = file.withFileExtension("mid");
                if (!safeThis->processor.exportMidi(file))
                    juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon, "BassMax", "No se pudo guardar el MIDI.");
                safeThis->exportChooser.reset();
            });
    };
}

BassMaxKnobEditor::~BassMaxKnobEditor()
{
    stopTimer();
    exportChooser.reset();
    stepsAttachment.reset();
    for (auto& attachment : attachments) attachment.reset();
}

void BassMaxKnobEditor::timerCallback()
{
    repaint(juce::Rectangle<int>(22, 405, getWidth()-44, 170));
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
    g.drawText("PATRON MIDI · VISTA PREVIA · MIDI OUT (DEPENDIENTE DEL HOST)", 24, 77, getWidth()-48, 20, juce::Justification::centredLeft);

    const auto pattern = processor.getPatternSnapshot();
    const auto plot = juce::Rectangle<float>(26.0f, 415.0f, 688.0f, 150.0f);
    g.setColour(juce::Colour(0xff202c38));
    g.fillRoundedRectangle(plot, 7.0f);
    const int low = 24, high = 72;
    for (int n = low; n <= high; n += 12)
    {
        const float y = plot.getBottom() - (n - low) * plot.getHeight() / (high-low);
        g.setColour(juce::Colour(0xff344351));
        g.drawHorizontalLine(static_cast<int>(y), plot.getX(), plot.getRight());
    }
    for (int i = 0; i <= pattern.steps; ++i)
    {
        const float x = plot.getX() + plot.getWidth() * i / pattern.steps;
        g.setColour(juce::Colour(i % 4 == 0 ? 0xff5b6d7d : 0xff344351));
        g.drawVerticalLine(static_cast<int>(x), plot.getY(), plot.getBottom());
    }
    for (int i = 0; i < pattern.steps; ++i)
    {
        if (!pattern.gates[static_cast<size_t>(i)]) continue;
        const float stepWidth = plot.getWidth() / pattern.steps;
        const float start = plot.getX() + (i + (i % 2 ? pattern.swing : 0.0)) * stepWidth;
        const float width = juce::jmax(2.0f, static_cast<float>(pattern.gateLength * stepWidth) - 1.0f);
        const int note = juce::jlimit(low, high, pattern.notes[static_cast<size_t>(i)]);
        const float y = plot.getBottom() - (note - low + 0.5f) * plot.getHeight() / (high-low);
        g.setColour(juce::Colour(0xff55dbc2));
        g.fillRoundedRectangle(start, y - 4.0f, width, 8.0f, 2.0f);
    }
    g.setColour(juce::Colour(0xffb5c8d3));
    g.setFont(juce::Font(juce::FontOptions(11.0f)));
    g.drawText("MIDI PATTERN  ·  " + juce::String(pattern.steps) + " STEPS  ·  EDITABLE EN ABLETON DESPUES DE EXPORTAR", 26, 576, 690, 20, juce::Justification::centredLeft);
}

void BassMaxKnobEditor::resized()
{
    constexpr int columns = 6;
    constexpr int cellW = 116;
    constexpr int cellH = 137;
    for (size_t i = 0; i < knobs.size(); ++i)
    {
        if (i == 7) continue;
        const int x = 22 + static_cast<int>(i % columns) * cellW;
        const int y = 110 + static_cast<int>(i / columns) * cellH;
        knobs[i].setBounds(x, y, cellW-8, 105);
        captions[i].setBounds(x, y+105, cellW-8, 24);
    }
    stepsChoice.setBounds(22 + 7 % columns * cellW, 110 + 7 / columns * cellH + 35, cellW - 8, 34);
    generateButton.setBounds(24, 607, 270, 42);
    exportButton.setBounds(305, 607, 270, 42);
    previewButton.setBounds(585, 607, 130, 42);
}
