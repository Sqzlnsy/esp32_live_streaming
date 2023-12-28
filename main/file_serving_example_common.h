/*
 * SPDX-FileCopyrightText: 2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
/* HTTP File Server Example, common declarations

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#pragma once

#include "sdkconfig.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t example_mount_storage(const char *base_path);

void https_server_task(void *pvParameters);

esp_err_t start_stream(void);

void video_stream_task(void *param);

void data_link_task(void *pvParameters);

void monitor_CPU(void);

#ifdef __cplusplus
}
#endif
