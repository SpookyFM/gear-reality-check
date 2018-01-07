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

#include <Elementary.h>
#include <efl_extension.h>
#include <dlog.h>
#include <bundle.h>
#include <app_preference.h>
#include <app.h>
#include <widget_service.h>
#include <widget_errno.h>

#include "gear-reality-check.h"
#include "data.h"
#include "view.h"

static struct data_info {
	app_control_h app_control;
	bundle *b;
	bundle *widget_data_b;
} s_info = {
	.app_control = NULL,
	.b = NULL,
	.widget_data_b = NULL,
};

#define HOURS_A_DAY 24
#define MINS_AN_HOUR 60
#define ALARM_SET_FOR "Alarm set for"
#define HOUR "hour"
#define MINUTE "minute"

static app_control_h _create_app_control(const char *operation, const char *app_id);
static void _destroy_app_control(app_control_h app_control);
static bundle *_decode_bundle(void);

/*
 * @brief Initialize data that is used in this application.
 */
void data_initialize(void)
{
	/*
	 * If you need to initialize managing data,
	 * please use this function.
	 */

	/*
	 * Create a app control to use alarm APIs.
	 */
	s_info.app_control = _create_app_control(APP_CONTROL_OPERATION_ALARM_ONTIME, PACKAGE);
	if (s_info.app_control == NULL) {
		dlog_print(DLOG_ERROR, LOG_TAG, "Failed to create a app control handle.");
		return;
	}

	/*
	 * Create a bundle so as to store alarm ID.
	 */
	s_info.b = _decode_bundle();
	if (s_info.b == NULL) {
		s_info.b = data_create_bundle();
		if (s_info.b == NULL) {
			dlog_print(DLOG_ERROR, LOG_TAG, "Failed to create a bundle.");
			return;
		}
	}
}

/*
 * @brief Initialize widget id in gendata that is used in this application.
 * @param[in] data Data structure that stores information of genlist, such as saved time, alarm ID, check state
 */
void data_initialize_widget_id_in_gendata(void *data)
{
	struct genlist_item_data *gendata = (struct genlist_item_data *) data;
	if (gendata == NULL) {
		dlog_print(DLOG_ERROR, LOG_TAG, "[%s:%d] gendata is NULL", __func__, __LINE__);
		return;
	}

	gendata->widget_id = NULL;
}

/*
 * @brief Initialize widget data that is used in this application.
 */
void data_initialize_widget_data(void)
{
	if (s_info.widget_data_b) {
		data_bundle_destroy(s_info.widget_data_b);
	}

	s_info.widget_data_b = bundle_create();
	if (s_info.widget_data_b == NULL) {
		dlog_print(DLOG_ERROR, LOG_TAG, "Failed to create a widget data bundle.");
		return;
	}
}

/*
 * @brief Destroys widget data that is used in this application.
 */
void data_widget_data_finalize(void)
{
	data_bundle_destroy(s_info.widget_data_b);
	s_info.widget_data_b = NULL;
}

/*
 * @brief Destroys data that is used in this application.
 */
void data_finalize(void)
{
	/*
	 * If you need to finalize managing data,
	 * please use this function.
	 */

	data_bundle_destroy(s_info.b);
	s_info.b = NULL;

	_destroy_app_control(s_info.app_control);
	s_info.app_control = NULL;
}

/*
 * @brief Gets app_control handle.
 */
app_control_h data_get_app_control(void)
{
	return s_info.app_control;
}

/*
 * @brief Gets bundle.
 */
bundle *data_get_bundle(void)
{
	return s_info.b;
}

/*
 * @brief Gets bundle will be used for widget data.
 */
bundle *data_get_widget_data_bundle(void)
{
	return s_info.widget_data_b;
}

/*
 * @brief Gets path of resource.
 * @param[in] file_in File name
 * @param[in] file_path_out The point to which save full path of the resource
 * @param[in] file_path_max Size of file name include path
 */
void data_get_resource_path(const char *file_in, char *file_path_out, int file_path_max)
{
	char *res_path = app_get_resource_path();
	if (res_path) {
		snprintf(file_path_out, file_path_max, "%s%s", res_path, file_in);
		free(res_path);
	}
}

/*
 * @brief This function will be operated when items are shown on the screen.
 * @param[in] data Data passed from 'elm_genlist_item_append' as third parameter
 * @param[in] obj Genlist
 * @param[in] part Name string of one of the existing text parts in the EDJ group implementing the item's theme
 */
char *data_get_title_text(void *data, Evas_Object *obj, const char *part)
{
	char buf[BUF_LEN] = { 0, };

	snprintf(buf, sizeof(buf), "%s", "Alarm");
	return strdup(buf);
}

/*
 * @brief This function will be operated when items are shown on the screen.
 * @param[in] data Data passed from 'elm_genlist_item_append' as third parameter
 * @param[in] obj Genlist
 * @param[in] part Name string of one of the existing text parts in the EDJ group implementing the item's theme
 */
char *data_get_saved_time_text(void *data, Evas_Object *obj, const char *part)
{
	struct genlist_item_data *gendata = data;
	struct tm *saved_time = NULL;
	char buf[BUF_LEN] = { 0, };

	if (!strcmp(part, "elm.text")) {
		saved_time = &gendata->saved_time;
		strftime(buf, sizeof(buf) - 1, "%l:%M %p", saved_time);
		return strdup(buf);
	}

	return NULL;
}

/*
 * @brief Creates a bundle.
 */
bundle *data_create_bundle(void)
{
	bundle *b = NULL;

	/*
	 * Create a bundle if the bundle not exists.
	 */
	b = bundle_create();

	return b;
}

/*
 * @brief Destroys the bundle.
 * @param[in] b Bundle that try to free
 */
void data_bundle_destroy(bundle *b)
{
	if (b == NULL) {
		dlog_print(DLOG_ERROR, LOG_TAG, "bundle is NULL.");
		return;
	}

	if (BUNDLE_ERROR_NONE != bundle_free(b)) {
		dlog_print(DLOG_ERROR, LOG_TAG, "failed at bundle_free(). Can't destroy bundle.");
	}
}

/*
 * @brief Adds a bundle with string type key-value
 * @param[in] bundle_key Key that uniquely identifies a bundle
 * @param[in] bundle_data Data that stores bundle as string type value
 */
void data_add_bundle_by_str(const char *bundle_key, const char *bundle_data)
{
	bundle_raw * r;
	int len;

	if (s_info.b == NULL) {
		dlog_print(DLOG_ERROR, LOG_TAG, "bundle is NULL.");
		return;
	}

	/*
	 * Add an alarm ID(int) that was converted into string type into a bundle.
	 */
	if (BUNDLE_ERROR_NONE != bundle_add_str(s_info.b, bundle_key, bundle_data)) {
		dlog_print(DLOG_ERROR, LOG_TAG, "Failed at bundle_add_str(). Can't add bundle by using string.");
	}

	/*
	 * And encodes a bundle to the bundle_raw format (uses base64 format).
	 */
	bundle_encode(s_info.b, &r, &len);

	/*
	 * Sets a value(string) & length(int) using preference API.
	 */
	if (PREFERENCE_ERROR_NONE != preference_set_string("DATA_KEY_BUNDLE_RAW", (const char *) r)) {
		dlog_print(DLOG_ERROR, LOG_TAG, "Failed at preference_set_string()");
	}

	if (PREFERENCE_ERROR_NONE != preference_set_int("DATA_KEY_BUNDLE_RAW_LEN", len)) {
		dlog_print(DLOG_ERROR, LOG_TAG, "Failed at preference_set_int()");
	}
}

/*
 * @brief Adds a bundle for widget with string type key-value
 * @param[in] bundle_key Key that uniquely identifies a bundle
 * @param[in] bundle_data Data that stores bundle as string type value
 */
void data_add_widget_data_bundle_by_str(const char *bundle_key, const char *bundle_data)
{
	if (s_info.widget_data_b == NULL) {
		dlog_print(DLOG_ERROR, LOG_TAG, "bundle is NULL.");
		return;
	}

	dlog_print(DLOG_INFO, LOG_TAG, "widget_bundle:%p, key(%s), data(%s)", s_info.widget_data_b, bundle_key, bundle_data);

	if (BUNDLE_ERROR_NONE != bundle_add_str(s_info.widget_data_b, bundle_key, bundle_data)) {
		dlog_print(DLOG_ERROR, LOG_TAG, "Failed at bundle_add_str(). Can't add bundle by using string.");
	}
}

/*
 * @brief Removes a key-value object with the given key
 * @param[in] key Key that uniquely identifies a bundle
 */
void data_delete_bundle(const char *key)
{
	bundle_raw * r;
	int len;

	if (s_info.b == NULL) {
		dlog_print(DLOG_ERROR, LOG_TAG, "bundle is NULL.");
		return;
	}

	if (BUNDLE_ERROR_NONE != bundle_del(s_info.b, key)) {
		dlog_print(DLOG_ERROR, LOG_TAG, "Failed at bundle_del(). Can't delete bundle.");
		return;
	}

	/*
	 * And encodes a bundle to the bundle_raw format (uses base64 format).
	 */
	bundle_encode(s_info.b, &r, &len);

	/*
	 * Sets a value(string) & length(int) using preference API.
	 */
	if (PREFERENCE_ERROR_NONE != preference_set_string("DATA_KEY_BUNDLE_RAW", (const char *) r)) {
		dlog_print(DLOG_ERROR, LOG_TAG, "Failed at preference_set_string()");
	}

	if (PREFERENCE_ERROR_NONE != preference_set_int("DATA_KEY_BUNDLE_RAW_LEN", len)) {
		dlog_print(DLOG_ERROR, LOG_TAG, "Failed at preference_set_int()");
	}
}

/*
 * @brief Removes a key-value object with the given key.
 * @param[in] key Key that uniquely identifies a bundle
 */
void data_delete_widget_data_bundle(const char *key)
{
	if (s_info.widget_data_b == NULL) {
		dlog_print(DLOG_ERROR, LOG_TAG, "bundle is NULL.");
		return;
	}

	if (BUNDLE_ERROR_NONE != bundle_del(s_info.widget_data_b, key)) {
		dlog_print(DLOG_ERROR, LOG_TAG, "Failed at bundle_del(). Can't delete bundle.");
		return;
	}
}

/*
 * @brief Gets the number of bundle items.
 */
int data_get_bundle_count(void)
{
	int count = 0;

	if (s_info.b == NULL) {
		dlog_print(DLOG_ERROR, LOG_TAG, "bundle is NULL.");
		return 0;
	}

	count = bundle_get_count(s_info.b);

	return count;
}

/*
 * @brief Allocates for data of genlist's items.
 */
struct genlist_item_data *data_alarm_create_genlist_item_data(void)
{
	struct genlist_item_data *gendata;

	gendata = malloc(sizeof(struct genlist_item_data));
	if (gendata == NULL) {
		dlog_print(DLOG_ERROR, LOG_TAG, "gendata cannot be allocated.");
		return NULL;
	}

	gendata->check_state = EINA_TRUE;
	gendata->instance_id = NULL;

	return gendata;
}

/*
 * @brief Removes data of genlist's items.
 * @param[in] gendata Data structure that stores information of genlist, such as saved time, alarm ID, check state
 */
void data_alarm_destroy_genlist_item_data(struct genlist_item_data *gendata)
{
	if (gendata == NULL) {
		dlog_print(DLOG_ERROR, LOG_TAG, "gendata is NULL.");
		return;
	}

	if (gendata->widget_id) {
		free(gendata->widget_id);
	}

	if (gendata->instance_id) {
		free(gendata->instance_id);
	}

	free(gendata);
}
/*
 * @brief Gets popup's text when the popup shows.
 * @param[in] saved_time Time that user sets
 */
char *data_get_popup_text(struct tm *saved_time)
{
	time_t t;
	struct tm *current_time;
	int hour_val, min_val;
	char time_text[BUF_LEN] = { 0, };

	if (saved_time == NULL) {
		dlog_print(DLOG_ERROR, LOG_TAG, "failed to get saved time.");
		return NULL;
	}

	/*
	 * Do the following steps.
	 * 1. Get current time
	 * 2. Calculate the rest of time until the alarm rings compared with saved time.
	 */

	t = time(NULL);
	current_time = localtime(&t);

	hour_val = saved_time->tm_hour - current_time->tm_hour;
	min_val = saved_time->tm_min - current_time->tm_min;

	if (hour_val == 0) {
		if (min_val == 0) {
			snprintf(time_text, sizeof(time_text), "%s", "Time is now.");
		} else if (min_val > 0) {
			snprintf(time_text, sizeof(time_text), "%s %d %s %s", ALARM_SET_FOR, min_val, MINUTE, "from now.");
		} else {
			/* min_val < 0 */
			int val;

			val = (MINS_AN_HOUR - current_time->tm_min) + saved_time->tm_min;
			snprintf(time_text, sizeof(time_text), "%s %d %s %s", "Alarm set for 23 hours", val, MINUTE, "from now.");
		}
	} else if (hour_val > 0) {
		if (min_val == 0) {
			snprintf(time_text, sizeof(time_text), "%s %d %s %s", ALARM_SET_FOR, hour_val, HOUR, "from now.");
		} else if (min_val > 0) {
			snprintf(time_text, sizeof(time_text), "%s %d %s %d %s %s", ALARM_SET_FOR, hour_val, HOUR, min_val, MINUTE, "from now.");
		} else {
			/* min_val < 0 */
			int val;

			val = (MINS_AN_HOUR - current_time->tm_min) + saved_time->tm_min;
			if (hour_val == 0) {
				snprintf(time_text, sizeof(time_text), "%s %d %s %s", ALARM_SET_FOR, val, MINUTE, "from now.");
			} else {
				snprintf(time_text, sizeof(time_text), "%s %d %s %d %s %s", ALARM_SET_FOR, hour_val - 1, HOUR, val, MINUTE, "from now.");
			}
		}
	} else {
		/* hour_val < 0 */
		if (min_val == 0) {
			int val;

			val = (HOURS_A_DAY - current_time->tm_hour) + saved_time->tm_hour;
			snprintf(time_text, sizeof(time_text), "%s %d %s %s", ALARM_SET_FOR, val, HOUR, "from now.");
		} else if (min_val > 0) {
			int val;

			val = (HOURS_A_DAY - current_time->tm_hour) + saved_time->tm_hour;
			snprintf(time_text, sizeof(time_text), "%s %d %s %d %s %s", ALARM_SET_FOR, val, HOUR, min_val, MINUTE, "from now.");
		} else {
			/* min_val < 0 */
			int h_val, m_val;

			h_val = (HOURS_A_DAY - current_time->tm_hour) + saved_time->tm_hour;
			m_val = (MINS_AN_HOUR - current_time->tm_min) + saved_time->tm_min;

			snprintf(time_text, sizeof(time_text), "%s %d %s %d %s %s", ALARM_SET_FOR, h_val - 1, HOUR, m_val, MINUTE, "from now.");
		}
	}

	dlog_print(DLOG_INFO, LOG_TAG, "Popup text : %s", time_text);

	return strdup(time_text);
}

/*
 * @note
 * Below functions are static functions.
 */

/*
 * @brief Creates an app_control handle.
 * @param[in] operation The operation to be performed
 * @param[in] app_id The ID of the application to explicitly launch
 */
static app_control_h _create_app_control(const char *operation, const char *app_id)
{
	app_control_h app_control;

	app_control_create(&app_control);
	app_control_set_operation(app_control, operation);

	/*
	 * Not only is PACKAGE the ID of the application to explicitly launch, but it is the package name.
	 * It is declared in "main.h"
	 */
	app_control_set_app_id(app_control, app_id);

	return app_control;
}

/*
 * @brief Destroys the app_control handle.
 * @param[in] app_control The app_control that try to destroy
 */
static void _destroy_app_control(app_control_h app_control)
{
	if (app_control == NULL) {
		dlog_print(DLOG_ERROR, LOG_TAG, "app_control is NULL");
		return;
	}

	if (APP_CONTROL_ERROR_NONE != app_control_destroy(app_control)) {
		dlog_print(DLOG_ERROR, LOG_TAG, "Failed at app_control_destroy(). Can't destroy app_control.");
	}
}

/*
 * @brief Decodes the bundle if the bundle already exists.
 */
static bundle *_decode_bundle(void)
{
	bundle *b = NULL;
	bundle_raw * r = NULL;
	char *str = NULL;
	int len = 0;

	/*
	 * Check whether the bundle already exists or not.
	 */
	preference_get_string("DATA_KEY_BUNDLE_RAW", &str);
	if (str == NULL) {
		dlog_print(DLOG_INFO, LOG_TAG, "str(DATA_KEY_BUNDLE_RAW) is NULL");
		return NULL;
	}
	preference_get_int("DATA_KEY_BUNDLE_RAW_LEN", &len);
	r = (bundle_raw *) str;

	/*
	 * Decode the bundle if the bundle already exists.
	 */
	b = bundle_decode(r, len);

	free(str);

	return b;
}

/*
 * @brief Checks whether alarm is exist.
 * @param[in] gendata Data structure that stores information of genlist, such as saved time, alarm ID, check state
 */
Eina_Bool data_check_exist_widget_alarm(struct genlist_item_data *gendata)
{
	if (gendata->instance_id != NULL) {
		dlog_print(DLOG_INFO, LOG_TAG, "This alarm is for widget");
		return EINA_TRUE;
	} else {
		dlog_print(DLOG_INFO, LOG_TAG, "This alarm is not widget");
		return EINA_FALSE;
	}
}

/* End of file */
