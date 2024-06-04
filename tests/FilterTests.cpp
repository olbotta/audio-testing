#include "helpers/Helpers.h"
#include "helpers/Matchers.h"
#include <PluginProcessor.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <utility>

const int samplesPerBlock = 4096;
const int sampleRate = 44100;
const int channelsNumber = 2;

juce::MidiBuffer midiBuffer;
auto testPluginProcessor = std::make_unique<FilterAudioProcessor>();
//std::cout << "one";

TEST_CASE ("one is equal to one", "[dummy]")
{
    REQUIRE (1 == 1);
}

TEST_CASE ("Wet Parameter influence on buffer", "[parameters]")
{
    //auto testPluginProcessor = std::make_unique<FilterAudioProcessor>();
    auto effectedBuffer = Helpers::generateNoise(channelsNumber,samplesPerBlock);
    auto originalBuffer (*effectedBuffer);

    testPluginProcessor->prepareToPlay (sampleRate, samplesPerBlock);

    auto const* parameters = testPluginProcessor->getParameters();
    juce::RangedAudioParameter* dryWetParam = parameters->getParameter (NAME_DW);

    SECTION ("dry/wet ratio = 1.0f implies no change to the signal")
    {
        dryWetParam->setValueNotifyingHost (1.0f);
        testPluginProcessor->processBlock (*effectedBuffer, midiBuffer);

        CHECK_THAT (*effectedBuffer, AudioBuffersMatch (originalBuffer));
    }

    SECTION ("dry/wet ratio < 1.0f implies change to the signal")
    {
        auto dryWetValue = GENERATE (0.0f, 0.5f, 0.9f);

        dryWetParam->setValueNotifyingHost (dryWetValue);
        testPluginProcessor->processBlock (*effectedBuffer, midiBuffer);

            SECTION("signal is not the same")
            {
                CHECK_THAT (*effectedBuffer, !AudioBuffersMatch (originalBuffer));
            }
            SECTION("signal cumulative energy decreases")
            {
                CHECK_THAT (*effectedBuffer, HasLowerCumulativeEnergyThan (originalBuffer));
            }
    }

    // TEARDOWN
    testPluginProcessor->releaseResources();
}

TEST_CASE ("Filter lowers energy in the lower 32 bins of sine sweep", "[functionality]")
{
    // auto effectedBuffer = Helpers::generateSineSweep(channelsNumber,samplesPerBlock,sampleRate);
    auto effectedBuffer = Helpers::generateNoise(channelsNumber,samplesPerBlock);

    testPluginProcessor->prepareToPlay (sampleRate, samplesPerBlock);

    auto const* parameters = testPluginProcessor->getParameters();
    juce::RangedAudioParameter* cutoffParam = parameters->getParameter (NAME_CUTOFF);

    auto cutoffValue = GENERATE (100.0f, 1000.0f, 10000.0f);

    cutoffParam->setValueNotifyingHost (cutoffValue);
    testPluginProcessor->processBlock (*effectedBuffer, midiBuffer);

    auto comparisonBins = juce::Array<float> ();
    int limitBin = (cutoffValue/20000.0f)*(fft_size);
    std::cout<<limitBin<<"\n";

    comparisonBins.insertMultiple(0,100.0f,limitBin);
    comparisonBins.insertMultiple(limitBin,-1.0f,fft_size);

    CHECK_THAT (*effectedBuffer, HasLowerFftBinEnergyThan (comparisonBins));
    //controlla la cosa sbagliata: andrebbe controllato che la parte filtrata si abbassa
    //mentre quelle non filtrata no
}
