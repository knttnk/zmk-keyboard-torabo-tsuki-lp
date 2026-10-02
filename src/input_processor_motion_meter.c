// SPDX-License-Identifier: GPL-2.0-or-later
// copyright (C) 2026 knttnk

// 通過した移動量を数える入力プロセッサ。値は書き換えない。
// スクロール用の処理の先頭に置き、親指キーを押している間にボールを回したかを調べるために使う。
// 数えた量は、いずれかのレイヤーが有効になるたびに0へ戻す。
// 親指キーは押した直後にレイヤーを有効にするので、押してからの量を数えることになる。

#define DT_DRV_COMPAT zmk_input_processor_motion_meter

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/atomic.h>
#include <drivers/input_processor.h>

#include <zmk/event_manager.h>
#include <zmk/events/layer_state_changed.h>

#include "ball_motion.h"

// 入力の処理とキーの処理は別のスレッドで動くので、アトミック変数に入れる。
static atomic_t motion = ATOMIC_INIT(0);

uint32_t ball_motion_since_layer_activation(void) { return (uint32_t)atomic_get(&motion); }

static int motion_meter_layer_listener(const zmk_event_t *eh) {
    const struct zmk_layer_state_changed *ev = as_zmk_layer_state_changed(eh);

    if (ev != NULL && ev->state) {
        atomic_set(&motion, 0);
    }

    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(motion_meter, motion_meter_layer_listener);
ZMK_SUBSCRIPTION(motion_meter, zmk_layer_state_changed);

// 入力リスナーがイベントを1件受け取るたびに呼び出す。
static int motion_meter_handle_event(const struct device *dev, struct input_event *event,
                                     uint32_t param1, uint32_t param2,
                                     struct zmk_input_processor_state *state) {
    if (event->type == INPUT_EV_REL) {
        atomic_add(&motion, event->value < 0 ? -event->value : event->value);
    }

    return ZMK_INPUT_PROC_CONTINUE;
}

static struct zmk_input_processor_driver_api motion_meter_driver_api = {
    .handle_event = motion_meter_handle_event,
};

#define MOTION_METER_INST(n)                                                                       \
    DEVICE_DT_INST_DEFINE(n, NULL, NULL, NULL, NULL, POST_KERNEL,                                  \
                          CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &motion_meter_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MOTION_METER_INST)
