#include "PluginEditor.h"

namespace
{

class BassMaxVintageLook final : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float position, float start, float end, juce::Slider&) override
    {
        const float diameter = juce::jmin(width, height) - 12.0f;
        const float cx = x + width * 0.5f, cy = y + height * 0.5f;
        const float radius = diameter * 0.5f;
        const auto face = juce::Colour(0xff101a1d);
        g.setColour(juce::Colour(0xff050a0c)); g.fillEllipse(cx-radius-3, cy-radius-3, diameter+6, diameter+6);
        juce::Path arc; arc.addCentredArc(cx, cy, radius+3, radius+3, 0, start, end, true);
        g.setColour(juce::Colour(0xff33464a)); g.strokePath(arc, juce::PathStrokeType(2.5f));
        juce::Path active; active.addCentredArc(cx, cy, radius+3, radius+3, 0, start, start+(end-start)*position, true);
        g.setColour(juce::Colour(0xff40dfd4)); g.strokePath(active, juce::PathStrokeType(3.0f));
        g.setColour(juce::Colour(0xff526267)); g.fillEllipse(cx-radius, cy-radius, diameter, diameter);
        g.setColour(face); g.fillEllipse(cx-radius+2, cy-radius+2, diameter-4, diameter-4);
        g.setColour(juce::Colour(0xff26373a)); g.drawEllipse(cx-radius+5, cy-radius+5, diameter-10, diameter-10, 1.0f);
        const float angle = start + (end-start)*position;
        const auto point = juce::Point<float>(cx + std::sin(angle)*(radius-9), cy - std::cos(angle)*(radius-9));
        g.setColour(juce::Colour(0xffffd18b)); g.drawLine(cx, cy, point.x, point.y, 2.2f);
    }
    void drawButtonBackground(juce::Graphics& g, juce::Button& button, const juce::Colour&, bool hovered, bool down) override
    {
        auto r=button.getLocalBounds().toFloat().reduced(1);
        g.setColour(juce::Colour(down?0xff365459:hovered?0xff263f44:0xff14252b)); g.fillRoundedRectangle(r, 4);
        g.setColour(juce::Colour(0xffa67a42)); g.drawRoundedRectangle(r, 4, 1);
    }
    void drawComboBox(juce::Graphics& g, int width, int height, bool, int, int, int, int, juce::ComboBox&) override
    {
        auto r=juce::Rectangle<float>(0,0,(float)width,(float)height).reduced(1);
        g.setColour(juce::Colour(0xff14252b)); g.fillRoundedRectangle(r,3);
        g.setColour(juce::Colour(0xff9b713f)); g.drawRoundedRectangle(r,3,1);
        juce::Path triangle; triangle.addTriangle(width-18.0f,height*0.42f,width-8.0f,height*0.42f,width-13.0f,height*0.62f);
        g.setColour(juce::Colour(0xffffc16b)); g.fillPath(triangle);
    }
};
    constexpr const char* ids[] = { "run", "groove", "root", "density", "swing", "length", "seed", "bars", "variation", "gain", "tone", "bpm", "attack", "release", "cutoff", "resonance", "drive", "sub", "reverb" };
    constexpr const char* names[] = { "RUN", "GROOVE", "ROOT", "DENSITY", "SWING", "GATE", "SEED", "BARS", "VARIATION", "GAIN", "TONE", "BPM", "ATTACK", "RELEASE", "CUTOFF", "RESONANCE", "DRIVE", "SUB", "REVERB" };
    static_assert(std::size(ids) == 19);
}

BassMaxKnobEditor::BassMaxKnobEditor(TechHouseBassLab& p)
    : juce::AudioProcessorEditor(p), processor(p)
{
    vintageLook = std::make_unique<BassMaxVintageLook>();
    setLookAndFeel(vintageLook.get());
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
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 62, 16);
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
    addAndMakeVisible(responseButton);
    responseButton.onClick = [this] { processor.generateResponse(); phraseChoice.setSelectedId(2, juce::sendNotification); repaint(); };
    phraseChoice.addItem("A · ORIGINAL", 1);
    phraseChoice.addItem("B · RESPONSE", 2);
    phraseChoice.addItem("A+B · COMPLETA", 3);
    phraseChoice.setSelectedId(1, juce::dontSendNotification);
    phraseChoice.onChange = [this] { processor.setPhraseView(phraseChoice.getSelectedId() - 1); repaint(); };
    addAndMakeVisible(phraseChoice);
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
    generateButton.onClick = [this] { processor.generateNewPattern(); phraseChoice.setSelectedId(1, juce::dontSendNotification); repaint(); };
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
    setLookAndFeel(nullptr);
    exportChooser.reset();
    barsAttachment.reset();
    styleAttachment.reset();
    for (auto& attachment : attachments) attachment.reset();
}

void BassMaxKnobEditor::timerCallback()
{
    undoButton.setEnabled(processor.canUndoGenerate());
    redoButton.setEnabled(processor.canRedoGenerate());
    phraseChoice.setItemEnabled(2, processor.hasResponse());
    phraseChoice.setItemEnabled(3, processor.hasResponse());
    if (phraseChoice.getSelectedId() != processor.getPhraseView() + 1)
        phraseChoice.setSelectedId(processor.getPhraseView() + 1, juce::dontSendNotification);
    const auto bars = processor.getPatternSnapshot().bars;
    const int oldSelection = pageChoice.getSelectedId();
    if (pageChoice.getNumItems() != bars)
    {
        pageChoice.clear(juce::dontSendNotification);
        for (int bar = 1; bar <= bars; ++bar)
            pageChoice.addItem("BAR " + juce::String(bar) + " / " + juce::String(bars), bar);
        pageChoice.setSelectedId(juce::jlimit(1, bars, oldSelection), juce::dontSendNotification);
    }
    repaint(juce::Rectangle<int>(40, 410, getWidth()-60, 180));
}

void BassMaxKnobEditor::paint(juce::Graphics& g)
{
    const auto gold = juce::Colour(0xffffc47a);
    const auto border = juce::Colour(0xff795b37);
    g.fillAll(juce::Colour(0xff0a1316));
    g.setColour(juce::Colour(0xff1b2b30)); g.fillRoundedRectangle(8.0f, 8.0f, 724.0f, 774.0f, 7.0f);
    g.setColour(border); g.drawRoundedRectangle(8.0f, 8.0f, 724.0f, 774.0f, 7.0f, 1.2f);
    g.setColour(juce::Colour(0xff101c21)); g.fillRoundedRectangle(18.0f, 14.0f, 704.0f, 65.0f, 4.0f);
    g.setColour(gold); g.setFont(juce::Font(juce::FontOptions(10.0f,juce::Font::bold)));
    g.drawText("CHISESCRAIST AUDIO",28,17,330,16,juce::Justification::centredLeft);
    g.setFont(juce::Font(juce::FontOptions(32.0f,juce::Font::bold)));
    g.drawText("BASSMAX",27,31,380,40,juce::Justification::centredLeft);
    g.setFont(juce::Font(juce::FontOptions(10.0f)));
    g.drawText("ANALOG SOUL  /  MODERN GROOVES",420,40,288,22,juce::Justification::centredRight);
    g.setColour(border); g.drawLine(20.0f,80.0f,720.0f,80.0f);
    for(int row=0;row<3;++row)
        for(int col=0;col<6;++col)
        {
            const float x=22.0f+col*116.0f, y=122.0f+row*90.0f;
            g.setColour(juce::Colour(0xff101b20)); g.fillRoundedRectangle(x,y,112.0f,87.0f,3.0f);
            g.setColour(juce::Colour(0xff425052)); g.drawRoundedRectangle(x,y,112.0f,87.0f,3.0f,0.7f);
        }
    const auto pattern = processor.getDisplayedPatternSnapshot();
    const auto plot = juce::Rectangle<float>(43.0f, 414.0f, 670.0f, 145.0f);
    g.setColour(juce::Colour(0xff070e10)); g.fillRect(plot);
    g.setColour(border); g.drawRect(plot,1.0f);
    const int low=24, high=72;
    for(int n=low;n<=high;++n)
    {
        const float y=plot.getBottom()-(n-low)*plot.getHeight()/(high-low);
        g.setColour(juce::Colour(n%12==0?0xff4b534c:0xff263034));
        g.drawHorizontalLine((int)y,plot.getX(),plot.getRight());
    }
    for(int i=0;i<=16;++i)
    {
        const float x=plot.getX()+plot.getWidth()*i/16.0f;
        g.setColour(juce::Colour(i%4==0?0xff8b6941:0xff283538));
        g.drawVerticalLine((int)x,plot.getY(),plot.getBottom());
    }
    const int firstStep=juce::jlimit(0,pattern.bars-1,pageChoice.getSelectedId()-1)*16;
    for(int localStep=0;localStep<16;++localStep)
    {
        const int i=firstStep+localStep;
        if(!pattern.gates[(size_t)i]) continue;
        const float stepWidth=plot.getWidth()/16.0f;
        const float start=plot.getX()+(localStep+(i%2?pattern.swing:0.0))*stepWidth;
        const float width=juce::jmax(2.0f,(float)(pattern.gateLength*stepWidth)-1.0f);
        const int note=juce::jlimit(low,high,pattern.notes[(size_t)i]);
        const float y=plot.getBottom()-(note-low+0.5f)*plot.getHeight()/(high-low);
        g.setColour(juce::Colour(0xfff4a942)); g.fillRoundedRectangle(start,y-3.0f,width,6.0f,1.5f);
        g.setColour(juce::Colour(0xffffd88d)); g.fillRect(start,y-3.0f,width,1.2f);
    }
    g.setColour(gold); g.setFont(juce::Font(juce::FontOptions(10.0f)));
    const int mode=processor.getPhraseView();
    g.drawText(juce::String(mode==2?"A+B: EDITAR A O B POR SEPARADO":"ARRASTRAR: MOVER  ·  DOBLE CLIC: CREAR  ·  CLIC DERECHO: BORRAR")+"   ·   "+juce::String(pattern.bars*(mode==2?2:1))+" BARS  ·  "+juce::String(pattern.bpm,0)+" BPM",43,565,670,19,juce::Justification::centredLeft);
    g.setColour(border); g.drawLine(20.0f,592.0f,720.0f,592.0f);
}

void BassMaxKnobEditor::resized()
{
    styleChoice.setBounds(24,87,194,29);
    barsChoice.setBounds(227,87,122,29);
    phraseChoice.setBounds(358,87,170,29);
    pageChoice.setBounds(537,87,177,29);
    constexpr int cellW=116, cellH=90;
    // The BARS parameter has its own selector in the header. Display the remaining 18 knobs in 3 x 6.
    int visible=0;
    for(size_t i=0;i<knobs.size();++i)
    {
        if(i==7) continue;
        const int col=visible%6,row=visible/6;
        const int x=22+col*cellW, y=122+row*cellH;
        captions[i].setBounds(x+3,y+2,106,15);
        knobs[i].setBounds(x+18,y+17,76,68);
        ++visible;
    }
    generateButton.setBounds(24,603,222,36);
    responseButton.setBounds(254,603,184,36);
    previewButton.setBounds(446,603,268,36);
    undoButton.setBounds(24,648,162,35);
    redoButton.setBounds(194,648,162,35);
    exportButton.setBounds(364,648,168,35);
    dragButton.setBounds(24,692,332,42);
    dragStatus.setBounds(365,692,349,42);
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

int BassMaxKnobEditor::stepAt(juce::Point<float> p) const
{
    if (p.x < 43.0f || p.x >= 713.0f || p.y < 414.0f || p.y >= 559.0f) return -1;
    const int bar = juce::jmax(0, pageChoice.getSelectedId() - 1);
    return bar * 16 + juce::jlimit(0, 15, static_cast<int>((p.x - 43.0f) / (670.0f / 16.0f)));
}
int BassMaxKnobEditor::pitchAt(juce::Point<float> p) const
{
    return juce::jlimit(24, 72, juce::roundToInt(24.0f + (559.0f - p.y) * 48.0f / 145.0f - 0.5f));
}
void BassMaxKnobEditor::mouseDown(const juce::MouseEvent& e)
{
    editSource = -1;
    if (processor.getPhraseView() == 2) return;
    const int step = stepAt(e.position);
    if (step < 0) return;
    const auto pattern = processor.getDisplayedPatternSnapshot();
    if (e.mods.isRightButtonDown())
    {
        processor.editNote(step, step, pitchAt(e.position), true);
        repaint();
        return;
    }
    if (e.getNumberOfClicks() >= 2)
    {
        processor.addNote(step, pitchAt(e.position));
        repaint();
        return;
    }
    if (pattern.gates[static_cast<size_t>(step)])
    {
        editSource = editTarget = step;
        editPitch = pattern.notes[static_cast<size_t>(step)];
    }
}
void BassMaxKnobEditor::mouseDrag(const juce::MouseEvent& e)
{
    if (editSource < 0) return;
    const int step = stepAt(e.position);
    if (step >= 0) editTarget = step;
    editPitch = pitchAt(e.position);
    repaint();
}
void BassMaxKnobEditor::mouseUp(const juce::MouseEvent& e)
{
    if (editSource < 0) return;
    const int step = stepAt(e.position);
    if (step >= 0) editTarget = step;
    processor.editNote(editSource, editTarget, editPitch);
    editSource = -1;
    repaint();
}
