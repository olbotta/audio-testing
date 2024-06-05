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

SCENARIO ("Wet Parameter influence on buffer value", "[drywet][parameters][amplitude]")
{
    auto effectedBuffer = Generators::generateNoise (channelsNumber, samplesPerBlock);
    auto originalBuffer (*effectedBuffer);

    testPluginProcessor->prepareToPlay (sampleRate, samplesPerBlock);

    auto const* parameters = testPluginProcessor->getParameters();
    juce::RangedAudioParameter* dryWetParam = parameters->getParameter (NAME_DW);
    juce::RangedAudioParameter* cutOffParam = parameters->getParameter (NAME_CUTOFF);

    GIVEN ("dry/wet ratio = 1.0f implies no change to the signal")
    {
        dryWetParam->setValueNotifyingHost (1.0f);
        testPluginProcessor->processBlock (*effectedBuffer, midiBuffer);

        REQUIRE_THAT (*effectedBuffer, matches (originalBuffer));
        REQUIRE_THAT (*effectedBuffer, matchesFrequencyBinsOf (originalBuffer));
    }

    auto dryWetValue = GENERATE (0.0f, 0.09f, 0.1f, 0.5f, 0.9f);
    auto cutOffValue = GENERATE (100.0f, 1000.0f, 10000.0f);

    GIVEN ("Low pass with dry/wet = " << dryWetValue << " and cutoff = " << cutOffValue)
    {
        dryWetParam->setValueNotifyingHost (dryWetValue);
        cutOffParam->setValueNotifyingHost (cutOffValue);
        testPluginProcessor->processBlock (*effectedBuffer, midiBuffer);

        THEN ("signal is not the same")
        {
            REQUIRE_THAT (*effectedBuffer, !matches (originalBuffer));
        }
        AND_THEN ("signal cumulative energy decreases")
        {
            REQUIRE_THAT (*effectedBuffer, hasLowerCumulativeEnergyThan (originalBuffer));
        }
    }

    // TEARDOWN
    testPluginProcessor->releaseResources();
}

TEST_CASE ("Low pass filter lowers energy in the higher bins of noise signal", "[energy][parameter][frequency]")
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
    REQUIRE_THAT (*effectedBuffer, hasLowerFftBinEnergyThan (originalBuffer,effectedRange));
    REQUIRE_THAT(*effectedBuffer, matchesFrequencyBinsOf(originalBuffer,notEffectedRange));

    // TEARDOWN
    testPluginProcessor->releaseResources();
}
