# JUCE Audio Processors Testing with CATCH2
Starting from the PAMPLEJUCE template, here is an implementation on how to implement some unit testing of a lowpass filter.
you can find the classes filter class that is tested in `/source`.
The test cases can be found in `tests/FilterTests.cpp`

## Usage

```shell
cmake -B build
cmake --build ./build --parallel
./build/Tests
```

## Acknowledgements
Based on the [PAMPLEJUCE](https://github.com/sudara/pamplejuce/) template, code takes also some inspiration from [here](https://github.com/ejaaskel/FilterUnitTest).