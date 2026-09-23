#include "PluginEditor.h"

namespace
{
    constexpr const char* ids[] = { "run", "groove", "root", "density", "swing", "length", "seed", "bars", "variation", "gain", "tone", "bpm", "attack", "release", "cutoff", "resonance", "drive", "sub", "reverb" };
    constexpr const char* names[] = { "RUN", "GROOVE", "ROOT", "DENSITY", "SWING", "GATE", "SEED", "BARS", "VARIATION", "GAIN", "TONE", "BPM", "ATTACK", "RELEASE", "CUTOFF", "RESONANCE", "DRIVE", "SUB", "REVERB" };
    static_assert(std::size(ids) == 19);
}

BassMaxKnobEditor::BassMaxKnobEditor(TechHouseBassLab& p)
    : juce::AudioProcessorEditor(p), processor(p)
{
    setSize(740, 790);
    styleChoice.addItem("TECH HOUSE", 1);
    styleChoice.addItem("MELODIC TECHNO", 2);
    styleChoice.addItem("AFRO TECHNO", 3);
    addAndMakeVisible(styleChoice);
    styleAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processor.getParameters(), "style", styleChoice);
    styleChoice.onChange = [this]
    {
        // Genre presets change the native synth timbre; every knob stays editable.
        const int style = styleChoice.getSelectedId();
        const char* ids[] = {"attack", "release", "cutoff", "resonance", "drive", "sub", "reverb", "tone", "length", "swing"};
        const float presets[3][10] = {
            {4, 125, 720, 0.20f, 0.38f, 0.50f, 0.04f, 0.43f, 45, 16},
            {15, 340, 1250, 0.34f, 0.27f, 0.58f, 0.15f, 0.60f, 76, 7},
            {7, 220, 540, 0.23f, 0.42f, 0.68f, 0.10f, 0.35f, 58, 25}
        };
        if (style < 1 || style > 3) return;
        for (int i = 0; i < 10; ++i)
            if (auto* parameter = processor.getParameters().getParameter(ids[i]))
                parameter->setValueNotifyingHost(parameter->convertTo0to1(presets[style - 1][i]));
    };
    startTimerHz(12);
    for (size_t i = 0; i < knobs.size(); ++i)
    {
        if (i == 7) continue; // BARS uses a 4/8/16 selector. 
        auto& slider = knobs[i];
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 54, 16);
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
    barsChoice.addItem("4 BARS", 1);
    barsChoice.addItem("8 BARS", 2);
    barsChoice.addItem("16 BARS", 3);
    addAndMakeVisible(barsChoice);
    barsAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processor.getParameters(), "bars", barsChoice);
    addAndMakeVisible(pageChoice);
    pageChoice.onChange = [this] { repaint(); };
    addAndMakeVisible(generateButton);
    addAndMakeVisible(exportButton);
    addAndMakeVisible(undoButton);
    addAndMakeVisible(redoButton);
    undoButton.onClick = [this] { processor.undoGenerate(); repaint(); };
    redoButton.onClick = [this] { processor.redoGenerate(); repaint(); };
    addAndMakeVisible(previewButton);
    addAndMakeVisible(dragButton);
    dragButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff287f72));
    dragButton.onDragMidi = [this] { startMidiDrag(); };
    dragStatus.setText("Arrastra DRAG MIDI hasta una pista MIDI de Ableton", juce::dontSendNotification);
    dragStatus.setJustificationType(juce::Justification::centred);
    dragStatus.setColour(juce::Label::textColourId, juce::Colour(0xffb5c8d3));
    addAndMakeVisible(dragStatus);
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
    barsAttachment.reset();
    styleAttachment.reset();
    for (auto& attachment : attachments) attachment.reset();
}

void BassMaxKnobEditor::timerCallback()
{
    undoButton.setEnabled(processor.canUndoGenerate());
    redoButton.setEnabled(processor.canRedoGenerate());
    const auto bars = processor.getPatternSnapshot().bars;
    const int oldSelection = pageChoice.getSelectedId();
    if (pageChoice.getNumItems() != bars)
    {
        pageChoice.clear(juce::dontSendNotification);
        for (int bar = 1; bar <= bars; ++bar)
            pageChoice.addItem("BAR " + juce::String(bar) + " / " + juce::String(bars), bar);
        pageChoice.setSelectedId(juce::jlimit(1, bars, oldSelection), juce::dontSendNotification);
    }
    repaint(juce::Rectangle<int>(22, 485, getWidth()-44, 155));
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
    const auto plot = juce::Rectangle<float>(26.0f, 488.0f, 688.0f, 112.0f);
    g.setColour(juce::Colour(0xff202c38));
    g.fillRoundedRectangle(plot, 7.0f);
    const int low = 24, high = 72;
    for (int n = low; n <= high; n += 12)
    {
        const float y = plot.getBottom() - (n - low) * plot.getHeight() / (high-low);
        g.setColour(juce::Colour(0xff344351));
        g.drawHorizontalLine(static_cast<int>(y), plot.getX(), plot.getRight());
    }
    for (int i = 0; i <= 16; ++i)
    {
        const float x = plot.getX() + plot.getWidth() * i / 16.0f;
        g.setColour(juce::Colour(i % 4 == 0 ? 0xff5b6d7d : 0xff344351));
        g.drawVerticalLine(static_cast<int>(x), plot.getY(), plot.getBottom());
    }
    const int firstStep = juce::jlimit(0, pattern.bars - 1, pageChoice.getSelectedId() - 1) * 16;
    for (int localStep = 0; localStep < 16; ++localStep)
    {
        const int i = firstStep + localStep;
        if (!pattern.gates[static_cast<size_t>(i)]) continue;
        const float stepWidth = plot.getWidth() / 16.0f;
        const float start = plot.getX() + (localStep + (i % 2 ? pattern.swing : 0.0)) * stepWidth;
        const float width = juce::jmax(2.0f, static_cast<float>(pattern.gateLength * stepWidth) - 1.0f);
        const int note = juce::jlimit(low, high, pattern.notes[static_cast<size_t>(i)]);
        const float y = plot.getBottom() - (note - low + 0.5f) * plot.getHeight() / (high-low);
        g.setColour(juce::Colour(0xff55dbc2));
        g.fillRoundedRectangle(start, y - 4.0f, width, 8.0f, 2.0f);
    }
    g.setColour(juce::Colour(0xffb5c8d3));
    g.setFont(juce::Font(juce::FontOptions(11.0f)));
    g.drawText("MIDI PATTERN  ·  " + juce::String(pattern.bars) + " BARS  ·  " + juce::String(pattern.bpm, 0) + " BPM  ·  DRAG MIDI", 26, 608, 690, 18, juce::Justification::centredLeft);
}

void BassMaxKnobEditor::resized()
{
    // Compact four-row layout with a genre selector in the header.
    styleChoice.setBounds(472, 45, 241, 30);
    constexpr int columns = 6;
    constexpr int cellW = 116;
    constexpr int cellH = 94;
    for (size_t i = 0; i < knobs.size(); ++i)
    {
        if (i == 7) continue;
        const int x = 22 + static_cast<int>(i % columns) * cellW;
        const int y = 96 + static_cast<int>(i / columns) * cellH;
        knobs[i].setBounds(x + 10, y, cellW - 28, 69);
        captions[i].setBounds(x, y + 68, cellW - 8, 19);
    }
    barsChoice.setBounds(22 + 7 % columns * cellW, 96 + 7 / columns * cellH + 19, cellW - 8, 28);
    pageChoice.setBounds(24, 631, 180, 26);
    generateButton.setBounds(24, 663, 270, 34);
    exportButton.setBounds(305, 663, 270, 34);
    previewButton.setBounds(585, 663, 130, 34);
    dragButton.setBounds(24, 705, 270, 34);
    dragStatus.setBounds(305, 705, 410, 34);
    undoButton.setBounds(24, 747, 270, 34);
    redoButton.setBounds(305, 747, 270, 34);
}

void BassMaxKnobEditor::startMidiDrag()
{
    // A unique file for each drag avoids overwriting a clip while the host imports it.
    const auto tempDir = juce::File::getSpecialLocation(juce::File::tempDirectory)
        .getChildFile("ChisesCraist_BassMax_MIDI_Drag");
    if (!tempDir.createDirectory())
    {
        dragStatus.setText("Error: no se pudo crear la carpeta temporal", juce::dontSendNotification);
        return;
    }
    const auto tempFile = tempDir.getNonexistentChildFile("BassMax_" + juce::String(juce::Time::getCurrentTime().toMilliseconds()), ".mid", false);
    if (!processor.exportMidi(tempFile))
    {
        dragStatus.setText("Error: no se pudo generar el archivo MIDI", juce::dontSendNotification);
        return;
    }
    dragStatus.setText("Solta el MIDI en una pista MIDI de Ableton", juce::dontSendNotification);
    // Native OS file drag. Whether the host accepts the drop from a plugin window
    // depends on its VST3 hosting and Windows drag/drop support.
    juce::DragAndDropContainer::performExternalDragDropOfFiles(
        juce::StringArray { tempFile.getFullPathName() }, false, this);
}
