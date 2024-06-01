#include "PluginProcessor.h"

FilterAudioProcessor::FilterAudioProcessor()
    : parameters (*this, nullptr, "Filter Parameters", Parameters::createParameterLayout())
{
    parameters.addParameterListener (NAME_DW, this);
    parameters.addParameterListener (NAME_CUTOFF, static_cast<Listener*> (this));
    drywetter.setDryWetRatio (DEFAULT_DW);
    cutoff = 1000.0f;
    tptFilter.setType(juce::dsp::StateVariableTPTFilterType::highpass);
}

//==============================================================================
void FilterAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    this->fs = sampleRate;
    drywetter.prepareToPlay (sampleRate, samplesPerBlock);

    juce::dsp::ProcessSpec spec { sampleRate, static_cast<juce::uint32> (samplesPerBlock), 2 };
    tptFilter.prepare(spec);
}

void FilterAudioProcessor::releaseResources()
{
    drywetter.releaseResources();
    filter.reset();
}

void FilterAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    drywetter.setDry (buffer);

    juce::dsp::AudioBlock<float> block (buffer);
    juce::dsp::ProcessContextReplacing<float> context (block);
    tptFilter.process(context);

    // Miscelo il segnale pulito salvato in drywetter con quello processato da delay
    drywetter.merge (buffer);
}

//==============================================================================
void FilterAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void FilterAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState != nullptr)
        if (xmlState->hasTagName (parameters.state.getType()))
            parameters.replaceState (juce::ValueTree::fromXml (*xmlState));
}

void FilterAudioProcessor::parameterChanged (const juce::String& paramID, float newValue)
{
    if (paramID == NAME_DW)
        drywetter.setDryWetRatio (newValue);
    else if (paramID == NAME_CUTOFF)
        tptFilter.setCutoffFrequency(newValue);
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    //return new FilterAudioProcessor();
        return nullptr;
}

juce::AudioProcessorValueTreeState* FilterAudioProcessor::getParameters()
{
    return &parameters;
}
