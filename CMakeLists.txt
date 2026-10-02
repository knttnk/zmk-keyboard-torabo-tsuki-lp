if(CONFIG_SHIELD_TORABO_TSUKI_LP_LEFT OR CONFIG_SHIELD_TORABO_TSUKI_LP_RIGHT)
  zephyr_library()
  zephyr_include_directories(${APPLICATION_SOURCE_DIR}/include)
  zephyr_library_sources(src/board.c src/mini_trackpad_init_reg.c)
  zephyr_library_sources(src/input_processor_axis_accel.c src/input_processor_deadzone.c)
  # キーマップを処理する側（分割キーボードではセントラル）でだけ使うソース。
  if((NOT CONFIG_ZMK_SPLIT) OR CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
    zephyr_library_sources(
      src/behavior_mo_unlock.c
      src/behavior_kp_unless_moved.c
      src/input_processor_motion_meter.c
    )
  endif()
endif()
