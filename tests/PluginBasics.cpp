#include "Helpers.h"
#include "Matchers.h"
#include <PluginProcessor.h>
#include <catch2/catch_all.hpp>
#include <juce_audio_processors/juce_audio_processors.h>

TEST_CASE ("one is equal to one", "[dummy]")
{
    REQUIRE (1 == 1);
}

TEST_CASE ("Wet Parameter", "[parameters]")
{
    auto testPluginProcessor = std::make_unique<FilterAudioProcessor>();

    juce::AudioBuffer<float>* buffer = Helpers::noiseGenerator();
    juce::AudioBuffer<float> originalBuffer (*buffer);

    juce::MidiBuffer midiBuffer;

    testPluginProcessor->prepareToPlay (44100, 4096);

    auto const* parameters = testPluginProcessor->getParameters();
    juce::RangedAudioParameter* pParam = parameters->getParameter ("WET");

    SECTION ("wet=0 implies no change to the signal")
    {
        pParam->setValueNotifyingHost (0.0f);
        testPluginProcessor->processBlock (*buffer, midiBuffer);

        CHECK_THAT (*buffer, AudioBuffersMatch (originalBuffer));
    }

    SECTION ("wet!=0 implies change to the signal")
    {
        auto value = GENERATE (0.1f, 0.5f, 1.0f);

        pParam->setValueNotifyingHost (value);
        testPluginProcessor->processBlock (*buffer, midiBuffer);

        CHECK_THAT (*buffer, !AudioBuffersMatch (originalBuffer));
    }
}

TEST_CASE ("Big Buffer Wet Parameter", "[parameters]")
{
    int blockCount = GENERATE (1, 256); //1 corresponds to previous case

    auto testPluginProcessor = std::make_unique<FilterAudioProcessor>();

    juce::AudioBuffer<float>* buffer = Helpers::noiseGenerator (2, 4096 * blockCount);
    juce::AudioBuffer<float> originalBuffer (*buffer);
    //ImageProcessing::drawAudioBufferImage(buffer, "RandomWet");

    juce::MidiBuffer midiBuffer;

    testPluginProcessor->prepareToPlay (44100, 4096);

    auto const* parameters = testPluginProcessor->getParameters();
    juce::RangedAudioParameter* pParam = parameters->getParameter ("WET");

    SECTION ("wet=0 implies no change to the signal")
    {
        pParam->setValueNotifyingHost (0.0f);

        for (int i = 0; i < blockCount; i++)
        {
            auto processBuffer = std::make_unique<juce::AudioBuffer<float>> (1, 4096);
            processBuffer->copyFrom (0, 0, *buffer, 0, i * 4096, 4096);
            testPluginProcessor->processBlock (*processBuffer, midiBuffer);
            buffer->copyFrom (0, i * 4096, *processBuffer, 0, 0, 4096);
        }

        CHECK_THAT (*buffer, AudioBuffersMatch (originalBuffer));
    }

    SECTION ("wet!=0 implies change to the signal")
    {
        auto value = GENERATE (0.1f, 0.5f, 1.0f);
        pParam->setValueNotifyingHost (value);

        for (int i = 0; i < blockCount; i++)
        {
            auto processBuffer = std::make_unique<juce::AudioBuffer<float>> (1, 4096);
            processBuffer->copyFrom (0, 0, *buffer, 0, i * 4096, 4096);
            testPluginProcessor->processBlock (*processBuffer, midiBuffer);
            buffer->copyFrom (0, i * 4096, *processBuffer, 0, 0, 4096);
        }

        CHECK_THAT (*buffer, !AudioBuffersMatch (originalBuffer));
    }
}

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
}
