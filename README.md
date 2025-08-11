# JUCE Audio Processors Testing with CATCH2

Starting from the PAMPLEJUCE template, here is an example on how to implement some unit testing of a lowpass filter.
Both time domain and frequency domain tests were implemented.

**Note that the examples provided still have big margins of improvements, this is more of a proof of concept.**

The filter class is located in `/source` and test cases can be found in `tests/FilterTests.cpp`

## How to start
assuming you already know JUCE and how to use its CMake integration:

- read [CATCH2 documentation](https://github.com/catchorg/Catch2/tree/devel/docs)
- implement your own JUCE class and put it in `/source`
- think about some meaningful tests and implement them in `tests`, note that you will probably need to write your own [custom matchers](https://github.com/catchorg/Catch2/blob/devel/docs/matchers.md#writing-custom-matchers-new-style)

## Build

```shell
cmake -B build
cmake --build ./build --parallel
```

## Run tests
```shell
./build/Tests
```

## Acknowledgements
Based on the [PAMPLEJUCE](https://github.com/sudara/pamplejuce/) template, code takes also some inspiration from [here](https://github.com/ejaaskel/FilterUnitTest).
