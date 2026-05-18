#include "logger.h"
#include "apu.h"
#include "bus.h"
#include <SDL2/SDL.h>
#include <string.h>

// SAMPLE RATE WE PRESENT TO SDL. ANYTHING > 22050 IS FINE, 48000 IS COMMON
#define APU_SAMPLE_RATE 48000

// CPU TICKS AT ~4.194304 MHZ. ONE SAMPLE EVERY N CPU T-CYCLES:
//   ~4194304 / 48000 ~= 87.38
// WE TRACK A FRACTIONAL ACCUMULATOR TO AVOID DRIFT.
#define APU_CPU_HZ 4194304
#define SAMPLE_PERIOD_T (APU_CPU_HZ / APU_SAMPLE_RATE) // 87
#define SAMPLE_PERIOD_REMAINDER (APU_CPU_HZ % APU_SAMPLE_RATE)

// FRAME SEQUENCER STEPS AT 512 HZ -> EVERY 8192 T-CYCLES
#define FRAME_SEQ_PERIOD_T 8192

// AUDIO QUEUE SIZE TARGET (KEEP A SMALL BUFFER TO AVOID UNDERRUNS).
#define AUDIO_QUEUE_TARGET_BYTES (4 * 1024)
#define AUDIO_QUEUE_MAX_BYTES    (16 * 1024)

// SAMPLE BUFFER WE FILL PER FRAME-ISH AND QUEUE TO SDL
#define BATCH_SAMPLES 512
static i16 sample_buf[BATCH_SAMPLES * 2];  // STEREO
static size_t sample_buf_pos = 0;

// SDL AUDIO STATE
static SDL_AudioDeviceID audio_dev = 0;

// DUTY PATTERNS FOR SQUARE CHANNELS (8-STEP WAVEFORMS, 1=HIGH)
static const u8 DUTY_PATTERNS[4] = {
    0x01,  // 12.5%   0 0 0 0 0 0 0 1
    0x81,  // 25%     1 0 0 0 0 0 0 1
    0x87,  // 50%     1 0 0 0 0 1 1 1
    0x7E   // 75%     0 1 1 1 1 1 1 0
};

// NOISE DIVISOR TABLE (FOR NR43 LOWER NIBBLE)
static const u16 NOISE_DIVISORS[8] = {8, 16, 32, 48, 64, 80, 96, 112};

typedef struct {
    // REGISTERS
    u8 nr10, nr11, nr12, nr13, nr14;  // CH1
    u8 nr21, nr22, nr23, nr24;        // CH2
    u8 nr30, nr31, nr32, nr33, nr34;  // CH3
    u8 nr41, nr42, nr43, nr44;        // CH4
    u8 nr50, nr51, nr52;
    u8 wave_ram[16];

    // POWER
    bool power;

    // FRAME SEQUENCER
    u32 frame_seq_counter;  // T-CYCLES TO NEXT STEP
    u8  frame_seq_step;     // 0..7

    // SAMPLE TIMING
    u32 sample_acc;         // T-CYCLES ACCUMULATED TOWARD NEXT SAMPLE
    u32 sample_remainder;   // FRACTIONAL REMAINDER ACCUMULATOR

    // CHANNEL 1 (SQUARE WITH SWEEP)
    bool ch1_enabled;
    u16  ch1_freq_timer;
    u8   ch1_duty_pos;
    u8   ch1_volume;
    u8   ch1_env_timer;
    u8   ch1_env_period;
    bool ch1_env_up;
    u16  ch1_length;        // 0..64
    bool ch1_length_enable;
    // SWEEP
    bool ch1_sweep_enable;
    u8   ch1_sweep_timer;
    u8   ch1_sweep_period;
    bool ch1_sweep_negate;
    u8   ch1_sweep_shift;
    u16  ch1_shadow_freq;

    // CHANNEL 2 (SQUARE)
    bool ch2_enabled;
    u16  ch2_freq_timer;
    u8   ch2_duty_pos;
    u8   ch2_volume;
    u8   ch2_env_timer;
    u8   ch2_env_period;
    bool ch2_env_up;
    u16  ch2_length;
    bool ch2_length_enable;

    // CHANNEL 3 (WAVE)
    bool ch3_enabled;
    u16  ch3_freq_timer;
    u8   ch3_sample_pos;    // 0..31 (32 4-BIT SAMPLES IN WAVE RAM)
    u16  ch3_length;        // 0..256
    bool ch3_length_enable;

    // CHANNEL 4 (NOISE)
    bool ch4_enabled;
    u16  ch4_freq_timer;
    u16  ch4_lfsr;          // 15-BIT LFSR
    u8   ch4_volume;
    u8   ch4_env_timer;
    u8   ch4_env_period;
    bool ch4_env_up;
    u16  ch4_length;        // 0..64
    bool ch4_length_enable;
} apu_state;

static apu_state apu;
static u16 ch1_frequency(void) { return ((apu.nr14 & 0x07) << 8) | apu.nr13; }
static u16 ch2_frequency(void) { return ((apu.nr24 & 0x07) << 8) | apu.nr23; }
static u16 ch3_frequency(void) { return ((apu.nr34 & 0x07) << 8) | apu.nr33; }

static void ch1_set_frequency(u16 f) {
    apu.nr13 = f & 0xFF;
    apu.nr14 = (apu.nr14 & 0xF8) | ((f >> 8) & 0x07);
}

// SQUARE PERIOD - (2048 - F) * 4 T-CYCLES
static u16 square_period(u16 f) { return (2048 - f) * 4; }
// WAVE PERIOD - (2048 - F) * 2 T-CYCLES
static u16 wave_period(u16 f)   { return (2048 - f) * 2; }

static void trigger_ch1(void) {
    apu.ch1_enabled = true;
    if (apu.ch1_length == 0) apu.ch1_length = 64;
    apu.ch1_freq_timer = square_period(ch1_frequency());
    apu.ch1_env_timer  = apu.ch1_env_period;
    apu.ch1_volume     = (apu.nr12 >> 4) & 0x0F;
    apu.ch1_env_up     = (apu.nr12 & 0x08) != 0;

    // SWEEP
    apu.ch1_shadow_freq  = ch1_frequency();
    apu.ch1_sweep_period = (apu.nr10 >> 4) & 0x07;
    apu.ch1_sweep_negate = (apu.nr10 & 0x08) != 0;
    apu.ch1_sweep_shift  = apu.nr10 & 0x07;
    apu.ch1_sweep_timer  = apu.ch1_sweep_period ? apu.ch1_sweep_period : 8;
    apu.ch1_sweep_enable = (apu.ch1_sweep_period != 0) || (apu.ch1_sweep_shift != 0);

    // DAC OFF (UPPER 5 BITS OF NR12 ZERO) DISABLES CHANNEL
    if ((apu.nr12 & 0xF8) == 0) apu.ch1_enabled = false;
}

static void trigger_ch2(void) {
    apu.ch2_enabled = true;
    if (apu.ch2_length == 0) apu.ch2_length = 64;
    apu.ch2_freq_timer = square_period(ch2_frequency());
    apu.ch2_env_timer  = apu.ch2_env_period;
    apu.ch2_volume     = (apu.nr22 >> 4) & 0x0F;
    apu.ch2_env_up     = (apu.nr22 & 0x08) != 0;
    if ((apu.nr22 & 0xF8) == 0) apu.ch2_enabled = false;
}

static void trigger_ch3(void) {
    apu.ch3_enabled = true;
    if (apu.ch3_length == 0) apu.ch3_length = 256;
    apu.ch3_freq_timer = wave_period(ch3_frequency());
    apu.ch3_sample_pos = 0;
    if (!(apu.nr30 & 0x80)) apu.ch3_enabled = false;  // DAC OFF
}

static void trigger_ch4(void) {
    apu.ch4_enabled = true;
    if (apu.ch4_length == 0) apu.ch4_length = 64;
    apu.ch4_lfsr = 0x7FFF;
    apu.ch4_env_timer = apu.ch4_env_period;
    apu.ch4_volume    = (apu.nr42 >> 4) & 0x0F;
    apu.ch4_env_up    = (apu.nr42 & 0x08) != 0;
    if ((apu.nr42 & 0xF8) == 0) apu.ch4_enabled = false;
}

static void clock_length(void) {
    if (apu.ch1_length_enable && apu.ch1_length > 0) {
        if (--apu.ch1_length == 0) apu.ch1_enabled = false;
    }
    if (apu.ch2_length_enable && apu.ch2_length > 0) {
        if (--apu.ch2_length == 0) apu.ch2_enabled = false;
    }
    if (apu.ch3_length_enable && apu.ch3_length > 0) {
        if (--apu.ch3_length == 0) apu.ch3_enabled = false;
    }
    if (apu.ch4_length_enable && apu.ch4_length > 0) {
        if (--apu.ch4_length == 0) apu.ch4_enabled = false;
    }
}

static u16 sweep_calc(void) {
    u16 next = apu.ch1_shadow_freq >> apu.ch1_sweep_shift;
    if (apu.ch1_sweep_negate) {
        next = apu.ch1_shadow_freq - next;
    } else {
        next = apu.ch1_shadow_freq + next;
    }
    if (next > 2047) {
        apu.ch1_enabled = false;
        apu.ch1_sweep_enable = false;
    }
    return next;
}

static void clock_sweep(void) {
    if (!apu.ch1_sweep_enable) return;
    if (apu.ch1_sweep_timer > 0) apu.ch1_sweep_timer--;
    if (apu.ch1_sweep_timer == 0) {
        apu.ch1_sweep_timer = apu.ch1_sweep_period ? apu.ch1_sweep_period : 8;
        if (apu.ch1_sweep_period != 0) {
            u16 next = sweep_calc();
            if (next <= 2047 && apu.ch1_sweep_shift != 0) {
                apu.ch1_shadow_freq = next;
                ch1_set_frequency(next);
                sweep_calc();  // OVERFLOW CHECK AGAIN
            }
        }
    }
}

static void clock_envelope_for(u8* timer, u8 period, u8* volume, bool up) {
    if (period == 0) return;
    if (*timer > 0) (*timer)--;
    if (*timer == 0) {
        *timer = period;
        if (up && *volume < 15) (*volume)++;
        else if (!up && *volume > 0) (*volume)--;
    }
}

static void clock_envelopes(void) {
    clock_envelope_for(&apu.ch1_env_timer, apu.ch1_env_period, &apu.ch1_volume, apu.ch1_env_up);
    clock_envelope_for(&apu.ch2_env_timer, apu.ch2_env_period, &apu.ch2_volume, apu.ch2_env_up);
    clock_envelope_for(&apu.ch4_env_timer, apu.ch4_env_period, &apu.ch4_volume, apu.ch4_env_up);
}

static void frame_sequencer_step(void) {
    // STANDARD GB FRAME SEQUENCER PATTERN:
    //   STEP 0: LENGTH
    //   STEP 2: LENGTH + SWEEP
    //   STEP 4: LENGTH
    //   STEP 6: LENGTH + SWEEP
    //   STEP 7: ENVELOPE
    switch (apu.frame_seq_step) {
        case 0: clock_length(); break;
        case 2: clock_length(); clock_sweep(); break;
        case 4: clock_length(); break;
        case 6: clock_length(); clock_sweep(); break;
        case 7: clock_envelopes(); break;
        default: break;
    }
    apu.frame_seq_step = (apu.frame_seq_step + 1) & 0x07;
}

static void step_channels_t(void) {
    // CH1
    if (apu.ch1_freq_timer > 0) apu.ch1_freq_timer--;
    if (apu.ch1_freq_timer == 0) {
        apu.ch1_freq_timer = square_period(ch1_frequency());
        apu.ch1_duty_pos = (apu.ch1_duty_pos + 1) & 0x07;
    }
    // CH2
    if (apu.ch2_freq_timer > 0) apu.ch2_freq_timer--;
    if (apu.ch2_freq_timer == 0) {
        apu.ch2_freq_timer = square_period(ch2_frequency());
        apu.ch2_duty_pos = (apu.ch2_duty_pos + 1) & 0x07;
    }
    // CH3
    if (apu.ch3_freq_timer > 0) apu.ch3_freq_timer--;
    if (apu.ch3_freq_timer == 0) {
        apu.ch3_freq_timer = wave_period(ch3_frequency());
        apu.ch3_sample_pos = (apu.ch3_sample_pos + 1) & 0x1F;
    }
    // CH4
    if (apu.ch4_freq_timer > 0) apu.ch4_freq_timer--;
    if (apu.ch4_freq_timer == 0) {
        u16 divisor = NOISE_DIVISORS[apu.nr43 & 0x07];
        u8 shift = (apu.nr43 >> 4) & 0x0F;
        apu.ch4_freq_timer = divisor << shift;

        // LFSR STEP: NEW BIT = OLD BIT0 XOR OLD BIT1
        u16 b = (apu.ch4_lfsr ^ (apu.ch4_lfsr >> 1)) & 1;
        apu.ch4_lfsr = (apu.ch4_lfsr >> 1) | (b << 14);
        if (apu.nr43 & 0x08) {
            apu.ch4_lfsr = (apu.ch4_lfsr & ~(1 << 6)) | (b << 6);
        }
    }
}

static u8 ch1_sample(void) {
    if (!apu.ch1_enabled) return 0;
    u8 pattern = DUTY_PATTERNS[(apu.nr11 >> 6) & 0x03];
    u8 bit = (pattern >> apu.ch1_duty_pos) & 1;
    return bit ? apu.ch1_volume : 0;
}

static u8 ch2_sample(void) {
    if (!apu.ch2_enabled) return 0;
    u8 pattern = DUTY_PATTERNS[(apu.nr21 >> 6) & 0x03];
    u8 bit = (pattern >> apu.ch2_duty_pos) & 1;
    return bit ? apu.ch2_volume : 0;
}

static u8 ch3_sample(void) {
    if (!apu.ch3_enabled || !(apu.nr30 & 0x80)) return 0;
    u8 byte = apu.wave_ram[apu.ch3_sample_pos >> 1];
    u8 nibble = (apu.ch3_sample_pos & 1) ? (byte & 0x0F) : (byte >> 4);
    u8 shift_code = (apu.nr32 >> 5) & 0x03;
    // CODE: 0=MUTE, 1=100%, 2=50%, 3=25%
    switch (shift_code) {
        case 0: return 0;
        case 1: return nibble;
        case 2: return nibble >> 1;
        case 3: return nibble >> 2;
    }
    return 0;
}

static u8 ch4_sample(void) {
    if (!apu.ch4_enabled) return 0;
    // CHANNEL EMITS WHEN BIT0 OF LFSR IS 0
    if ((apu.ch4_lfsr & 1) == 0) return apu.ch4_volume;
    return 0;
}

// ONE-POLE DC-BLOCKING HPF STATE. ALPHA ~ 1 - 2*PI*FC/FS.
// AT FS=48000 AND FC=78 HZ (DMG ANALOG STAGE), ALPHA ~ 0.9898.
// USING FLOATS HERE; AT ~48K SAMPLES/SEC THIS IS NEGLIGIBLE.
static float hpf_prev_in_l = 0.0f, hpf_prev_out_l = 0.0f;
static float hpf_prev_in_r = 0.0f, hpf_prev_out_r = 0.0f;
static const float HPF_ALPHA = 0.9898f;

static i16 hpf_step(float in, float* prev_in, float* prev_out) {
    float out = HPF_ALPHA * (*prev_out + in - *prev_in);
    *prev_in  = in;
    *prev_out = out;
    if (out > 32767.0f)  out = 32767.0f;
    if (out < -32768.0f) out = -32768.0f;
    return (i16)out;
}

static void emit_sample(void) {
    // GATHER 4-BIT CHANNEL OUTPUTS (0..15). CHANNELS THAT ARE INACTIVE
    // EMIT 0; ACTIVE ONES EMIT 0..15.
    u8 s1 = ch1_sample();
    u8 s2 = ch2_sample();
    u8 s3 = ch3_sample();
    u8 s4 = ch4_sample();

    // ROUTE TO L/R PER NR51
    u8 nr51 = apu.nr51;
    i32 left  = 0, right = 0;
    if (nr51 & 0x10) left  += s1;
    if (nr51 & 0x20) left  += s2;
    if (nr51 & 0x40) left  += s3;
    if (nr51 & 0x80) left  += s4;
    if (nr51 & 0x01) right += s1;
    if (nr51 & 0x02) right += s2;
    if (nr51 & 0x04) right += s3;
    if (nr51 & 0x08) right += s4;

    // MASTER VOLS (NR50 3-BIT, HARDWARE = VALUE+1, RANGE 1..8)
    u8 lvol = ((apu.nr50 >> 4) & 0x07) + 1;
    u8 rvol = (apu.nr50 & 0x07) + 1;
    left  *= lvol;
    right *= rvol;

    // SCALE TO 16-BIT RANGE. PEAK = 4 CHANNELS * 15 * 8 MASTER = 480;
    // *60 -> ~28800 PEAK, LEAVES SOME HEADROOM.
    float l_in = (float)(left  * 60);
    float r_in = (float)(right * 60);

    // DC BLOCK
    i16 l = hpf_step(l_in, &hpf_prev_in_l, &hpf_prev_out_l);
    i16 r = hpf_step(r_in, &hpf_prev_in_r, &hpf_prev_out_r);

    sample_buf[sample_buf_pos++] = l;
    sample_buf[sample_buf_pos++] = r;

    if (sample_buf_pos >= BATCH_SAMPLES * 2) {
        if (audio_dev != 0) {
            Uint32 queued = SDL_GetQueuedAudioSize(audio_dev);
            if (queued < AUDIO_QUEUE_MAX_BYTES) {
                SDL_QueueAudio(audio_dev, sample_buf, sample_buf_pos * sizeof(i16));
            }
        }
        sample_buf_pos = 0;
    }
}

void apu_init(void) {
    memset(&apu, 0, sizeof(apu));
    apu.power = true;
    apu.nr52 = 0xF1;     // POWER ON, ALL CHANNELS OFF
    apu.nr50 = 0x77;     // FULL MASTER VOL ON BOTH SIDES (POST-BOOT VALUE)
    apu.nr51 = 0xF3;     // POST-BOOT ROUTING
    apu.frame_seq_counter = FRAME_SEQ_PERIOD_T;
    apu.sample_acc = 0;
    apu.ch4_lfsr = 0x7FFF;

    SDL_AudioSpec want = {0}, have = {0};
    want.freq = APU_SAMPLE_RATE;
    want.format = AUDIO_S16SYS;
    want.channels = 2;
    want.samples = 1024;
    want.callback = NULL;

    audio_dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    if (audio_dev == 0) {
        LOG_WARN(LOG_MAIN, "APU: SDL_OpenAudioDevice failed: %s", SDL_GetError());
        return;
    }
    SDL_PauseAudioDevice(audio_dev, 0);  // UNPAUSE
    LOG_INFO(LOG_MAIN, "APU: audio device opened @ %d Hz, %d channels", have.freq, have.channels);
}

void apu_cleanup(void) {
    if (audio_dev != 0) {
        SDL_CloseAudioDevice(audio_dev);
        audio_dev = 0;
    }
}

void apu_tick(void) {
    // CALLED PER M-CYCLE = 4 T-CYCLES
    for (int t = 0; t < 4; t++) {
        if (apu.power) {
            step_channels_t();
        }

        // FRAME SEQUENCER
        if (apu.frame_seq_counter > 0) apu.frame_seq_counter--;
        if (apu.frame_seq_counter == 0) {
            apu.frame_seq_counter = FRAME_SEQ_PERIOD_T;
            if (apu.power) frame_sequencer_step();
        }

        // SAMPLE OUTPUT
        apu.sample_acc++;
        if (apu.sample_acc >= SAMPLE_PERIOD_T) {
            apu.sample_acc = 0;
            emit_sample();
        }
    }
}

// REGISTER ACCESS
u8 apu_read(u16 addr) {
    // WAVE RAM IS READABLE EVEN WHEN POWERED OFF
    if (addr >= WAVE_RAM_START && addr <= WAVE_RAM_START + 0xF) {
        return apu.wave_ram[addr - WAVE_RAM_START];
    }

    switch (addr) {
        // OR-MASKS BELOW REFLECT WHICH BITS READ AS 1 ON REAL HARDWARE
        case NR10_REG: return apu.nr10 | 0x80;
        case NR11_REG: return apu.nr11 | 0x3F;
        case NR12_REG: return apu.nr12;
        case NR13_REG: return 0xFF;
        case NR14_REG: return apu.nr14 | 0xBF;

        case NR21_REG: return apu.nr21 | 0x3F;
        case NR22_REG: return apu.nr22;
        case NR23_REG: return 0xFF;
        case NR24_REG: return apu.nr24 | 0xBF;

        case NR30_REG: return apu.nr30 | 0x7F;
        case NR31_REG: return 0xFF;
        case NR32_REG: return apu.nr32 | 0x9F;
        case NR33_REG: return 0xFF;
        case NR34_REG: return apu.nr34 | 0xBF;

        case NR41_REG: return 0xFF;
        case NR42_REG: return apu.nr42;
        case NR43_REG: return apu.nr43;
        case NR44_REG: return apu.nr44 | 0xBF;

        case NR50_REG: return apu.nr50;
        case NR51_REG: return apu.nr51;
        case NR52_REG: {
            u8 v = 0x70;
            if (apu.power) v |= 0x80;
            if (apu.ch1_enabled) v |= 0x01;
            if (apu.ch2_enabled) v |= 0x02;
            if (apu.ch3_enabled) v |= 0x04;
            if (apu.ch4_enabled) v |= 0x08;
            return v;
        }
    }
    return 0xFF;
}

void apu_write(u16 addr, u8 val) {
    // WAVE RAM IS ALWAYS WRITABLE
    if (addr >= WAVE_RAM_START && addr <= WAVE_RAM_START + 0xF) {
        apu.wave_ram[addr - WAVE_RAM_START] = val;
        return;
    }

    // NR52 POWER CONTROL IS ALWAYS WRITABLE
    if (addr == NR52_REG) {
        bool new_power = (val & 0x80) != 0;
        if (apu.power && !new_power) {
            // POWER-OFF CLEARS ALL REGISTERS (EXCEPT WAVE RAM)
            u8 saved_wave[16];
            memcpy(saved_wave, apu.wave_ram, sizeof(saved_wave));
            memset(&apu, 0, sizeof(apu));
            memcpy(apu.wave_ram, saved_wave, sizeof(saved_wave));
            // RESET HPF SO IT DOESN'T CARRY A STEP TRANSIENT
            hpf_prev_in_l = hpf_prev_out_l = 0.0f;
            hpf_prev_in_r = hpf_prev_out_r = 0.0f;
        }
        apu.power = new_power;
        return;
    }

    // ALL OTHER WRITES IGNORED WHILE POWERED OFF
    if (!apu.power) return;

    switch (addr) {
        // CH1
        case NR10_REG: apu.nr10 = val; break;
        case NR11_REG:
            apu.nr11 = val;
            apu.ch1_length = 64 - (val & 0x3F);
            break;
        case NR12_REG:
            apu.nr12 = val;
            apu.ch1_env_period = val & 0x07;
            if ((val & 0xF8) == 0) apu.ch1_enabled = false;  // DAC OFF
            break;
        case NR13_REG: apu.nr13 = val; break;
        case NR14_REG:
            apu.nr14 = val;
            apu.ch1_length_enable = (val & 0x40) != 0;
            if (val & 0x80) trigger_ch1();
            break;

        // CH2
        case NR21_REG:
            apu.nr21 = val;
            apu.ch2_length = 64 - (val & 0x3F);
            break;
        case NR22_REG:
            apu.nr22 = val;
            apu.ch2_env_period = val & 0x07;
            if ((val & 0xF8) == 0) apu.ch2_enabled = false;
            break;
        case NR23_REG: apu.nr23 = val; break;
        case NR24_REG:
            apu.nr24 = val;
            apu.ch2_length_enable = (val & 0x40) != 0;
            if (val & 0x80) trigger_ch2();
            break;

        // CH3
        case NR30_REG:
            apu.nr30 = val;
            if (!(val & 0x80)) apu.ch3_enabled = false;
            break;
        case NR31_REG:
            apu.nr31 = val;
            apu.ch3_length = 256 - val;
            break;
        case NR32_REG: apu.nr32 = val; break;
        case NR33_REG: apu.nr33 = val; break;
        case NR34_REG:
            apu.nr34 = val;
            apu.ch3_length_enable = (val & 0x40) != 0;
            if (val & 0x80) trigger_ch3();
            break;

        // CH4
        case NR41_REG:
            apu.nr41 = val;
            apu.ch4_length = 64 - (val & 0x3F);
            break;
        case NR42_REG:
            apu.nr42 = val;
            apu.ch4_env_period = val & 0x07;
            if ((val & 0xF8) == 0) apu.ch4_enabled = false;
            break;
        case NR43_REG: apu.nr43 = val; break;
        case NR44_REG:
            apu.nr44 = val;
            apu.ch4_length_enable = (val & 0x40) != 0;
            if (val & 0x80) trigger_ch4();
            break;

        // CONTROL
        case NR50_REG: apu.nr50 = val; break;
        case NR51_REG: apu.nr51 = val; break;
    }
}
