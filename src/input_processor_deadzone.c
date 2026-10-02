// SPDX-License-Identifier: GPL-2.0-or-later
// copyright (C) 2026 knttnk

// 指定した種類の入力のうち、大きさが閾値に満たない値を0に書き換える入力プロセッサ。
// 縦スクロール中に混ざる小さな横スクロールを取り除くために使う。
// 閾値以上の値は書き換えずにそのまま通す。

#define DT_DRV_COMPAT zmk_input_processor_deadzone

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <drivers/input_processor.h>

// 左手側のように、デバイスツリーにこのプロセッサのノードがないビルドでは以下をコンパイルしない。
#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

struct deadzone_config {
    uint8_t type;
    int32_t threshold;
    size_t codes_len;
    const uint16_t *codes;
};

// 入力リスナーがイベントを1件受け取るたびに呼び出す。
static int deadzone_handle_event(const struct device *dev, struct input_event *event,
                                 uint32_t param1, uint32_t param2,
                                 struct zmk_input_processor_state *state) {
    const struct deadzone_config *cfg = dev->config;

    if (event->type != cfg->type) {
        return ZMK_INPUT_PROC_CONTINUE;
    }

    for (size_t i = 0; i < cfg->codes_len; i++) {
        if (cfg->codes[i] != event->code) {
            continue;
        }

        // イベントを破棄すると報告の区切りを示すsyncも失われるので、値だけを0にして後続へ渡す。
        if (event->value > -cfg->threshold && event->value < cfg->threshold) {
            event->value = 0;
        }
        break;
    }

    return ZMK_INPUT_PROC_CONTINUE;
}

static struct zmk_input_processor_driver_api deadzone_driver_api = {
    .handle_event = deadzone_handle_event,
};

// デバイスツリーのノード1つにつき、設定を1組ずつ定義する。
#define DEADZONE_INST(n)                                                                           \
    static const uint16_t deadzone_codes_##n[] = DT_INST_PROP(n, codes);                           \
    static const struct deadzone_config deadzone_config_##n = {                                    \
        .type = DT_INST_PROP_OR(n, type, INPUT_EV_REL),                                            \
        .threshold = DT_INST_PROP(n, threshold),                                                   \
        .codes_len = DT_INST_PROP_LEN(n, codes),                                                   \
        .codes = deadzone_codes_##n,                                                               \
    };                                                                                             \
    DEVICE_DT_INST_DEFINE(n, NULL, NULL, NULL, &deadzone_config_##n, POST_KERNEL,                  \
                          CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &deadzone_driver_api);

DT_INST_FOREACH_STATUS_OKAY(DEADZONE_INST)

#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */
