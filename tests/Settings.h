#ifndef TEST_SETTINGS
#define TEST_SETTINGS
const int FFT_ORDER = 10;
const int FFT_SIZE = 1 << FFT_ORDER;

const int SEED = 42;
static const float FILTER_TOLERANCE = 0.2f;
static const float EQUALITY_TOLERANCE = std::numeric_limits<float>::epsilon();
#endif
