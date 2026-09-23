
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
    f("style", "Style: Tech House / Melodic Techno / Afro Techno", 0, 2, 0);
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
    f("attack", "Bass attack ms", 1, 120, 4, 1);
    f("release", "Bass release ms", 20, 900, 130, 1);
    f("cutoff", "Bass filter cutoff Hz", 70, 5000, 650, 1);
    f("resonance", "Bass filter resonance", 0, 0.9f, 0.18f, 0.01f);
    f("drive", "Bass saturation", 0, 1, 0.32f, 0.01f);
    f("sub", "Bass sub oscillator", 0, 1, 0.48f, 0.01f);
    f("reverb", "Bass reverb mix", 0, 0.5f, 0.06f, 0.01f);

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
    filterLow = filterBand = 0.0;
    synthEnvelope = 0.0;
    reverb.reset();
    reverb.setSampleRate(sr);
    previousSeed = -1;
    previousStyle = -1;
    previousGroove = previousRoot = previousDensity = previousVariation = -1;
}

void TechHouseBassLab::regenerate(int seed)
{
    const juce::ScopedLock lock(patternLock);
    random.setSeed(seed);

    const int style = juce::jlimit(0, 2, static_cast<int>(state.getRawParameterValue("style")->load()));
    const int groove =
        static_cast<int>(state.getRawParameterValue("groove")->load());

    const int root =
        static_cast<int>(state.getRawParameterValue("root")->load());

    const int density =
        static_cast<int>(state.getRawParameterValue("density")->load());

    const int variation =
        static_cast<int>(state.getRawParameterValue("variation")->load());

    // Each genre has its own rhythmic vocabulary and pitch movement.
    static const bool motifs[3][4][16] = {
        { // Tech House: short, punchy syncopations and octave/fifth jumps.
            {1,0,0,1,0,0,1,0,1,0,1,0,0,0,1,0},
            {0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0},
            {1,0,0,1,0,0,0,1,0,0,1,0,0,1,0,1},
            {0,0,1,0,0,0,0,1,0,0,0,1,0,0,1,0}
        },
        { // Melodic Techno: longer, evolving ostinatos and minor-key movement.
            {1,0,0,0,1,0,1,0,1,0,0,0,1,0,1,0},
            {1,0,0,0,0,0,1,0,1,0,0,0,0,0,1,0},
            {1,0,0,1,0,0,1,0,1,0,0,1,0,0,1,0},
            {1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0}
        },
        { // Afro Techno: interlocking offbeats, 3+3+2 accents and call/response.
            {1,0,0,1,0,0,1,0,0,1,0,0,1,0,1,0},
            {0,0,1,0,0,1,0,0,1,0,0,1,0,0,1,0},
            {1,0,0,1,0,1,0,0,0,1,0,1,0,0,1,0},
            {1,0,0,0,1,0,1,0,0,1,0,0,1,0,0,1}
        }
    };
    static const int intervals[3][16] = {
        {0,0,0,7,0,0,3,0,0,0,7,0,0,3,0,12},
        {0,0,7,0,3,0,7,0,0,12,7,0,3,0,10,0},
        {0,0,3,0,7,0,0,10,0,3,0,7,0,0,12,0}
    };
    for (int i = 0; i < 256; ++i)
    {
        const int bar = i / 16;
        const int step = i % 16;
        const bool base = motifs[style][groove][step];
        const int accent = style == 2 ? ((step % 8 == 0 || step % 8 == 3 || step % 8 == 6) ? 13 : -4)
                         : style == 1 ? ((bar % 4 == 3) ? 9 : -2)
                                      : ((bar % 4 == 3) ? 12 : 0);
        const int mainChance = style == 1 ? 19 : style == 2 ? 24 : 35;
        const int ghostChance = style == 1 ? 72 : style == 2 ? 62 : 55;
        gates[i] = base ? random.nextInt(100) < juce::jlimit(0, 100, density + mainChance + accent)
                        : random.nextInt(100) < juce::jmax(0, density - ghostChance + accent);
        const int interval = intervals[style][(step + (style == 1 ? bar % 4 : 0)) % 16];
        const int off = random.nextInt(100) < variation ? interval : 0;
        pitches[i] = root + off;
        const int velocityAccent = style == 2 ? ((step % 8 == 0 || step % 8 == 3 || step % 8 == 6) ? 19 : -5)
                                 : style == 1 ? (step % 4 == 0 ? 14 : -2)
                                              : (step % 4 == 0 ? 22 : 0);
        velocities[i] = juce::jlimit(35, 127, 84 + velocityAccent + random.nextInt(17) - 8);
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

    const int styleNow = static_cast<int>(state.getRawParameterValue("style")->load());
    const int grooveNow = static_cast<int>(state.getRawParameterValue("groove")->load());
    const int rootNow = static_cast<int>(state.getRawParameterValue("root")->load());
    const int densityNow = static_cast<int>(state.getRawParameterValue("density")->load());
    const int variationNow = static_cast<int>(state.getRawParameterValue("variation")->load());
    if (seed != previousSeed || styleNow != previousStyle || grooveNow != previousGroove || rootNow != previousRoot
        || densityNow != previousDensity || variationNow != previousVariation)
    {
        regenerate(seed);
        previousSeed = seed;
        previousStyle = styleNow;
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

        if (audio.getNumChannels() >= 2)
        reverb.processStereo(audio.getWritePointer(0), audio.getWritePointer(1), n);
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

    const double tone = state.getRawParameterValue("tone")->load();
    const double attack = state.getRawParameterValue("attack")->load() * 0.001;
    const double release = state.getRawParameterValue("release")->load() * 0.001;
    const double cutoff = state.getRawParameterValue("cutoff")->load();
    const double resonance = state.getRawParameterValue("resonance")->load();
    const double drive = state.getRawParameterValue("drive")->load();
    const double sub = state.getRawParameterValue("sub")->load();
    const float reverbMix = state.getRawParameterValue("reverb")->load();
    juce::Reverb::Parameters rv;
    rv.roomSize = 0.36f; rv.damping = 0.65f; rv.wetLevel = reverbMix;
    rv.dryLevel = 1.0f - reverbMix; rv.width = 0.8f;
    reverb.setParameters(rv);
    const double attackCoeff = std::exp(-1.0 / (sr * attack));
    const double releaseCoeff = std::exp(-1.0 / (sr * release));
    const double filterF = juce::jlimit(0.001, 0.85, 2.0 * std::sin(pi * juce::jmin(cutoff, sr * 0.18) / sr));

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

        const int phraseMode = responseAvailable.load() ? phraseView.load() : 0;
        const int phraseStep = ((globalStep % (phraseMode == 2 ? length * 2 : length)) + (phraseMode == 2 ? length * 2 : length)) % (phraseMode == 2 ? length * 2 : length);
        const bool playAnswer = phraseMode == 1 || (phraseMode == 2 && phraseStep >= length);
        const int index = phraseStep % length;

        const double stepStart =
            static_cast<double>(globalStep) / 4.0
            + (globalStep % 2 != 0 ? swing / 4.0 : 0.0);

        if (globalStep != activeStep)
        {
            noteOff(out, i);
            activeStep = globalStep;

            const bool hit = playAnswer ? responsePattern.gates[static_cast<size_t>(index)] : gates[index];
            if (hit)
            {
                activeNote = playAnswer ? responsePattern.notes[static_cast<size_t>(index)] : pitches[index];

                out.addEvent(
                    juce::MidiMessage::noteOn(
                        1,
                        activeNote,
                        static_cast<juce::uint8>(
                            (playAnswer ? responsePattern.velocities[static_cast<size_t>(index)] : velocities[index])
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

        const bool keyDown = activeNote >= 0;
        synthEnvelope = keyDown ? 1.0 - (1.0 - synthEnvelope) * attackCoeff
                                : synthEnvelope * releaseCoeff;
        env = synthEnvelope;

        const double saw =
            2.0 * phase - 1.0;

        const double sine =
            std::sin(2.0 * pi * phase);

        const double oscillator = (sine * (1.0 - tone) + saw * tone) * (1.0 - 0.28 * sub)
                                + std::sin(pi * phase) * 0.28 * sub;
        const double saturated = std::tanh(oscillator * (1.0 + 5.0 * drive)) / (1.0 + drive);
        filterLow += filterF * filterBand;
        const double high = saturated - filterLow - (0.25 + 1.6 * (1.0 - resonance)) * filterBand;
        filterBand += filterF * high;
        filterLow = juce::jlimit(-2.0, 2.0, filterLow);
        filterBand = juce::jlimit(-2.0, 2.0, filterBand);
        const float sample = static_cast<float>(filterLow * env * gain * 0.65);

        for (int ch = 0; ch < audio.getNumChannels(); ++ch)
        {
            audio.setSample(ch, i, sample);
        }
    }

    if (audio.getNumChannels() >= 2)
        reverb.processStereo(audio.getWritePointer(0), audio.getWritePointer(1), n);
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


namespace { constexpr const char* historyIds[] = { "run", "groove", "root", "density", "swing", "length", "seed", "bars", "variation", "gain", "tone", "bpm", "style" }; }

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
        previousStyle = static_cast<int>(entry.params[12]);
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
    responseAvailable.store(false);
    phraseView.store(0);
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
    responseAvailable.store(false);
    phraseView.store(0);
    restoreHistory(undoEntry);
    undoAvailable.store(false);
    redoAvailable.store(true);
    return true;
}

bool TechHouseBassLab::redoGenerate()
{
    if (!redoAvailable.load()) return false;
    undoEntry = captureHistory();
    responseAvailable.store(false);
    phraseView.store(0);
    restoreHistory(redoEntry);
    redoAvailable.store(false);
    undoAvailable.store(true);
    return true;
}

void TechHouseBassLab::generateResponse()
{
    const auto original = getPatternSnapshot();
    const int style = juce::jlimit(0, 2, static_cast<int>(state.getRawParameterValue("style")->load()));
    const int root = static_cast<int>(state.getRawParameterValue("root")->load());
    juce::Random rng(juce::Random::getSystemRandom().nextInt());
    PatternSnapshot answer = original;
    const int steps = original.steps;
    const int intervals[3][5] = { {0, 0, 3, 5, 7}, {0, 3, 5, 7, 10}, {0, 2, 3, 5, 7} };
    for (int bar = 0; bar < original.bars; ++bar)
    {
        for (int step = 0; step < 16; ++step)
        {
            const int i = bar * 16 + step;
            const int source = (bar % 2 == 0 ? bar : bar - 1) * 16 + step;
            const bool sourceHit = original.gates[static_cast<size_t>(source)];
            const bool rest = (style == 0 ? (step == 0 || step == 8) : style == 1 ? (step < 4) : (step == 0 || step == 4));
            bool hit = sourceHit;
            if (rest && rng.nextInt(100) < 65) hit = false;
            if (!hit && !rest && rng.nextInt(100) < (style == 1 ? 28 : style == 2 ? 24 : 18)) hit = true;
            if (hit && rng.nextInt(100) < 15) hit = false;
            answer.gates[static_cast<size_t>(i)] = hit;
            const int offset = intervals[style][rng.nextInt(5)];
            const int originalNote = original.notes[static_cast<size_t>(source)];
            answer.notes[static_cast<size_t>(i)] = juce::jlimit(24, 84,
                (rng.nextInt(100) < 58 ? originalNote : root + offset) + (style == 1 && bar % 4 == 3 ? 12 : 0));
            answer.velocities[static_cast<size_t>(i)] = juce::jlimit(40, 125,
                original.velocities[static_cast<size_t>(source)] + rng.nextInt(25) - 12);
        }
        // A short closing gesture leads back to the original phrase.
        const int finalStep = bar * 16 + 15;
        if (bar % 2 == 1 && rng.nextBool())
        {
            answer.gates[static_cast<size_t>(finalStep)] = true;
            answer.notes[static_cast<size_t>(finalStep)] = root + (style == 1 ? 7 : 0);
            answer.velocities[static_cast<size_t>(finalStep)] = 98;
        }
    }
    {
        const juce::ScopedLock lock(patternLock);
        responsePattern = answer;
    }
    responseAvailable.store(true);
    phraseView.store(1);
}

bool TechHouseBassLab::editNote(int sourceStep, int targetStep, int newPitch, bool erase)
{
    const auto snapshot = getDisplayedPatternSnapshot();
    if (sourceStep < 0 || sourceStep >= snapshot.steps || targetStep < 0 || targetStep >= snapshot.steps) return false;
    const int view = phraseView.load() == 1 && responseAvailable.load() ? 1 : 0;
    if (view == 0) getPatternSnapshot();
    const juce::ScopedLock lock(patternLock);
    auto& gg = view ? responsePattern.gates : gates;
    auto& nn = view ? responsePattern.notes : pitches;
    auto& vv = view ? responsePattern.velocities : velocities;
    if (!gg[static_cast<size_t>(sourceStep)]) return false;
    const int velocity = vv[static_cast<size_t>(sourceStep)];
    gg[static_cast<size_t>(sourceStep)] = false;
    if (!erase)
    {
        gg[static_cast<size_t>(targetStep)] = true;
        nn[static_cast<size_t>(targetStep)] = juce::jlimit(24, 84, newPitch);
        vv[static_cast<size_t>(targetStep)] = velocity;
    }
    return true;
}

bool TechHouseBassLab::addNote(int step, int pitch)
{
    const auto snapshot = getDisplayedPatternSnapshot();
    if (step < 0 || step >= snapshot.steps) return false;
    const int view = phraseView.load() == 1 && responseAvailable.load() ? 1 : 0;
    const juce::ScopedLock lock(patternLock);
    auto& gg = view ? responsePattern.gates : gates;
    auto& nn = view ? responsePattern.notes : pitches;
    auto& vv = view ? responsePattern.velocities : velocities;
    gg[static_cast<size_t>(step)] = true;
    nn[static_cast<size_t>(step)] = juce::jlimit(24, 84, pitch);
    vv[static_cast<size_t>(step)] = 105;
    return true;
}

TechHouseBassLab::PatternSnapshot TechHouseBassLab::getDisplayedPatternSnapshot()
{
    const auto original = getPatternSnapshot();
    if (phraseView.load() == 0 || !responseAvailable.load()) return original;
    const juce::ScopedLock lock(patternLock);
    return responsePattern;
}

bool TechHouseBassLab::exportMidi(const juce::File& file)
{
    const auto original = getPatternSnapshot();
    const int view = responseAvailable.load() ? phraseView.load() : 0;
    PatternSnapshot answer;
    if (view != 0)
    {
        const juce::ScopedLock lock(patternLock);
        answer = responsePattern;
    }
    constexpr int ticksPerQuarter = 960;
    constexpr double ticksPerStep = ticksPerQuarter / 4.0;
    juce::MidiMessageSequence sequence;
    sequence.addEvent(juce::MidiMessage::tempoMetaEvent(
        juce::roundToInt(60000000.0 / juce::jlimit(60.0, 200.0, original.bpm))), 0.0);
    sequence.addEvent(juce::MidiMessage::timeSignatureMetaEvent(4, 4), 0.0);
    auto append = [&](const PatternSnapshot& pattern, int stepOffset)
    {
        for (int i = 0; i < pattern.steps; ++i)
        {
            if (!pattern.gates[static_cast<size_t>(i)]) continue;
            const double start = (stepOffset + i) * ticksPerStep + ((i % 2) ? pattern.swing * ticksPerStep : 0.0);
            const double end = start + juce::jmax(1.0, pattern.gateLength * ticksPerStep);
            const int note = juce::jlimit(0, 127, pattern.notes[static_cast<size_t>(i)]);
            sequence.addEvent(juce::MidiMessage::noteOn(1, note,
                static_cast<juce::uint8>(pattern.velocities[static_cast<size_t>(i)])), start);
            sequence.addEvent(juce::MidiMessage::noteOff(1, note), end);
        }
    };
    if (view == 0 || view == 2) append(original, 0);
    if (view == 1) append(answer, 0);
    if (view == 2) append(answer, original.steps);
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
    const int style = static_cast<int>(state.getRawParameterValue("style")->load());
    const int groove = static_cast<int>(state.getRawParameterValue("groove")->load());
    const int root = static_cast<int>(state.getRawParameterValue("root")->load());
    const int density = static_cast<int>(state.getRawParameterValue("density")->load());
    const int variation = static_cast<int>(state.getRawParameterValue("variation")->load());
    PatternSnapshot snapshot;
    {
        const juce::ScopedLock lock(patternLock);
        // Rebuild deterministically when a parameter changes, even while transport is stopped.
        if (seed != previousSeed || style != previousStyle || groove != previousGroove || root != previousRoot
            || density != previousDensity || variation != previousVariation)
        {
            // regenerate() also takes patternLock, so release before calling below.
        }
    }
    if (seed != previousSeed || style != previousStyle || groove != previousGroove || root != previousRoot
        || density != previousDensity || variation != previousVariation)
    {
        regenerate(seed);
        previousSeed = seed;
        previousStyle = style;
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
