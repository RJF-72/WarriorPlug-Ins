#ifndef WARRIOR_SYNTH_ENGINE_H
#define WARRIOR_SYNTH_ENGINE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

// Opaque handle
typedef struct { void* ptr; } SynthHandle;

// Parameters (must match Rust layout)
typedef struct {
    float gain_db;
    float cutoff;
    float resonance;
    float attack;
    float decay;
    float sustain;
    float release;
    int32_t unison;
} SynthParams;

// Lifecycle
SynthHandle synth_new(float sample_rate);
void synth_free(SynthHandle h);

// Control
void synth_set_params(SynthHandle h, SynthParams p);
void synth_note_on(SynthHandle h, uint8_t note, uint8_t vel);
void synth_note_off(SynthHandle h, uint8_t note);

// Render: interleaved stereo should be split into two buffers before calling
void synth_render(SynthHandle h, float* out_l, float* out_r, size_t frames);

// Sample-based instrument loading
bool synth_load_sample_instrument(SynthHandle h, uint8_t root_note, const uint8_t* wav_path, size_t wav_len);
void synth_instr_note_on(SynthHandle h, uint8_t note, uint8_t vel);
void synth_instr_note_off(SynthHandle h, uint8_t note);
bool synth_load_sfz_instrument(SynthHandle h, const uint8_t* sfz_path, size_t sfz_len);

#ifdef __cplusplus
}
#endif

#endif // WARRIOR_SYNTH_ENGINE_H
