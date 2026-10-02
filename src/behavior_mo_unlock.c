// SPDX-License-Identifier: GPL-2.0-or-later
// copyright (C) 2026 knttnk

// 押している間だけ指定したレイヤーを有効にするビヘイビア。
// 押したときに、unlock-layersに書いたレイヤーの固定も解除する。
// knttnk/keyballで、無変換キーやレイヤー2のキーを押すとレイヤー3の固定が外れる動作にあたる。
//
// 同じ動作はマクロでも書けるが、マクロは各ステップを後から順に実行する。
// ホールドタップのホールド側に使うと、レイヤーを無効にする前に次のキーが処理されることがある。
// このビヘイビアは呼び出された時点でレイヤーを切り替えるので、キーの処理と順番が入れ替わらない。

#define DT_DRV_COMPAT zmk_behavior_momentary_layer_unlock

#include <zephyr/device.h>
#include <drivers/behavior.h>
#include <zephyr/logging/log.h>

#include <zmk/keymap.h>
#include <zmk/behavior.h>

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

struct mo_unlock_config {
    size_t unlock_layers_len;
    const uint8_t *unlock_layers;
};

// ZMK Studioがパラメータをレイヤーとして表示するための情報。
#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)

static const struct behavior_parameter_value_metadata param_values[] = {
    {
        .display_name = "Layer",
        .type = BEHAVIOR_PARAMETER_VALUE_TYPE_LAYER_ID,
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

static int mo_unlock_binding_pressed(struct zmk_behavior_binding *binding,
                                     struct zmk_behavior_binding_event event) {
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    const struct mo_unlock_config *cfg = dev->config;

    LOG_DBG("position %d layer %d", event.position, binding->param1);

    for (size_t i = 0; i < cfg->unlock_layers_len; i++) {
        zmk_keymap_layer_deactivate(cfg->unlock_layers[i]);
    }

    return zmk_keymap_layer_activate(binding->param1);
}

static int mo_unlock_binding_released(struct zmk_behavior_binding *binding,
                                      struct zmk_behavior_binding_event event) {
    LOG_DBG("position %d layer %d", event.position, binding->param1);
    return zmk_keymap_layer_deactivate(binding->param1);
}

static const struct behavior_driver_api mo_unlock_driver_api = {
    .binding_pressed = mo_unlock_binding_pressed,
    .binding_released = mo_unlock_binding_released,
#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
    .parameter_metadata = &metadata,
#endif // IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
};

// デバイスツリーのノード1つにつき、設定を1組ずつ定義する。
#define MO_UNLOCK_INST(n)                                                                          \
    static const uint8_t mo_unlock_layers_##n[] = DT_INST_PROP(n, unlock_layers);                  \
    static const struct mo_unlock_config mo_unlock_config_##n = {                                  \
        .unlock_layers_len = DT_INST_PROP_LEN(n, unlock_layers),                                   \
        .unlock_layers = mo_unlock_layers_##n,                                                     \
    };                                                                                             \
    BEHAVIOR_DT_INST_DEFINE(n, NULL, NULL, NULL, &mo_unlock_config_##n, POST_KERNEL,               \
                            CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &mo_unlock_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MO_UNLOCK_INST)

#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */
