/*
 * This work is licensed under the Creative Commons CC0 1.0 Universal (CC0 1.0)
 * Public Domain Dedication. To view a copy of this license, visit
 * https://creativecommons.org/publicdomain/zero/1.0/.
 *
 * The person who associated a work with this deed has dedicated the work to the public
 * domain by waiving all of his or her rights to the work worldwide under copyright law,
 * including all related and neighboring rights, to the extent allowed by law.
 *
 * You can copy, modify, distribute, and perform the work, even for commercial purposes,
 * all without asking permission.
 */

/*
 * Samples made using TTS Maker (voice 178 - Chloe) and sped up by 1.05x
 */

#pragma once

#include <cstdint>
#include <cstddef>
#include "common/dsp/sampling/SamplePlayer.hpp"

namespace kastle2
{

namespace version
{

static constexpr size_t one_length = 6594;
static constexpr size_t eight_length = 5679;
static constexpr size_t five_length = 7306;
static constexpr size_t beta_version_length = 10497;
static constexpr size_t four_length = 6311;
static constexpr size_t nine_length = 7653;
static constexpr size_t version_length = 7084;
static constexpr size_t three_length = 5424;
static constexpr size_t point_length = 6630;
static constexpr size_t seven_length = 7859;
static constexpr size_t kastle2_length = 11902;
static constexpr size_t two_length = 6388;
static constexpr size_t six_length = 8114;
static constexpr size_t zero_length = 8414;
static constexpr size_t ten_length = 6898;
static constexpr size_t test_mode_length = 9436;
static constexpr size_t test_success_length = 11587;
static constexpr size_t citadel_length = 9873;
static constexpr size_t release_candidate_length = 13188;
static constexpr size_t rc_length = 8467;
static constexpr size_t calibrated_length = 7308;

extern const int16_t one_data[];
extern const int16_t eight_data[];
extern const int16_t five_data[];
extern const int16_t beta_version_data[];
extern const int16_t four_data[];
extern const int16_t nine_data[];
extern const int16_t version_data[];
extern const int16_t three_data[];
extern const int16_t point_data[];
extern const int16_t seven_data[];
extern const int16_t kastle2_data[];
extern const int16_t two_data[];
extern const int16_t six_data[];
extern const int16_t zero_data[];
extern const int16_t ten_data[];
extern const int16_t test_mode_data[];
extern const int16_t test_success_data[];
extern const int16_t citadel_data[];
extern const int16_t release_candidate_data[];
extern const int16_t rc_data[];
extern const int16_t calibrated_data[];

 constexpr SamplePlayer16bit::Sample one = {.data = one_data, .length = one_length};
 constexpr SamplePlayer16bit::Sample eight = {.data = eight_data, .length = eight_length};
 constexpr SamplePlayer16bit::Sample five = {.data = five_data, .length = five_length};
 constexpr SamplePlayer16bit::Sample beta_version = {.data = beta_version_data, .length = beta_version_length};
 constexpr SamplePlayer16bit::Sample four = {.data = four_data, .length = four_length};
 constexpr SamplePlayer16bit::Sample nine = {.data = nine_data, .length = nine_length};
 constexpr SamplePlayer16bit::Sample version = {.data = version_data, .length = version_length};
 constexpr SamplePlayer16bit::Sample three = {.data = three_data, .length = three_length};
 constexpr SamplePlayer16bit::Sample point = {.data = point_data, .length = point_length};
 constexpr SamplePlayer16bit::Sample seven = {.data = seven_data, .length = seven_length};
 constexpr SamplePlayer16bit::Sample kastle2 = {.data = kastle2_data, .length = kastle2_length};
 constexpr SamplePlayer16bit::Sample two = {.data = two_data, .length = two_length};
 constexpr SamplePlayer16bit::Sample six = {.data = six_data, .length = six_length};
 constexpr SamplePlayer16bit::Sample zero = {.data = zero_data, .length = zero_length};
 constexpr SamplePlayer16bit::Sample ten = {.data = ten_data, .length = ten_length};
 constexpr SamplePlayer16bit::Sample test_mode = {.data = test_mode_data, .length = test_mode_length};
 constexpr SamplePlayer16bit::Sample test_success = {.data = test_success_data, .length = test_success_length};
 constexpr SamplePlayer16bit::Sample citadel = {.data = citadel_data, .length = citadel_length};
 constexpr SamplePlayer16bit::Sample release_candidate = {.data = release_candidate_data, .length = release_candidate_length};
 constexpr SamplePlayer16bit::Sample rc = {.data = rc_data, .length = rc_length};
 constexpr SamplePlayer16bit::Sample calibrated = {.data = calibrated_data, .length = calibrated_length};
 constexpr SamplePlayer16bit::Sample empty = {.data = {}, .length = 0};

}

}
