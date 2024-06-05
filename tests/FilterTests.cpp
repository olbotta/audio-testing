#include "helpers/Generators.h"
#include "helpers/Matchers.h"
#include <PluginProcessor.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <utility>

const int samplesPerBlock = 4096;
const int sampleRate = 44100;
const int channelsNumber = 2;

//juce::MidiBuffer midiBuffer;
//auto testPluginProcessor = std::make_unique<FilterAudioProcessor>();
//std::cout << "one";

TEST_CASE ("one is equal to one", "[dummy]")
{
    REQUIRE (1 == 1);
}

TEST_CASE ("Wet Parameter influence on buffer", "[parameters]")
{
    auto testPluginProcessor = std::make_unique<FilterAudioProcessor>();
    juce::MidiBuffer midiBuffer;
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
                CHECK_THAT (*effectedBuffer, HasLowerCumulativeEnergyThan (originalBuffer));
            }
    }

    // TEARDOWN
    testPluginProcessor->releaseResources();
}

TEST_CASE ("Low pass filter lowers energy in the higher bins of noise signal", "[functionality]")
{
    // SETUP
    auto testPluginProcessor = std::make_unique<FilterAudioProcessor>();
    juce::MidiBuffer midiBuffer;
    auto effectedBuffer = Generators::generateNoise(channelsNumber,samplesPerBlock);
    auto originalBuffer (*effectedBuffer);

    testPluginProcessor->prepareToPlay (sampleRate, samplesPerBlock);

    auto const* parameters = testPluginProcessor->getParameters();
    juce::RangedAudioParameter* cutoffParam = parameters->getParameter (NAME_CUTOFF);

    auto cutoffValue = GENERATE (2000.0f);//, 1000.0f, 10000.0f);

    cutoffParam->setValueNotifyingHost (cutoffValue);
    testPluginProcessor->processBlock (*effectedBuffer, midiBuffer);

    auto comparisonBins = juce::Array<float> ();
    auto range = juce::makeRange((int)((cutoffValue/20000.0f)*(FFT_SIZE*2)), FFT_SIZE);
    //auto range = juce::makeRange(FFT_SIZE-550, FFT_SIZE);

    // VERIFY
    std::cout<<range.begin()<<" "<<range.end()<<"\n";
    CHECK_THAT (*effectedBuffer, HasLowerFftBinEnergyThan (originalBuffer,range));

    // TEARDOWN
    testPluginProcessor->releaseResources();
}
