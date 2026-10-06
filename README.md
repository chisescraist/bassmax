# CHISESCRAIST VOX 0.1

Base VST3/Standalone de efectos vocales en tiempo real para Ableton Live y producción Tech House.

## Incluye
- Input / Output
- Compressor
- Drive
- DARK
- RADIO
- FILTER
- STUTTER sincronizado 1/4, 1/8, 1/16, 1/32
- Delay sincronizado
- THROW
- Reverb
- Lectura del BPM del host
- Guardado de parámetros

## Todavía NO incluye
Pitch/Formant, CHILD, FEMALE y transformaciones avanzadas. Se agregan después de validar el núcleo.

## Requisitos
- Windows 10/11
- Visual Studio 2022 con Desktop development with C++
- CMake 3.22+
- JUCE
- Ableton Live 12 para pruebas

## JUCE como submódulo
git submodule add https://github.com/juce-framework/JUCE.git JUCE
git submodule update --init --recursive

## Configurar
cmake -B build -G "Visual Studio 17 2022" -A x64

## Compilar
cmake --build build --config Release

## VST3
El artefacto estará normalmente bajo:
build/ChisescraistVOX_artefacts/Release/VST3/

Copiar CHISESCRAIST VOX.vst3 a:
C:\Program Files\Common Files\VST3

Luego hacer rescan en Ableton Live.

Workflow incluido: .github/workflows/build.yml
