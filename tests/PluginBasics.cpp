#include "helpers/Helpers.h"
#include "helpers/Matchers.h"
#include "helpers/sine sweep/SineSweep.h"
#include <PluginProcessor.h>
#include <catch2/catch_all.hpp>
#include <juce_audio_processors/juce_audio_processors.h>

const int samplesPerBlock = 4096;
const int sampleRate = 44100;
const int channelsNumber = 2;

TEST_CASE ("one is equal to one", "[dummy]")
{
    REQUIRE (1 == 1);
}

TEST_CASE ("Wet Parameter influence on buffer", "[parameters]")
{
    auto testPluginProcessor = std::make_unique<FilterAudioProcessor>();
    auto buffer = Helpers::noiseGenerator(channelsNumber,samplesPerBlock);
    auto originalBuffer (*buffer);
    juce::MidiBuffer midiBuffer;

    testPluginProcessor->prepareToPlay (sampleRate, samplesPerBlock);

    auto const* parameters = testPluginProcessor->getParameters();
    juce::RangedAudioParameter* pParam = parameters->getParameter (NAME_DW);

    SECTION ("dry/wet ratio = 1.0f implies no change to the signal")
    {
        pParam->setValueNotifyingHost (1.0f);
        testPluginProcessor->processBlock (*buffer, midiBuffer);

        CHECK_THAT (*buffer, AudioBuffersMatch (originalBuffer));
    }

    SECTION ("dry/wet ratio < 1.0f implies change to the signal")
    {
        auto value = GENERATE (0.1f, 0.1f, 0.5f);

        pParam->setValueNotifyingHost (value);
        testPluginProcessor->processBlock (*buffer, midiBuffer);

            SECTION("signal is not the same")
            {
                CHECK_THAT (*buffer, !AudioBuffersMatch (originalBuffer));
            }
            SECTION("signal cumulative energy decreases")
            {
                CHECK_THAT (*buffer, AudioBufferLowerEnergy (originalBuffer));
            }
    }

    // TEARDOWN
    testPluginProcessor->releaseResources();
}

/*
TEST_CASE ("Filter", "[functionality]")
{
    int samplesPerBlock = 4096;
    int sampleRate = 44100;
    auto testPluginProcessor = std::make_unique<FilterAudioProcessor>();

    juce::MemoryMappedAudioFormatReader* reader = Helpers::readSineSweep();
    auto sweep = std::make_unique<juce::AudioBuffer<float>> (reader->numChannels, reader->lengthInSamples);
    reader->read (sweep->getArrayOfWritePointers(), 1, 0, reader->lengthInSamples);
    juce::AudioBuffer<float> originalBuffer (*sweep);

    int blockCount = sweep->getNumSamples() / samplesPerBlock; //TODO: take into account last partial block

    juce::MidiBuffer midiBuffer;

    testPluginProcessor->prepareToPlay (sampleRate, samplesPerBlock);

    for (int i = 0; i < blockCount; i++)
    {
        juce::AudioBuffer<float> processBuffer (sweep->getNumChannels(), samplesPerBlock);
        for (int ch = 0; ch < sweep->getNumChannels(); ++ch)
        {
            processBuffer.copyFrom (0, 0, *sweep, ch, i * samplesPerBlock, samplesPerBlock);
        }

        testPluginProcessor->processBlock (processBuffer, midiBuffer);

        for (int ch = 0; ch < sweep->getNumChannels(); ++ch)
        {
            sweep->copyFrom (0, i * samplesPerBlock, processBuffer, ch, 0, samplesPerBlock);
        }
    }

    SECTION ("filter lowers total energy")
    {
        CHECK_THAT (originalBuffer, !AudioBufferHigherEnergy (*sweep));
    }

    // SECTION ("filter lowers energy in bins 0 to 32")
    // {
    //     // auto maxEnergies = juce::Array<float> (fft_size / 2);
    //     // juce::FloatVectorOperations::fill (maxEnergies.begin(), 100, 33);
    //     // juce::FloatVectorOperations::fill (maxEnergies.begin() + 32, -1, maxEnergies.size() - 32);
    //     // //std::fill (maxEnergies.begin(), maxEnergies.begin() + 32, 100);
    //     // //std::fill (maxEnergies.begin() + 32, maxEnergies.end()-1, -1);
    //     REQUIRE(1==1);
    //     // CHECK_THAT (originalBuffer, !AudioBufferHigherEnergy (*buffer));
    //     // //CHECK_THAT (*buffer, AudioBufferCheckMaxEnergy (maxEnergies));
    // }
}*/
