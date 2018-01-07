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

#if !defined(_DATA_H)
#define _DATA_H

struct genlist_item_data {
	struct tm saved_time;
	int alarm_id;
	char *widget_id;
	char *instance_id;
	Elm_Object_Item *item;
	Eina_Bool check_state;
};

#define APP_CONTROL_OPERATION_ALARM_ONTIME "http://tizen.org/appcontrol/operation/my_ontime_alarm"
#define APP_CONTROL_OPERATION_FROM_WIDGET "launch_request_from_widget"

/*
 * Initialize the data component
 */
void data_initialize(void);
void data_initialize_widget_data(void);

void data_initialize_widget_id_in_gendata(void *data);
/*
 * Finalize the data component
 */
void data_finalize(void);
void data_widget_data_finalize(void);

app_control_h data_get_app_control(void);
bundle *data_get_bundle(void);
bundle *data_get_widget_data_bundle(void);

void data_get_resource_path(const char *edj_file_in, char *file_path_out, int file_path_max);

char *data_get_title_text(void *data, Evas_Object *obj, const char *part);
char *data_get_saved_time_text(void *data, Evas_Object *obj, const char *part);

bundle *data_create_bundle(void);
void data_bundle_destroy(bundle *b);
void data_add_bundle_by_str(const char *bundle_key, const char *bundle_data);
void data_add_widget_data_bundle_by_str(const char *bundle_key, const char *bundle_data);
void data_set_widget_alarm_to_preference(const char *widget_id, const char *instance_id);
void data_set_widget_on_off_to_preference(struct genlist_item_data *gendata, char *on_off, char *alarm_id);
void data_delete_bundle(const char *key);
int data_get_bundle_count(void);

struct genlist_item_data *data_alarm_create_genlist_item_data(void);
void data_alarm_destroy_genlist_item_data(struct genlist_item_data *gendata);

char *data_get_popup_text(struct tm *saved_time);
void data_set_id_to_gendata(struct genlist_item_data *gendata, const char *widget_id, const char *instance_id);
Eina_Bool data_check_exist_widget_alarm(struct genlist_item_data *gendata);

void data_set_item_to_gendata(struct genlist_item_data *gendata, Elm_Object_Item *item);
Elm_Object_Item *data_get_item_from_gendata(struct genlist_item_data *gendata);

#endif
