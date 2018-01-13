/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
 *
 * Licensed under the Flora License, Version 1.1 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://floralicense.org/license/
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#if !defined(_ALARM_H)
#define _ALARM_H

#if !defined(PACKAGE)
#define PACKAGE "net.mehm.gear-reality-check"
#endif

#ifdef  LOG_TAG
#undef  LOG_TAG
#endif
#define LOG_TAG "gear-reality-check"

#define BUF_LEN 1024

void alarm_destroy_widget(void *user_data);
void alarm_set_widget_on_off(char *on_off, void *user_data);

#endif
