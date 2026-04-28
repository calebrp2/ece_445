/* rf_waveform_classifier.h */
#ifndef RF_WAVEFORM_CLASSIFIER_H
#define RF_WAVEFORM_CLASSIFIER_H

#include <stdint.h>

#define RF_N_FEATURES  7
#define RF_N_CLASSES   5
#define RF_N_TREES     100

typedef enum {
    RF_SINE      = 0,
    RF_SQUARE    = 1,
    RF_SAWTOOTH  = 2,
    RF_TRIANGLE  = 3,
    RF_PULSE_25  = 4,
} RF_Waveform_t;

typedef struct {
    RF_Waveform_t waveform;
    uint8_t       confidence_pct;
    float         confidence[RF_N_CLASSES];
} RF_Result_t;

#define RF_FEAT_CREST    0
#define RF_FEAT_THD      1
#define RF_FEAT_SYMMETRY 2
#define RF_FEAT_H2_H1    3
#define RF_FEAT_H3_H1    4
#define RF_FEAT_H4_H1    5
#define RF_FEAT_ROLLOFF  6

void        RF_Classify(const uint16_t *adc_buf, RF_Result_t *result);
void        RF_ClassifyFeatures(const float *features, RF_Result_t *result);
const char *RF_GetLabel(RF_Waveform_t w);

#endif /* RF_WAVEFORM_CLASSIFIER_H */
