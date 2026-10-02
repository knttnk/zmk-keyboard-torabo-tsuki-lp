// SPDX-License-Identifier: GPL-2.0-or-later
// copyright (C) 2026 knttnk

// キーを送るビヘイビア。ただし、スクロール中にボールを閾値以上動かしていたら何も送らない。
// 親指キーのホールドタップで、タップ側に使う。
// 無変換キーを押しながらスクロールしただけのときに、離しても無変換を送らないようにする。

#define DT_DRV_COMPAT zmk_behavior_kp_unless_moved

#include <zephyr/device.h>
#include <drivers/behavior.h>
#include <zephyr/logging/log.h>

#include <zmk/event_manager.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/behavior.h>

#include "ball_motion.h"

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

struct kp_unless_moved_config {
    uint32_t motion_threshold;
};

struct kp_unless_moved_data {
    // 押したときにキーを送らなかった場合、離したときにも送らないために覚えておく。
    bool suppressed;
};

// ZMK Studioがパラメータをキーとして表示するための情報。
#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)

static const struct behavior_parameter_value_metadata param_values[] = {
    {
        .display_name = "Key",
        .type = BEHAVIOR_PARAMETER_VALUE_TYPE_HID_USAGE,
    },
};

static const struct behavior_parameter_metadata_set param_metadata_set[] = {{
    .param1_values = param_values,
    .param1_values_len = ARRAY_SIZE(param_values),
}};

static const struct behavior_parameter_metadata metadata = {
    .sets_len = ARRAY_SIZE(param_metadata_set),
    .sets = param_metadata_set,
};

#endif

static int kp_unless_moved_binding_pressed(struct zmk_behavior_binding *binding,
                                           struct zmk_behavior_binding_event event) {
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    const struct kp_unless_moved_config *cfg = dev->config;
    struct kp_unless_moved_data *data = dev->data;

    uint32_t motion = ball_motion_since_layer_activation();
    data->suppressed = motion >= cfg->motion_threshold;

    LOG_DBG("position %d keycode 0x%02X motion %u suppressed %d", event.position, binding->param1,
            motion, data->suppressed);

    if (data->suppressed) {
        return ZMK_BEHAVIOR_OPAQUE;
    }

    return raise_zmk_keycode_state_changed_from_encoded(binding->param1, true, event.timestamp);
}

static int kp_unless_moved_binding_released(struct zmk_behavior_binding *binding,
                                            struct zmk_behavior_binding_event event) {
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    struct kp_unless_moved_data *data = dev->data;

    if (data->suppressed) {
        data->suppressed = false;
        return ZMK_BEHAVIOR_OPAQUE;
    }

    return raise_zmk_keycode_state_changed_from_encoded(binding->param1, false, event.timestamp);
}

static const struct behavior_driver_api kp_unless_moved_driver_api = {
    .binding_pressed = kp_unless_moved_binding_pressed,
    .binding_released = kp_unless_moved_binding_released,
#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
    .parameter_metadata = &metadata,
#endif // IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
};

// デバイスツリーのノード1つにつき、設定と状態を1組ずつ定義する。
#define KP_UNLESS_MOVED_INST(n)                                                                    \
    static struct kp_unless_moved_data kp_unless_moved_data_##n = {};                              \
    static const struct kp_unless_moved_config kp_unless_moved_config_##n = {                      \
        .motion_threshold = DT_INST_PROP(n, motion_threshold),                                     \
    };                                                                                             \
    BEHAVIOR_DT_INST_DEFINE(n, NULL, NULL, &kp_unless_moved_data_##n,                              \
                            &kp_unless_moved_config_##n, POST_KERNEL,                              \
                            CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &kp_unless_moved_driver_api);

DT_INST_FOREACH_STATUS_OKAY(KP_UNLESS_MOVED_INST)

#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */
