// SPDX-License-Identifier: GPL-2.0-or-later
// copyright (C) 2026 knttnk

#pragma once

#include <stdint.h>

// いずれかのレイヤーが最後に有効になってから、スクロール中にボールが動いた量の合計を返す。
// 実装はsrc/input_processor_motion_meter.cにある。
uint32_t ball_motion_since_layer_activation(void);
