#include "helpers/Generators.h"
#include "helpers/Matchers.h"
#include <PluginProcessor.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <utility>

const int samplesPerBlock = 4096;
const int sampleRate = 44100;
const int channelsNumber = 2;

juce::MidiBuffer midiBuffer;
auto testPluginProcessor = std::make_unique<FilterAudioProcessor>();

TEST_CASE ("one is equal to one", "[dummy]")
{
    REQUIRE (1 == 1);
}

TEST_CASE ("Wet Parameter influence on buffer", "[drywet]")
{
    auto effectedBuffer = Generators::generateNoise(channelsNumber,samplesPerBlock);
    auto originalBuffer (*effectedBuffer);

    testPluginProcessor->prepareToPlay (sampleRate, samplesPerBlock);

    auto const* parameters = testPluginProcessor->getParameters();
    juce::RangedAudioParameter* dryWetParam = parameters->getParameter (NAME_DW);
    juce::RangedAudioParameter* cutOffParam = parameters->getParameter (NAME_CUTOFF);

    SECTION ("dry/wet ratio = 1.0f implies no change to the signal")
    {
        dryWetParam->setValueNotifyingHost (1.0f);
        testPluginProcessor->processBlock (*effectedBuffer, midiBuffer);

        CHECK_THAT (*effectedBuffer, matches (originalBuffer));
        CHECK_THAT(*effectedBuffer, matchesFrequencyBinsOf(originalBuffer));
    }

    SECTION ("dry/wet ratio < 1.0f implies change to the signal")
    {
        auto dryWetValue = GENERATE (0.0f, 0.09f, 0.1f, 0.5f, 0.9f);
        auto cutOffValue = GENERATE (100.0f, 1000.0f, 10000.0f);

        INFO("Low pass with wet/dry = "<<dryWetValue<<" cutoff = "<<cutOffValue);
        dryWetParam->setValueNotifyingHost (dryWetValue);
        cutOffParam->setValueNotifyingHost (cutOffValue);
        testPluginProcessor->processBlock (*effectedBuffer, midiBuffer);

            SECTION("signal is not the same")
            {
                CHECK_THAT(*effectedBuffer, !matches (originalBuffer));
            }
            SECTION("signal cumulative energy decreases")
            {
                CHECK_THAT (*effectedBuffer, hasLowerCumulativeEnergyThan (originalBuffer));
            }
    }

    // TEARDOWN
    testPluginProcessor->releaseResources();
}

TEST_CASE ("Low pass filter lowers energy in the higher bins of noise signal", "[energy]")
{
    // SETUP
    auto effectedBuffer = Generators::generateNoise(channelsNumber,samplesPerBlock);
    auto originalBuffer (*effectedBuffer);

    testPluginProcessor->prepareToPlay (sampleRate, samplesPerBlock);

    auto const* parameters = testPluginProcessor->getParameters();
    juce::RangedAudioParameter* cutoffParam = parameters->getParameter (NAME_CUTOFF);

    auto cutoffValue = GENERATE (2000.0f, 1000.0f, 10000.0f);

    cutoffParam->setValueNotifyingHost (cutoffValue);
    testPluginProcessor->processBlock (*effectedBuffer, midiBuffer);

    auto comparisonBins = juce::Array<float> ();
    auto cutoffBin = (int)std::floor((cutoffValue/20000.0f)*(FFT_SIZE/2));
    auto notEffectedRange = juce::makeRange(0, cutoffBin-1);
    auto effectedRange = juce::makeRange(cutoffBin, FFT_SIZE/2);

    // VERIFY
    INFO("freq = "<<cutoffValue<<", begin bin: "<<notEffectedRange.begin()<<", cutoff bin: "<<effectedRange.begin()<<", end bin: "<<effectedRange.end());
    CHECK_THAT (*effectedBuffer, hasLowerFftBinEnergyThan (originalBuffer,effectedRange));
    CHECK_THAT(*effectedBuffer, matchesFrequencyBinsOf(originalBuffer,notEffectedRange));

    // TEARDOWN
    testPluginProcessor->releaseResources();
}
