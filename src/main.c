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

#include <app.h>
#include <app_alarm.h>
#include <efl_extension.h>
#include <dlog.h>
#include <system_settings.h>
#include <app_preference.h>
#include <bundle.h>
#include <widget_service.h>
#include <widget_errno.h>
#include <device/power.h>

#include <Elementary.h>

#include "gear-reality-check.h"
#include "data.h"
#include "view.h"
#include "reality-check.h"

#define INSTANCE_ID_FOR_APP_CONTROL "widget_instance_id_for_app_control"

static struct main_info {
	Elm_Object_Item *padding_item;
	Elm_Object_Item *widget_alarm;
	char *widget_id;
	char *instance_id;
	int port_id_for_widget;
	Eina_Bool first_alarm;
} s_info = {
	.padding_item = NULL,
	.widget_alarm = NULL,
	.widget_id = NULL,
	.instance_id = NULL,
	.port_id_for_widget = 0,
	.first_alarm = EINA_FALSE,
};


static struct anim_data {
	Evas_Object* rect;
	int a;
	int direction;
	int count;
};

static Evas_Object *_create_layout_no_alarmlist(Evas_Object *parent, const char *edje_path, const char *group_name);
static void _set_layout_exist_alarmlist(Evas_Object *layout);
static Evas_Object *_create_layout_set_time(Evas_Object *parent);
static Evas_Object* _create_layout_ring_alarm(Evas_Object *parent, struct tm *saved_time);
static Eina_Bool _naviframe_pop_cb(void *data, Elm_Object_Item *it);
static void _no_alarm_down_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _no_alarm_up_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _no_alarm_clicked_cb(void *data, Evas_Object *obj, void *event_info);
static void _add_clicked_cb(void *data, Evas_Object *obj, void *event_info);
static void _set_clicked_cb(void *data, Evas_Object *obj, void *event_info);
static void _dismiss_clicked_cb(void *data, Evas_Object *obj, void *event_info);
static void _push_set_time_layout_to_naviframe(void);

/*
 * @brief Destroys alarm widget by instance id.
 * @param[in] user_data Data needed in this function
 */
void alarm_destroy_widget(void *user_data)
{
	struct genlist_item_data *gendata = NULL;
	bundle *b = NULL;
	int ret = 0;

	gendata = user_data;
	if (gendata == NULL) {
		dlog_print(DLOG_ERROR, LOG_TAG, "Failed to get gendata");
		return;
	}

	b = data_get_widget_data_bundle();
	if (b == NULL) {
		dlog_print(DLOG_ERROR, LOG_TAG, "Failed to get data bundle");
		return;
	}

	bundle_del(b, "Operation");
	bundle_add_str(b, "Operation", "Destroy");

	ret = widget_service_trigger_update(gendata->widget_id, gendata->instance_id, b, 0);
	if (ret != WIDGET_ERROR_NONE) {
		dlog_print(DLOG_ERROR, LOG_TAG, "Failed to (). ret = %d", ret);
	}
}

/*
 * @brief Destroys alarm widget by instance id.
 * @param[in] on_off The state of alarm(On/Off)
 * @param[in] user_data Data needed in this function
 */
void alarm_set_widget_on_off(char *on_off, void *user_data)
{
	struct genlist_item_data *gendata = NULL;
	bundle *b = NULL;
	char alarm_id_str[BUF_LEN] = { 0, };
	int ret = 0;

	gendata = user_data;
	if (gendata == NULL) {
		dlog_print(DLOG_ERROR, LOG_TAG, "Failed to get gendata");
		return;
	}

	snprintf(alarm_id_str, sizeof(alarm_id_str), "%d", gendata->alarm_id);

	b = data_get_widget_data_bundle();
	if (b == NULL) {
		dlog_print(DLOG_ERROR, LOG_TAG, "widget data bundle is NULL");
		return;
	}

	bundle_del(b, "OnOff");
	if (!strcmp(on_off, "On")) {
		bundle_add_str(b, "OnOff", "On");
	} else {
		bundle_add_str(b, "OnOff", "Off");
	}

	bundle_del(b, "AlarmId");
	bundle_add_str(b, "AlarmId", alarm_id_str);

	bundle_del(b, "Operation");
	bundle_add_str(b, "Operation", "SetOnOff");

	dlog_print(DLOG_DEBUG, LOG_TAG, "%s[%d] widget_id(%s), instance_id(%s), alarm id(%s)",
			__func__, __LINE__, gendata->widget_id, gendata->instance_id, alarm_id_str);

	widget_service_trigger_update(gendata->widget_id, gendata->instance_id, b, 0);
	if (ret != WIDGET_ERROR_NONE) {
		dlog_print(DLOG_ERROR, LOG_TAG, "Failed to widget_service_trigger_update(). ret = %d", ret);
	}
}

/*
 * @brief Hooks to take necessary actions before main event loop starts.
 * Initialize UI resources and application's data
 * If this function returns true, the main loop of application starts
 * If this function returns false, the application is terminated
 */
static bool app_create(void *user_data)
{
	Evas_Object *layout = NULL;
	Evas_Object *nf = NULL;
	char edje_path[BUF_LEN] = { 0, };

	dlog_print(DLOG_INFO, LOG_TAG, "App create");

	data_initialize();

	/*
	 * Create base GUI.
	 */
	view_create();

	/*
	 * Create GUI for alarm application.
	 */
	view_alarm_create();

	/*
	 * Create a layout when there is no alarm list.
	 */
	data_get_resource_path("edje/main.edj", edje_path, sizeof(edje_path));

	nf = view_get_naviframe();
	layout = _create_layout_no_alarmlist(nf, edje_path, "base_alarm");
	if (layout == NULL) {
		dlog_print(DLOG_ERROR, LOG_TAG, "failed to create a layout of no alarm.");
		view_destroy();
		return false;
	}

	/*
	 * Create a layout that shows alarm lists when user sets the alarm.
	 */
	_set_layout_exist_alarmlist(layout);

	/*
	 * Hide the layout that exists alarm list before user sets the alarm.
	 */
	view_send_signal_to_edje(layout, "genlist.hide", "alarm");

	/*
	 * Push the layout at a naviframe.
	 */
	view_push_item_to_naviframe(nf, layout, _naviframe_pop_cb, NULL);
	/*
	 * Save base layout.
	 */
	view_set_base_layout(layout);

	return true;
}


static Eina_Bool on_next_frame1(void *data)
{
	struct anim_data* my_anim_data = (struct anim_data*) data;
	const float tick_rate = 1.0 / 60.0;
	// This is how long it should take for one cycle white -> transparent -> white
	const float period = 0.4f;
	const float range = 255.0f;
	const float half_period = period * 0.5f;
	const float rate_of_change = (tick_rate / half_period) * range;

	my_anim_data->a += my_anim_data->direction * rate_of_change;
	if (my_anim_data->a < 0)
	{
		my_anim_data->a = 0;
		my_anim_data->direction = 1;
		my_anim_data->count++;
	} else if (my_anim_data->a > 255)
	{
		my_anim_data->a = 255;
		my_anim_data->direction = -1;
		my_anim_data->count++;
	}

	evas_object_color_set(my_anim_data->rect, 255, 255, 255, my_anim_data->a);
	if (my_anim_data->count < 5)
	{
		return ECORE_CALLBACK_RENEW;
	} else
	{
		evas_object_hide(my_anim_data->rect);
		evas_object_del(my_anim_data->rect);
		free(my_anim_data);
		return ECORE_CALLBACK_DONE;
	}
}



/*
 * @brief This callback function is called when another application
 * sends the launch request to the application
 */
static void app_control(app_control_h app_control, void *user_data)
{
	char *operation = NULL;
	char *alarm_id = NULL;
	Elm_Object_Item *item = NULL;
	Evas_Object *genlist = NULL;
	Evas_Object *nf = NULL;
	struct genlist_item_data *gendata = NULL;
	struct tm *saved_time = NULL;
	int ret = 0;

	dlog_print(DLOG_INFO, LOG_TAG, "App control");


	app_control_h app_control_2 = data_get_app_control();

	// Try to update tomorrow's alarms
	update_alarms(app_control_2);

	/*
	 * When it comes time to sound alarm that has set alarm_schedule_at_date(),
	 * alarm API calls app_control(), with operation that has set by app_control_set_operation() in advance.
	 */
	app_control_get_operation(app_control, &operation);
	dlog_print(DLOG_INFO, LOG_TAG, "operation = %s", operation);

	if (!strncmp(APP_CONTROL_OPERATION_ALARM_ONTIME, operation, strlen(APP_CONTROL_OPERATION_ALARM_ONTIME))) {
		ret = app_control_get_extra_data(app_control, APP_CONTROL_DATA_ALARM_ID, &alarm_id);
		if (ret != APP_CONTROL_ERROR_NONE) {
			dlog_print(DLOG_ERROR, LOG_TAG, "Failed to app_control_get_extra_data(). Can't get extra data.");
			free(operation);
			return;
		}

		// We don't have extra data, just show the alarm window
		//@@TODO: Remove the code that creates it



		// Turn on the screen
		ret = device_power_wakeup(false);
		if (ret != DEVICE_ERROR_NONE)
		{
			dlog_print(DLOG_ERROR, LOG_TAG, "Failed to turn display on.");
		}
		ret = device_power_request_lock(POWER_LOCK_DISPLAY, 1000);
		if (ret != DEVICE_ERROR_NONE)
		{
			dlog_print(DLOG_ERROR, LOG_TAG, "Failed to lock the display on.");
		}

		/*
		 * Create a layout when the alarm sounds.
		 */
		saved_time = &gendata->saved_time;
		nf = view_get_naviframe();
		if (!nf)
		{
			return;
		}
		Evas_Object* layout_ring_alarm = _create_layout_ring_alarm(nf, saved_time);

		// Vibrate to get user's attention
		start_alarm_vibrate();

		// Playing around with animations
		// Get the rectangle for animation purposes
		Evas_Object* layout_edje = elm_layout_edje_get(layout_ring_alarm);

		const Evas_Object* rect = edje_object_part_object_get(layout_edje, "flashing.rect");
		if (rect)
		{
			struct anim_data* my_anim_data = malloc(sizeof(struct anim_data));
			my_anim_data->direction = -1;
			my_anim_data->count = 0;
			my_anim_data->rect = rect;
			my_anim_data->a = 255;
			ecore_animator_add(on_next_frame1, my_anim_data);
			ecore_animator_frametime_set(1. / 60);
		} else
		{
			dlog_print(DLOG_INFO, LOG_TAG, "Unable to find the rectangle");
		}


		/*
		 * Remove widget and genlist's item that is consistent with alarm id.
		 */
		// alarm_destroy_widget(gendata);
		// elm_object_item_del(item);

	} else if (!strncmp(APP_CONTROL_OPERATION_MAIN, operation, strlen(APP_CONTROL_OPERATION_MAIN))) {
		evas_object_show(view_get_window());
	} else if (!strncmp(APP_CONTROL_OPERATION_DEFAULT, operation, strlen(APP_CONTROL_OPERATION_DEFAULT))) {
		ret = app_control_get_app_id(app_control, &s_info.widget_id);
		if (ret != APP_CONTROL_ERROR_NONE) {
			dlog_print(DLOG_ERROR, LOG_TAG, "Failed to get app id from appcontrol");
			free(operation);
			return;
		}

		ret = app_control_get_extra_data(app_control, INSTANCE_ID_FOR_APP_CONTROL, &s_info.instance_id);
		if (ret != APP_CONTROL_ERROR_NONE) {
			dlog_print(DLOG_ERROR, LOG_TAG, "Failed to app_control_get_extra_data(). Can't get extra data.");
			free(operation);

			free(s_info.widget_id);
			s_info.widget_id = NULL;

			return;
		}

		dlog_print(DLOG_INFO, LOG_TAG, "%s[%d] widget_id(%s), instance_id(%s)",
				__func__, __LINE__, s_info.widget_id, s_info.instance_id);

		data_initialize_widget_data();

		_push_set_time_layout_to_naviframe();
	}

	free(operation);
}

/*
 * @brief This callback function is called each time.
 * the application is completely obscured by another application
 * and becomes invisible to the user
 */
static void app_pause(void *user_data)
{
	Evas_Object *nf = NULL;
	Elm_Object_Item *top_item = NULL;
	Elm_Object_Item *bottom_item = NULL;

	/* Take necessary actions when application becomes invisible. */
	dlog_print(DLOG_INFO, LOG_TAG, "App pause");

	nf = view_get_naviframe();

	top_item = elm_naviframe_top_item_get(nf);
	bottom_item = elm_naviframe_bottom_item_get(nf);

	/*
	 * There are items to pop if the top of the naviframe's item is NOT the same as the bottom of the naviframe's item.
	 */
	if (top_item != bottom_item) {
		elm_naviframe_item_pop(nf);
	}
}

/*
 * @brief This callback function is called each time.
 * the application becomes visible to the user
 */
static void app_resume(void *user_data)
{
	/* Take necessary actions when application becomes visible. */
	dlog_print(DLOG_INFO, LOG_TAG, "App resume");

	evas_object_show(view_get_window());
}

/*
 * @brief This callback function is called once after the main loop of the application exits.
 */
static void app_terminate(void *user_data)
{
	/*
	 * Release all resources.
	 */
	dlog_print(DLOG_INFO, LOG_TAG, "App terminate");

	if (ALARM_ERROR_NONE != alarm_cancel_all()) {
		dlog_print(DLOG_ERROR, LOG_TAG, "Failed to cancel all scheduled alarms.");
	}

	view_alarm_destroy();

	data_widget_data_finalize();
	data_finalize();

	view_destroy();
}

/*
 * @brief This function will be called when the language is changed.
 */
static void ui_app_lang_changed(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_LANGUAGE_CHANGED*/
	char *locale = NULL;

	system_settings_get_value_string(SYSTEM_SETTINGS_KEY_LOCALE_LANGUAGE, &locale);

	if (locale != NULL) {
		elm_language_set(locale);
		free(locale);
	}
	return;
}

/*
 * @brief Main function of the application.
 */
int main(int argc, char *argv[])
{
	int ret;

	ui_app_lifecycle_callback_s event_callback = {0, };
	app_event_handler_h handlers[5] = {NULL, };

	event_callback.create = app_create;
	event_callback.terminate = app_terminate;
	event_callback.pause = app_pause;
	event_callback.resume = app_resume;
	event_callback.app_control = app_control;

	/*
	 * If you want to handling more events,
	 * Please check the application lifecycle guide
	 */
	ui_app_add_event_handler(&handlers[APP_EVENT_LANGUAGE_CHANGED], APP_EVENT_LANGUAGE_CHANGED, ui_app_lang_changed, NULL);

	ret = ui_app_main(argc, argv, &event_callback, NULL);
	if (ret != APP_ERROR_NONE)
		dlog_print(DLOG_ERROR, LOG_TAG, "ui_app_main() is failed. err = %d", ret);

	return ret;
}

/*
 * @note
 * Below functions are static functions.
 */

/*
 * @brief Creates layout for the first page of the alarm application when there is no alarm list.
 * @param[in] parent The object to which you want to add this layout
 * @param[in] edje_path The path of EDJE
 * @param[in] group_name The name of group in EDJE you want to set to
 */
static Evas_Object *_create_layout_no_alarmlist(Evas_Object *parent, const char *edje_path, const char *group_name)
{
	Evas_Object *layout = NULL;

	if (parent == NULL) {
		dlog_print(DLOG_ERROR, LOG_TAG, "failed to get parent.");
	}

	layout = view_create_layout(parent, edje_path, "base_alarm", NULL, NULL);
	if (layout == NULL) {
		dlog_print(DLOG_ERROR, LOG_TAG, "failed to create a layout of base alarm.");
		return NULL;
	}

	view_set_text(layout, "no_alarm.title", "Alarm");

	// view_set_button(layout, "swallow.no_alarm.button", "focus", NULL, NULL, _no_alarm_down_cb, _no_alarm_up_cb, _no_alarm_clicked_cb, layout);

	view_set_spinner(layout, "spinner.number_of_alarms", 1, 10);

	view_set_text(layout, "no_alarm.text", "Add alarm");

	return layout;
}

/*
 * @brief Sets layout to given layout for the first page that shows alarm lists when user set the alarm.
 * @param[in] layout Layout will be shown the alarm lists
 */
static void _set_layout_exist_alarmlist(Evas_Object *layout)
{
	Evas_Object *genlist = NULL;

	if (!layout) {
		dlog_print(DLOG_ERROR, LOG_TAG, "failed to get layout.");
		return;
	}

	genlist = view_create_circle_genlist(layout);
	if (genlist == NULL) {
		dlog_print(DLOG_ERROR, LOG_TAG, "failed to create a genlist of saving alarm.");
		return;
	}

	/*
	 * Append a genlist's item as a title.
	 */
	view_append_item_to_genlist(genlist, "title", NULL, NULL, NULL);

	/*
	 * Create a genlist's item as a padding item.
	 * Padding item makes genlist's items is located at the middle of the screen.
	 */
	s_info.padding_item = view_append_item_to_genlist(genlist, "padding", NULL, NULL, NULL);
	if (s_info.padding_item == NULL) {
		dlog_print(DLOG_ERROR, LOG_TAG, "failed to create padding item of genlist.");
		return;
	}

	view_set_content_to_part(layout, "swallow.genlist", genlist);

	view_set_button(layout, "swallow.genlist.button", "bottom", NULL, "Add", NULL, NULL, _add_clicked_cb, NULL);

	view_set_genlist(genlist);
}

/*
 * @brief Creates layout for setting time.
 * @param[in] parent The object to which you want to add this layout
 */
static Evas_Object *_create_layout_set_time(Evas_Object *parent)
{
	Evas_Object *layout = NULL;
	char image_path[BUF_LEN] = {0, };

	if (!parent) {
		dlog_print(DLOG_ERROR, LOG_TAG, "failed to get parent.");
		return NULL;
	}

	/*
	 * Create a layout that is able to set time.
	 */
	layout = view_create_layout_by_theme(parent, "layout", "circle", "datetime");

	/*
	 * Set the title of the layout.
	 */
	view_set_text(layout, "elm.text", "Set alarm");

	/*
	 * Set bottom button attached check image.
	 * The button can set the alarm when it is pressed.
	 */
	data_get_resource_path("images/ic_popup_btn_check.png", image_path, sizeof(image_path));

	view_set_button(layout, "elm.swallow.btn", "bottom", image_path, NULL, NULL, NULL, _set_clicked_cb, NULL);

	/*
	 * Create datetime.
	 */
	view_create_datetime(layout, "timepicker/circle");

	return layout;
}

/*
 * @brief Creates layout for a page that shows when the alarm sounds.
 * @param[in] parent The object to which you want to add this layout
 * @param[in] saved_time Time that sound the alarm
 */
static Evas_Object* _create_layout_ring_alarm(Evas_Object *parent, struct tm *saved_time)
{
	Evas_Object *layout = NULL;
	char buf[BUF_LEN] = {0, };
	char file_path[BUF_LEN] = {0, };

	if (parent == NULL) {
		dlog_print(DLOG_ERROR, LOG_TAG, "failed to get parent.");
		return NULL;
	}

	/*
	 * Create a layout that shows when the alarm sounds.
	 */
	data_get_resource_path("edje/main.edj", file_path, sizeof(file_path));

	layout = view_create_layout(parent, file_path, "ringing_alarm", NULL, NULL);
	if (layout == NULL) {
		dlog_print(DLOG_ERROR, LOG_TAG, "failed to create a layout.");
		return NULL;
	}

	if (saved_time) {
		strftime(buf, sizeof(buf) - 1, "%l:%M %p", saved_time);
		view_set_text(layout, "ringing_alarm.text", buf);
	}

	Evas_Object* test = elm_layout_edje_get(layout);


	dlog_print(DLOG_INFO, LOG_TAG, "Testing if 'ringing_alarm.text' part exists: %s\n",
	           edje_object_part_exists(test, "ringing_alarm.text") ? "yes!" : "no");

	dlog_print(DLOG_INFO, LOG_TAG, "Testing if 'flashing.rect' part exists: %s\n",
		           edje_object_part_exists(test, "flashing.rect") ? "yes!" : "no");

	/*
	 * Set bottom button.
	 * The button can dismiss the alarm when it is pressed.
	 */
	view_set_button(layout, "swallow.button", "bottom", NULL, "Dismiss", NULL, NULL, _dismiss_clicked_cb, NULL);

	view_push_item_to_naviframe(parent, layout, NULL, NULL);

	return layout;
}

/*
 * @brief This function will be operated when preference value is changed.
 * @param[in] key The name of the key to monitor
 * @param[in] user_data The user data to be passed to the callback function
 */
static void _alarm_on_off_changed_cb(const char *key, void *user_data)
{
	struct genlist_item_data *gendata;
	char *on_off = NULL;
	int ret = 0;
	Eina_Bool check_state = EINA_FALSE;
	Eina_Bool signal = EINA_FALSE;
	Elm_Object_Item *item = NULL;

	gendata = user_data;
	if (gendata == NULL) {
		dlog_print(DLOG_ERROR, LOG_TAG, "Failed to get gendata");
		return;
	}

	check_state = gendata->check_state;
	if (check_state == EINA_FALSE) {
		dlog_print(DLOG_ERROR, LOG_TAG, "check_state is strange");
	}

	item = gendata->item;
	if (item == NULL) {
		dlog_print(DLOG_ERROR, LOG_TAG, "item is strange");
	}

	ret = preference_get_string(key, &on_off);
	if (ret != PREFERENCE_ERROR_NONE) {
		dlog_print(DLOG_ERROR, LOG_TAG, "Failed to get preference value : on/off");
		return;
	}

	if (!strcmp(on_off, "On")) {
		dlog_print(DLOG_INFO, LOG_TAG, "signal is On");
		signal = EINA_TRUE;
	} else {
		dlog_print(DLOG_INFO, LOG_TAG, "signal is Off");
		signal = EINA_FALSE;
	}

	free(on_off);

	if (signal == check_state) {
		dlog_print(DLOG_INFO, LOG_TAG, "Signal is from itself, DO NOT ANYTHING");
	} else {
		struct tm *saved_time = NULL;
		int alarm_id = 0;
		char buf[BUF_LEN] = {0, };

		saved_time = &gendata->saved_time;
		alarm_id = gendata->alarm_id;

		if (signal == EINA_TRUE) {
			app_control_h app_control;

			/*
			 * Store the current state of check box in gendata.
			 */
			gendata->check_state = signal;

			/*
			 * Reset alarm using time that user sets before.
			 * But, alarm ID is new.
			 */
			app_control = data_get_app_control();
			alarm_schedule_at_date(app_control, saved_time, 0, &alarm_id);

			/*
			 * Store the new alarm ID in gendata.
			 */
			gendata->alarm_id = alarm_id;

			/*
			 * Add bundle using specific alarm ID as key.
			 */
			snprintf(buf, sizeof(buf), "%d", alarm_id);
			data_add_bundle_by_str(buf, buf);
		} else {
			/*
			 * Store the current state of check box in gendata.
			 */
			gendata->check_state = signal;

			/*
			 * Cancels the alarm with the specific alarm ID.
			 */
			alarm_cancel(alarm_id);

			/*
			 * Remove bundle using specific alarm ID as key.
			 */
			snprintf(buf, sizeof(buf), "%d", alarm_id);
			data_delete_bundle(buf);
		}
		elm_genlist_item_update(item);
	}
}

/*
 * @brief Sends a message to alarm widget when the alarm is scheduled.
 * @param[in] gendata Gendata is used to get time of scheduled alarm that will be sent to alarm widget
 */
static void _alarm_set_time_for_widget(void *user_data)
{
	struct genlist_item_data *gendata = user_data;
	struct tm *saved_time = NULL;
	char alarm_time[BUF_LEN] = { 0, };
	char alarm_id_str[BUF_LEN] = { 0, };
	int ret = 0;


	if (gendata == NULL) {
		dlog_print(DLOG_ERROR, LOG_TAG, "gendata is NULL");
		return;
	}

	/*
	 * Let the widget knows what the time is for alarm.
	 */
	saved_time = &gendata->saved_time;
	if (saved_time->tm_hour > 12) {
		snprintf(alarm_time, sizeof(alarm_time), "%02d:%02d PM", saved_time->tm_hour, saved_time->tm_min);
	} else {
		snprintf(alarm_time, sizeof(alarm_time), "%02d:%02d AM", saved_time->tm_hour, saved_time->tm_min);
	}

	snprintf(alarm_id_str, sizeof(alarm_id_str), "%d", gendata->alarm_id);

	data_add_widget_data_bundle_by_str("Operation", "SetAlarm");
	data_add_widget_data_bundle_by_str("AlarmTime", alarm_time);
	data_add_widget_data_bundle_by_str("OnOff", "On");
	data_add_widget_data_bundle_by_str("AlarmId", alarm_id_str);

	gendata->widget_id = strdup(s_info.widget_id);
	gendata->instance_id = strdup(s_info.instance_id);
	dlog_print(DLOG_DEBUG, LOG_TAG, "%s[%d] widget_id(%s), instance id(%s)", __func__, __LINE__, s_info.widget_id, s_info.instance_id);

	ret = widget_service_trigger_update(gendata->widget_id, gendata->instance_id, data_get_widget_data_bundle(), 0);
	if (ret != WIDGET_ERROR_NONE) {
		dlog_print(DLOG_ERROR, LOG_TAG, "Failed to widget_service_trigger_update(). ret = %d", ret);
	}

	ret = preference_set_string(alarm_id_str, "On");
	if (ret != PREFERENCE_ERROR_NONE) {
		dlog_print(DLOG_ERROR, LOG_TAG, "Failed to set preference string : %s", alarm_id_str);
	}

	ret = preference_set_changed_cb(alarm_id_str, _alarm_on_off_changed_cb, gendata);
	if (ret != PREFERENCE_ERROR_NONE) {
		dlog_print(DLOG_ERROR, LOG_TAG, "Failed to set changed cb(%d) : on/off", ret);
	}

	free(s_info.instance_id);
	free(s_info.widget_id);
	s_info.instance_id = NULL;
	s_info.widget_id = NULL;
}

/*
 * @brief This function will be operated when the first item of the naviframe is going to popped.
 * @param[in] data Data needed in this function
 * @param[in] it The item of naviframe
 */
static Eina_Bool _naviframe_pop_cb(void *data, Elm_Object_Item *it)
{
	ui_app_exit();

	return EINA_FALSE;
}

/*
 * @brief This function will be operated when the button is pressed.
 * @param[in] data Data will be the same value passed to evas_object_event_callback_add() as the data parameter
 * @param[in] e The e lets the user know what evas canvas the event occurred on
 * @param[in] obj Object handles on which the event occurred
 * @param[in] event_info Event_info is a pointer to a data structure about event
 */
static void _no_alarm_down_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	Evas_Object *layout = data;

	if (layout == NULL) {
		dlog_print(DLOG_ERROR, LOG_TAG, "Failed to get layout.");
	}

	view_send_signal_to_edje(layout, "mouse.down", "button");
}

/*
 * @brief This function will be operated when the button is released.
 * @param[in] data Data will be the same value passed to evas_object_event_callback_add() as the data parameter
 * @param[in] e The e lets the user know what evas canvas the event occurred on
 * @param[in] obj The Object handles on which the event occurred
 * @param[in] event_info Event_info is a pointer to a data structure about event
 */
static void _no_alarm_up_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	Evas_Object *layout = data;

	if (layout == NULL) {
		dlog_print(DLOG_ERROR, LOG_TAG, "Failed to get layout.");
	}

	view_send_signal_to_edje(layout, "mouse.up", "button");
}

/*
 * @brief This function will be operated when the button is clicked.
 * @param[in] data Data has the same value passed to evas_object_smart_callback_add() as the data parameter
 * @param[in] obj The obj is a handle to the object on which the event occurred
 * @param[in] event_info A pointer to data which is totally dependent on the smart object's implementation and semantic for the given event
 */
static void _no_alarm_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
	_push_set_time_layout_to_naviframe();
}

/*
 * @brief This function will be operated when the button is clicked.
 * @param[in] data Data has the same value passed to evas_object_smart_callback_add() as the data parameter
 * @param[in] obj The obj is a handle to the object on which the event occurred
 * @param[in] event_info A pointer to data which is totally dependent on the smart object's implementation and semantic for the given event
 */
static void _add_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
	Evas_Object *nf = NULL;
	Evas_Object *layout = NULL;

	/*
	 * Add layout that is able to set time to naviframe.
	 */
	nf = view_get_naviframe();

	layout = _create_layout_set_time(nf);
	if (layout == NULL) {
		dlog_print(DLOG_ERROR, LOG_TAG, "failed to create a layout of setting time.");
		return;
	}

	view_push_item_to_naviframe(nf, layout, NULL, NULL);
}

/*
 * @brief This function will be operated when the button is clicked.
 * @param[in] data Data has the same value passed to evas_object_smart_callback_add() as the data parameter
 * @param[in] obj The obj is a handle to the object on which the event occurred
 * @param[in] event_info A pointer to data which is totally dependent on the smart object's implementation and semantic for the given event
 */
static void _set_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
	struct genlist_item_data *gendata = NULL;
	Evas_Object *layout = NULL;
	Evas_Object *genlist = NULL;
	Evas_Object *nf = NULL;
	char *popup_text = NULL;
	char buf[BUF_LEN] = {0, };

	/*
	 * Do the following steps when press set button.
	 * 1. Allocate gendata memory.
	 * 2. Set alarm by using alarm API.
	 * 3. Store the alarm id in bundle.
	 * 4. Create popup that shows how much time left before the alarm rings.
	 * 5. Append the alarm to genlist as a item.
	 * 6. Pop the current layout from naviframe.
	 */

	/*
	 * Allocate gendata memory.
	 */
	gendata = data_alarm_create_genlist_item_data();
	if (gendata == NULL) {
		dlog_print(DLOG_ERROR, LOG_TAG, "gendata is NULL");
		return;
	}

	/*
	 * Set alarm by using alarm API.
	 */
	view_alarm_schedule_alarm(gendata);

	/*
	 * Store the alarm id in bundle.
	 */
	snprintf(buf, sizeof(buf), "%d", gendata->alarm_id);
	data_add_bundle_by_str(buf, buf);

	/*
	 * Create popup that shows how much time left before the alarm rings.
	 */
	layout = view_get_base_layout();
	popup_text = data_get_popup_text(&gendata->saved_time);
	view_create_text_popup(layout, 2.0, popup_text);
	if (popup_text) {
		free(popup_text);
	}

	/*
	 * Append the alarm to genlist as a item.
	 */
	elm_object_item_del(s_info.padding_item);
	if (s_info.padding_item) {
		s_info.padding_item = NULL;
	}

	nf = view_get_naviframe();
	genlist = view_get_genlist();

	gendata->item = view_append_item_to_genlist(genlist, "1text.1icon.1", (void *)gendata, NULL, NULL);

	view_send_signal_to_edje(layout, "genlist.show", "alarm");

	s_info.padding_item = view_append_item_to_genlist(genlist, "padding", NULL, NULL, NULL);

	/*
	 * Pop the current layout from naviframe.
	 */
	elm_naviframe_item_pop(nf);

	if (s_info.instance_id) {
		_alarm_set_time_for_widget(gendata);
	} else {
		data_initialize_widget_id_in_gendata(gendata);
	}
}

/*
 * @brief This function will be operated when the button is clicked.
 * @param[in] data Data has the same value passed to evas_object_smart_callback_add() as the data parameter
 * @param[in] obj The object is a handle to the object on which the event occurred
 * @param[in] event_info A pointer to data which is totally dependent on the smart object's implementation and semantic for the given event
 */
static void _dismiss_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
	Evas_Object *layout = NULL;
	Evas_Object *nf = NULL;
	Evas_Object *genlist = NULL;
	int count = 0;

	/*
	 * There are two genlist items when the alarm list is empty.
	 * (The remaining two genlist items, one is the title and another one is padding.)
	 *  Alarm app hides layout that shows alarm list if alarm list is empty using view_send_signal_to_edje().
	 */
	genlist = view_get_genlist();
	count = elm_genlist_items_count(genlist);

	if (count < 3) {
		layout = view_get_base_layout();
		view_send_signal_to_edje(layout, "genlist.hide", "alarm");
	}

	nf = view_get_naviframe();
	elm_naviframe_item_pop(nf);
}

/*
 * @brief This function pushes a specific layout to naviframe.
 */
static void _push_set_time_layout_to_naviframe(void)
{
	Evas_Object *nf = NULL;
	Evas_Object *layout = NULL;

	/*
	 * Add layout that is able to set time to naviframe.
	 */
	nf = view_get_naviframe();

	layout = _create_layout_set_time(nf);
	if (layout == NULL) {
		dlog_print(DLOG_ERROR, LOG_TAG, "failed to create a layout of setting time.");
		return;
	}

	view_push_item_to_naviframe(nf, layout, NULL, NULL);
}
/* End of files */
