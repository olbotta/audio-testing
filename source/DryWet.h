#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
//#include <juce_SmoothedValue.h>
#include "Parameters.h"
#include <cmath>

class DryWet
{
public:
    DryWet() = default;
    ~DryWet() = default;

    void prepareToPlay (double sr, int maxNumSamples)
    {
        drySignal.setSize (2, maxNumSamples);

        smoothedDryLevel.reset (sr, SMOOTHING_TIME);
        smoothedWetLevel.reset (sr, SMOOTHING_TIME);
        setDryWetRatio (dryWetRatio);
    }

    void releaseResources()
    {
        drySignal.setSize (0, 0);
    }

    void setDry (const juce::AudioBuffer<float>& buffer)
    {
        //questo ciclo strano è per chiamare getnumchannels solo una volta, e non ad ogni confronto del for
        // quindi bisogna fare quella magia con il --ch
        for (int ch = buffer.getNumChannels(); --ch >= 0;)
            drySignal.copyFrom (ch, 0, buffer, ch, 0, buffer.getNumSamples());
    }

    void merge (juce::AudioBuffer<float>& wetBuffer)
    {
        auto numSamples = wetBuffer.getNumSamples();

        smoothedWetLevel.applyGain (wetBuffer, numSamples); //v3
        smoothedDryLevel.applyGain (drySignal, numSamples); //v3 oss: scrive numSamples! non drySignal.getNumSamples()

        for (int ch = wetBuffer.getNumChannels(); --ch >= 0;) //ciclo strano di presti che scorre i canali al contrario
        { //add stored dry buffer to wet buffer
            wetBuffer.addFrom (ch, 0, drySignal, ch, 0, numSamples); //v1,v3
            // wetBuffer.addFrom(ch, 0, drySignal, ch, 0, numSamples, dryLevel); //v2, più compatta e ottimizzata
        }
    }

    void setDryWetRatio (const float newValue)
    {
        dryWetRatio = newValue;
        smoothedWetLevel.setTargetValue (sqrt (1 - dryWetRatio));
        smoothedDryLevel.setTargetValue (sqrt (dryWetRatio));
    }

private:
    float dryWetRatio = 0.5;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedDryLevel;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedWetLevel;

    juce::AudioBuffer<float> drySignal;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DryWet);
};
