#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(central_uart)

# NORDIC SDK APP START
target_sources(app PRIVATE
  src/main.c
  src/system.c
  src/communications.c
  src/database.c
  src/spin3204_control.c
  src/commands.c
  src/commands_tasks.c
  src/lcd_spiModule.c
  src/remote.c
  src/adt_cbuffer.c
  src/encoding_checksum.cpp
)

target_include_directories(app
  PRIVATE
    ./inc
)
# NORDIC SDK APP END

zephyr_library_include_directories(${CMAKE_CURRENT_SOURCE_DIR})
