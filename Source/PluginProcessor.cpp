
#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <algorithm>
#include <cmath>

namespace
{
    constexpr double pi = 3.14159265358979323846;
}

juce::AudioProcessorValueTreeState::ParameterLayout
TechHouseBassLab::makeParams()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;

    auto f = [&](const char* id,
                 const char* name,
                 float lo,
                 float hi,
                 float def,
                 float step = 1.0f)
    {
        p.push_back(
            std::make_unique<juce::AudioParameterFloat>(
                juce::ParameterID { id, 1 },
                name,
                juce::NormalisableRange<float>(lo, hi, step),
                def
            )
        );
    };

    f("run", "RUN (0 off / 1 on)", 0, 1, 1);
    f("groove", "Groove: 0 Rolling 1 Offbeat 2 Syncopated 3 Minimal",
      0, 3, 0);
    f("root", "Root MIDI note (F1 = 29)", 24, 60, 29);
    f("density", "Density percent", 0, 100, 65);
    f("swing", "Swing percent", 0, 70, 15);
    f("length", "Gate percent", 10, 95, 48);
    f("seed", "Generate: change seed", 0, 9999, 1);
    f("bars", "Phrase length (4 / 8 / 16 bars)", 0, 2, 0);
    f("bpm", "MIDI export tempo BPM", 60, 200, 126);
    f("variation", "Pitch variation percent", 0, 100, 35);
    f("gain", "Internal bass gain (0 = silent preview)", 0, 1, 0.22f, 0.01f);
    f("tone", "Internal bass tone", 0, 1, 0.35f, 0.01f);

    return { p.begin(), p.end() };
}

TechHouseBassLab::TechHouseBassLab()
    : AudioProcessor(
          BusesProperties().withOutput(
              "Output",
              juce::AudioChannelSet::stereo(),
              true
          )
      ),
      state(*this, nullptr, "PARAMS", makeParams())
{
}

void TechHouseBassLab::prepareToPlay(double sampleRate, int)
{
    sr = sampleRate;
    phase = 0.0;
    env = 0.0;
    activeNote = -1;
    activeStep = -1;
    lastPpq = -1.0;
    previousSeed = -1;
    previousGroove = previousRoot = previousDensity = previousVariation = -1;
}

void TechHouseBassLab::regenerate(int seed)
{
    const juce::ScopedLock lock(patternLock);
    random.setSeed(seed);

    const int groove =
        static_cast<int>(state.getRawParameterValue("groove")->load());

    const int root =
        static_cast<int>(state.getRawParameterValue("root")->load());

    const int density =
        static_cast<int>(state.getRawParameterValue("density")->load());

    const int variation =
        static_cast<int>(state.getRawParameterValue("variation")->load());

    static const bool bases[4][16] =
    {
        { 1,0,0,1,0,0,1,0,1,0,1,0,0,0,1,0 },
        { 0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0 },
        { 1,0,0,1,0,0,0,1,0,0,1,0,0,1,0,1 },
        { 0,0,1,0,0,0,0,1,0,0,0,1,0,0,1,0 }
    };

    static const int offsets[] =
    {
        0,0,0,7,0,0,3,0,0,0,7,0,0,3,0,0
    };

    for (int i = 0; i < 256; ++i)
    {
        // A recognizable groove motif with deterministic, bar-specific variations.
        const int bar = i / 16;
        const bool base = bases[groove][i % 16];
        const int barAccent = (bar % 4 == 3 ? 12 : (bar % 4 == 1 ? -5 : 0));

        gates[i] = base
            ? random.nextInt(100) < std::min(100, density + 35 + barAccent)
            : random.nextInt(100) < std::max(0, density - 55 + barAccent);

        const int off =
            random.nextInt(100) < variation
                ? offsets[(i + bar % 4) % 16]
                : 0;

        pitches[i] = root + off;

        velocities[i] = juce::jlimit(
            35,
            127,
            85 + (i % 4 == 0 ? 22 : 0)
               + random.nextInt(17) - 8
        );
    }
}

void TechHouseBassLab::noteOff(
    juce::MidiBuffer& midi,
    int offset
)
{
    if (activeNote >= 0)
    {
        midi.addEvent(
            juce::MidiMessage::noteOff(1, activeNote),
            offset
        );

        activeNote = -1;
    }
}

void TechHouseBassLab::processBlock(
    juce::AudioBuffer<float>& audio,
    juce::MidiBuffer& midi
)
{
    juce::ScopedNoDenormals noDenormals;

    audio.clear();

    const int n = audio.getNumSamples();

    if (n == 0)
        return;

    juce::MidiBuffer out;

    for (const auto meta : midi)
    {
        out.addEvent(
            meta.getMessage(),
            meta.samplePosition
        );
    }

    const bool run =
        state.getRawParameterValue("run")->load() > 0.5f;

    const int seed =
        static_cast<int>(
            state.getRawParameterValue("seed")->load()
        );

    const int grooveNow = static_cast<int>(state.getRawParameterValue("groove")->load());
    const int rootNow = static_cast<int>(state.getRawParameterValue("root")->load());
    const int densityNow = static_cast<int>(state.getRawParameterValue("density")->load());
    const int variationNow = static_cast<int>(state.getRawParameterValue("variation")->load());
    if (seed != previousSeed || grooveNow != previousGroove || rootNow != previousRoot
        || densityNow != previousDensity || variationNow != previousVariation)
    {
        regenerate(seed);
        previousSeed = seed;
        previousGroove = grooveNow;
        previousRoot = rootNow;
        previousDensity = densityNow;
        previousVariation = variationNow;
    }

    juce::AudioPlayHead::CurrentPositionInfo pos;
    bool havePos = false;

    if (auto* hostPlayHead = getPlayHead())
    {
        if (auto position = hostPlayHead->getPosition())
        {
            if (auto ppq = position->getPpqPosition())
            {
                pos.ppqPosition = *ppq;
                havePos = true;
            }

            if (auto bpm = position->getBpm())
            {
                pos.bpm = *bpm;
            }

            // getIsPlaying() devuelve bool en esta versión de JUCE.
            pos.isPlaying = position->getIsPlaying();
        }
    }

    if (!run || !havePos || !pos.isPlaying)
    {
        noteOff(out, 0);

        activeStep = -1;
        lastPpq = -1.0;

        midi.swapWith(out);
        env = 0.0;

        return;
    }

    const juce::ScopedLock patternRead(patternLock);

    const double bpm =
        pos.bpm > 1.0 ? pos.bpm : 126.0;

    const double ppqPerSample =
        bpm / (60.0 * sr);

    const double swing =
        state.getRawParameterValue("swing")->load() / 100.0;

    const double gateLength =
        state.getRawParameterValue("length")->load() / 100.0;

    const int length =
        static_cast<int>(
            (64 << static_cast<int>(state.getRawParameterValue("bars")->load()))
        );

    // Audible internal preview remains available until external routing is verified.
    const double gain = previewEnabled.load() ? state.getRawParameterValue("gain")->load() : 0.0;

    const double tone =
        state.getRawParameterValue("tone")->load();

    for (int i = 0; i < n; ++i)
    {
        const double ppq =
            pos.ppqPosition + i * ppqPerSample;

        if (lastPpq >= 0.0
            && (ppq < lastPpq - 0.01
                || ppq - lastPpq > 1.0))
        {
            noteOff(out, i);
            activeStep = -1;
        }

        lastPpq = ppq;

        // Las semicorcheas impares se retrasan según el swing.
        const double cycle =
            std::floor(ppq / 4.0) * 4.0;

        const double local =
            ppq - cycle;

        const int raw =
            static_cast<int>(std::floor(local * 4.0));

        int step = raw;

        if (raw % 2 == 1
            && local * 4.0 - raw < swing)
        {
            step = raw - 1;
        }

        const int globalStep =
            static_cast<int>(
                std::floor(ppq / 4.0) * 16.0
            ) + step;

        const int index =
            ((globalStep % length) + length) % length;

        const double stepStart =
            static_cast<double>(globalStep) / 4.0
            + (globalStep % 2 != 0 ? swing / 4.0 : 0.0);

        if (globalStep != activeStep)
        {
            noteOff(out, i);
            activeStep = globalStep;

            if (gates[index])
            {
                activeNote = pitches[index];

                out.addEvent(
                    juce::MidiMessage::noteOn(
                        1,
                        activeNote,
                        static_cast<juce::uint8>(
                            velocities[index]
                        )
                    ),
                    i
                );

                env = 1.0;
            }
        }

        if (activeNote >= 0
            && ppq - stepStart >= gateLength * 0.25)
        {
            noteOff(out, i);
        }

        const double hz =
            activeNote >= 0
                ? juce::MidiMessage::getMidiNoteInHertz(
                      activeNote
                  )
                : 0.0;

        if (activeNote >= 0)
            phase += hz / sr;

        phase -= std::floor(phase);

        env *= std::exp(-1.0 / (sr * 0.11));

        const double saw =
            2.0 * phase - 1.0;

        const double sine =
            std::sin(2.0 * pi * phase);

        const float sample =
            static_cast<float>(
                (sine * (1.0 - tone) + saw * tone)
                * env
                * gain
            );

        for (int ch = 0; ch < audio.getNumChannels(); ++ch)
        {
            audio.setSample(ch, i, sample);
        }
    }

    midi.swapWith(out);
}

void TechHouseBassLab::getStateInformation(
    juce::MemoryBlock& dest
)
{
    auto xml = state.copyState().createXml();
    copyXmlToBinary(*xml, dest);
}

void TechHouseBassLab::setStateInformation(
    const void* data,
    int size
)
{
    auto xml = getXmlFromBinary(data, size);

    if (xml && xml->hasTagName(state.state.getType()))
    {
        state.replaceState(
            juce::ValueTree::fromXml(*xml)
        );
    }

    previousSeed = -1;
    undoAvailable.store(false);
    redoAvailable.store(false);
}


namespace { constexpr const char* historyIds[] = { "run", "groove", "root", "density", "swing", "length", "seed", "bars", "variation", "gain", "tone", "bpm" }; }

TechHouseBassLab::HistoryEntry TechHouseBassLab::captureHistory()
{
    HistoryEntry entry;
    entry.pattern = getPatternSnapshot();
    for (size_t i = 0; i < entry.params.size(); ++i)
        entry.params[i] = state.getRawParameterValue(historyIds[i])->load();
    return entry;
}

void TechHouseBassLab::restoreHistory(const HistoryEntry& entry)
{
    for (size_t i = 0; i < entry.params.size(); ++i)
        if (auto* param = state.getParameter(historyIds[i]))
        {
            param->beginChangeGesture();
            param->setValueNotifyingHost(param->convertTo0to1(entry.params[i]));
            param->endChangeGesture();
        }
    {
        const juce::ScopedLock lock(patternLock);
        pitches = entry.pattern.notes;
        gates = entry.pattern.gates;
        velocities = entry.pattern.velocities;
        previousSeed = static_cast<int>(entry.params[6]);
        previousGroove = static_cast<int>(entry.params[1]);
        previousRoot = static_cast<int>(entry.params[2]);
        previousDensity = static_cast<int>(entry.params[3]);
        previousVariation = static_cast<int>(entry.params[8]);
    }
}

void TechHouseBassLab::generateNewPattern()
{
    auto* parameter = state.getParameter("seed");
    if (parameter == nullptr) return;
    undoEntry = captureHistory();
    undoAvailable.store(true);
    redoAvailable.store(false);
    const int oldSeed = getPatternSeed();
    int nextSeed = juce::Random::getSystemRandom().nextInt(10000);
    if (nextSeed == oldSeed) nextSeed = (oldSeed + 1) % 10000;
    parameter->beginChangeGesture();
    parameter->setValueNotifyingHost(parameter->convertTo0to1(static_cast<float>(nextSeed)));
    parameter->endChangeGesture();
    // Make the new notes immediately available to the piano roll and drag/export.
    getPatternSnapshot();
}

bool TechHouseBassLab::undoGenerate()
{
    if (!undoAvailable.load()) return false;
    redoEntry = captureHistory();
    restoreHistory(undoEntry);
    undoAvailable.store(false);
    redoAvailable.store(true);
    return true;
}

bool TechHouseBassLab::redoGenerate()
{
    if (!redoAvailable.load()) return false;
    undoEntry = captureHistory();
    restoreHistory(redoEntry);
    redoAvailable.store(false);
    undoAvailable.store(true);
    return true;
}

bool TechHouseBassLab::exportMidi(const juce::File& file)
{
    const auto snapshot = getPatternSnapshot();
    const auto& noteCopy = snapshot.notes;
    const auto& gateCopy = snapshot.gates;
    const auto& velocityCopy = snapshot.velocities;
    const int steps = snapshot.steps;
    const double swing = snapshot.swing;
    const double gateLength = snapshot.gateLength;
    constexpr int ticksPerQuarter = 960;
    constexpr double ticksPerStep = ticksPerQuarter / 4.0;
    juce::MidiMessageSequence sequence;
    sequence.addEvent(juce::MidiMessage::tempoMetaEvent(
        juce::roundToInt(60000000.0 / juce::jlimit(60.0, 200.0, snapshot.bpm))), 0.0);
    sequence.addEvent(juce::MidiMessage::timeSignatureMetaEvent(4, 4), 0.0);
    for (int i = 0; i < steps; ++i)
    {
        if (!gateCopy[static_cast<size_t>(i)]) continue;
        const double start = i * ticksPerStep + ((i % 2) ? swing * ticksPerStep : 0.0);
        const double end = start + juce::jmax(1.0, gateLength * ticksPerStep);
        const int note = juce::jlimit(0, 127, noteCopy[static_cast<size_t>(i)]);
        sequence.addEvent(juce::MidiMessage::noteOn(1, note, static_cast<juce::uint8>(velocityCopy[static_cast<size_t>(i)])), start);
        sequence.addEvent(juce::MidiMessage::noteOff(1, note), end);
    }
    sequence.updateMatchedPairs();
    juce::MidiFile midiFile;
    midiFile.setTicksPerQuarterNote(ticksPerQuarter);
    midiFile.addTrack(sequence);
    auto stream = file.createOutputStream();
    if (stream == nullptr) return false;
    const bool ok = midiFile.writeTo(*stream);
    stream->flush();
    return ok;
}

TechHouseBassLab::PatternSnapshot TechHouseBassLab::getPatternSnapshot()
{
    const int seed = getPatternSeed();
    const int groove = static_cast<int>(state.getRawParameterValue("groove")->load());
    const int root = static_cast<int>(state.getRawParameterValue("root")->load());
    const int density = static_cast<int>(state.getRawParameterValue("density")->load());
    const int variation = static_cast<int>(state.getRawParameterValue("variation")->load());
    PatternSnapshot snapshot;
    {
        const juce::ScopedLock lock(patternLock);
        // Rebuild deterministically when a parameter changes, even while transport is stopped.
        if (seed != previousSeed || groove != previousGroove || root != previousRoot
            || density != previousDensity || variation != previousVariation)
        {
            // regenerate() also takes patternLock, so release before calling below.
        }
    }
    if (seed != previousSeed || groove != previousGroove || root != previousRoot
        || density != previousDensity || variation != previousVariation)
    {
        regenerate(seed);
        previousSeed = seed;
        previousGroove = groove;
        previousRoot = root;
        previousDensity = density;
        previousVariation = variation;
    }
    {
        const juce::ScopedLock lock(patternLock);
        snapshot.notes = pitches;
        snapshot.gates = gates;
        snapshot.velocities = velocities;
    }
    snapshot.bars = (4 << juce::jlimit(0, 2, static_cast<int>(state.getRawParameterValue("bars")->load())));
    snapshot.steps = snapshot.bars * 16;
    snapshot.bpm = state.getRawParameterValue("bpm")->load();
    snapshot.swing = state.getRawParameterValue("swing")->load() / 100.0;
    snapshot.gateLength = state.getRawParameterValue("length")->load() / 100.0;
    return snapshot;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new TechHouseBassLab();
}

// The editor is intentionally isolated from the stable audio engine.
juce::AudioProcessorEditor* TechHouseBassLab::createEditor()
{
    return new BassMaxKnobEditor(*this);
}
