/**
 * @file    waveform_classifier.h
 * @brief   Time and frequency domain feature extraction for RF waveform classifier.
 */

#ifndef INC_WAVEFORM_CLASSIFIER_H_
#define INC_WAVEFORM_CLASSIFIER_H_

#include <stdint.h>

typedef struct {
    float    crest;        /* Peak-to-RMS ratio after DC removal              */
    float    thd_pct;      /* Total Harmonic Distortion (%) relative to H1    */
    float    symmetry;     /* Fraction of samples above the signal mean        */
    float    h2_h1_ratio;  /* 2nd harmonic magnitude / fundamental magnitude   */
    float    h3_h1_ratio;  /* 3rd harmonic magnitude / fundamental magnitude   */
    float    h4_h1_ratio;  /* 4th harmonic magnitude / fundamental magnitude   */
    uint32_t rolloff_bin;  /* Smallest FFT bin where cumul. energy >= 85%      */
} WC_Features_t;

typedef struct {
    WC_Features_t features;
} WC_Result_t;

void WC_Classify(const uint16_t *adc_buf, WC_Result_t *result);

#endif /* INC_WAVEFORM_CLASSIFIER_H_ */
