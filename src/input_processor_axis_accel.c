// SPDX-License-Identifier: GPL-2.0-or-later
// copyright (C) 2026 knttnk

// ポインタの速度を、トラックボールの速度の5/4乗に比例させる入力プロセッサ。
// knttnk/keyballのkeyball_on_apply_motion_to_mouse_moveと同じ式を使う。
//
// PAW3222のドライバは、ボールが動いている間15msごとに移動量を報告する。
// そのため、1回の報告の移動量vはボールの速度に比例する。
// このプロセッサは、XとYの移動量を軸ごとに v × |v|^(1/4) × scale に書き換える。
// scaleは、x-scale-permilleとy-scale-permilleに千分率で指定する。

#define DT_DRV_COMPAT zmk_input_processor_axis_accel

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <drivers/input_processor.h>

// 左手側のように、デバイスツリーにこのプロセッサのノードがないビルドでは以下をコンパイルしない。
#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

// 倍率は小数部12ビットの固定小数点で表す。GAIN_ONEが倍率1.0にあたる。
#define GAIN_FRAC_BITS 12
#define GAIN_ONE (1 << GAIN_FRAC_BITS)
// 48ビット左シフトしてもuint64_tに収まるよう、倍率の計算に使う移動量をこの値までに制限する。
#define VALUE_LIMIT 32767

struct axis_accel_config {
    // 移動量が1のときの倍率。千分率で表す。
    uint32_t x_scale_permille;
    uint32_t y_scale_permille;
};

// nの平方根の整数部分を返す。浮動小数点を使わず、上位の桁から2進数で1桁ずつ決める。
static uint32_t isqrt64(uint64_t n) {
    uint64_t res = 0;
    uint64_t bit = 1ULL << 62;

    while (bit > n) {
        bit >>= 2;
    }

    while (bit != 0) {
        if (n >= res + bit) {
            n -= res + bit;
            res = (res >> 1) + bit;
        } else {
            res >>= 1;
        }
        bit >>= 2;
    }

    return (uint32_t)res;
}

// 倍率 |value|^(1/4) × scale を求め、小数部12ビットの固定小数点で返す。
static uint32_t gain_for(int32_t value, uint32_t scale_permille) {
    uint64_t mag = value < 0 ? -(int64_t)value : value;
    mag = MIN(mag, VALUE_LIMIT);

    // 小数部を48ビットにしてから平方根を2回取ると、4乗根の小数部は12ビットになる。
    uint64_t root4 = isqrt64(isqrt64(mag << 48));

    return root4 * scale_permille / 1000;
}

// 入力リスナーがイベントを1件受け取るたびに呼び出す。XとYの移動量に倍率を掛けて書き換える。
static int axis_accel_handle_event(const struct device *dev, struct input_event *event,
                                   uint32_t param1, uint32_t param2,
                                   struct zmk_input_processor_state *state) {
    const struct axis_accel_config *cfg = dev->config;

    if (event->type != INPUT_EV_REL) {
        return ZMK_INPUT_PROC_CONTINUE;
    }

    uint32_t gain;
    switch (event->code) {
    case INPUT_REL_X:
        gain = gain_for(event->value, cfg->x_scale_permille);
        break;
    case INPUT_REL_Y:
        gain = gain_for(event->value, cfg->y_scale_permille);
        break;
    default:
        return ZMK_INPUT_PROC_CONTINUE;
    }

    // 前回までに切り捨てた端数を足してから整数にし、新しい端数は次回に持ち越す。
    // 端数を持ち越すので、倍率が1未満になる低速でも移動量を失わない。
    // state->remainderは、ノードにtrack-remaindersを指定したとき入力リスナーが用意する。
    int64_t scaled = (int64_t)event->value * gain;
    if (state && state->remainder) {
        scaled += *state->remainder;
    }

    int32_t out = scaled / GAIN_ONE;
    if (state && state->remainder) {
        *state->remainder = scaled - (int64_t)out * GAIN_ONE;
    }

    LOG_DBG("accelerated %d to %d with gain %u/%d", event->value, out, gain, GAIN_ONE);

    event->value = out;

    return ZMK_INPUT_PROC_CONTINUE;
}

static struct zmk_input_processor_driver_api axis_accel_driver_api = {
    .handle_event = axis_accel_handle_event,
};

// デバイスツリーのノード1つにつき、設定を1組ずつ定義する。
#define AXIS_ACCEL_INST(n)                                                                         \
    static const struct axis_accel_config axis_accel_config_##n = {                                \
        .x_scale_permille = DT_INST_PROP(n, x_scale_permille),                                     \
        .y_scale_permille = DT_INST_PROP(n, y_scale_permille),                                     \
    };                                                                                             \
    DEVICE_DT_INST_DEFINE(n, NULL, NULL, NULL, &axis_accel_config_##n, POST_KERNEL,                \
                          CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &axis_accel_driver_api);

DT_INST_FOREACH_STATUS_OKAY(AXIS_ACCEL_INST)

#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */
