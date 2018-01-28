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

#include <tizen.h>
#include <dlog.h>
#include <efl_extension.h>
#include <Elementary.h>
#include <app_alarm.h>
#include <app.h>
#include <bundle.h>
#include <app_preference.h>

#include "gear-reality-check.h"
#include "data.h"
#include "view.h"

#define FORMAT "%d/%b/%Y%I:%M%p"

static struct view_info {
	Evas_Object *win;
	Evas_Object *conform;
	Evas_Object *layout;
	Evas_Object *nf;
	Evas_Object *genlist;
	Evas_Object *datetime;
	Eext_Circle_Surface *circle_surface;
} s_info = {
	.win = NULL,
	.conform = NULL,
	.layout = NULL,
	.nf = NULL,
	.genlist = NULL,
	.datetime = NULL,
	.circle_surface = NULL,
};

static Elm_Genlist_Item_Class *_set_genlist_item_class(const char *style);
static void _win_delete_request_cb(void *data, Evas_Object *obj, void *event_info);
static void _gl_selected_cb(void *data, Evas_Object *obj, void *event_info);
static Evas_Object* _get_check_icon(void *data, Evas_Object *obj, const char *part);
static void _del_data(void *data, Evas_Object *obj);
static void _icon_clicked_cb(void *data, Evas_Object *obj, void *event_info);
static void _popup_hide_finished_cb(void *data, Evas_Object *obj, void *event_info);
static void _popup_hide_cb(void *data, Evas_Object *obj, void *event_info);
static void _naviframe_back_cb(void *data, Evas_Object *obj, void *event_info);

/*
 * @brief Gets window.
 */
Evas_Object * view_get_window(void)
{
	return s_info.win;
}

/*
 * @brief Gets layout.
 */
Evas_Object *view_get_base_layout(void)
{
	return s_info.layout;
}

/*
 * @brief Sets layout.
 * @param[in] layout The layout will be stored in view_info
 */
void view_set_base_layout(Evas_Object *layout)
{
	s_info.layout = layout;
}

/*
 * @brief Gets naviframe.
 */
Evas_Object *view_get_naviframe(void)
{
	return s_info.nf;
}

/*
 * @brief Gets genlist.
 */
Evas_Object *view_get_genlist(void)
{
	return s_info.genlist;
}

/*
 * @brief Sets genlist.
 * @param[in] genlist Genlist will be stored in view_info
 */
void view_set_genlist(Evas_Object *genlist)
{
	s_info.genlist = genlist;
}

/*
 * @brief Creates essential Objects window, conformant and layout.
 */
Eina_Bool view_create(void)
{
	/* Create window */
	s_info.win = view_create_win(PACKAGE);
	if (s_info.win == NULL) {
		dlog_print(DLOG_ERROR, LOG_TAG, "failed to create a window.");
		return EINA_FALSE;
	}

	/* Create conformant */
	s_info.conform = view_create_conformant_without_indicator(s_info.win);
	if (s_info.conform == NULL) {
		dlog_print(DLOG_ERROR, LOG_TAG, "failed to create a conformant");
		return EINA_FALSE;
	}

	/* Show window after main view is set up */
	evas_object_show(s_info.win);
	return EINA_TRUE;
}

/*
 * @brief Creates essential Objects only use in this application.
 */
void view_alarm_create(void)
{
	s_info.nf = view_create_naviframe(s_info.conform);
	if (s_info.nf == NULL) {
		dlog_print(DLOG_ERROR, LOG_TAG, "failed to create a naviframe.");
		evas_object_del(s_info.win);
		return;
	}

	/*
	 * Eext circle
	 * Create Eext circle surface for circular genlist and datetime object
	 * This make this app can show circular layout.
	 */
	s_info.circle_surface = eext_circle_surface_naviframe_add(s_info.nf);
}

/*
 * @brief Makes a basic window named package.
 * @param[in] pkg_name The name of the window
 */
Evas_Object *view_create_win(const char *pkg_name)
{
	Evas_Object *win = NULL;

	/*
	 * Window
	 * Create and initialize elm_win.
	 * elm_win is mandatory to manipulate window.
	 */

	win = elm_win_util_standard_add(pkg_name, pkg_name);
	elm_win_conformant_set(win, EINA_TRUE);
	elm_win_autodel_set(win, EINA_TRUE);

	/* Rotation setting */
	if (elm_win_wm_rotation_supported_get(win)) {
		int rots[4] = { 0, 90, 180, 270 };
		elm_win_wm_rotation_available_rotations_set(win, (const int *) (&rots), 4);
	}

	evas_object_smart_callback_add(win, "delete,request", _win_delete_request_cb, NULL);

	return win;
}

/*
 * @brief Makes conformant without indicator for wearable app.
 * @param[in] win Window to which you want to set this conformant
 */
Evas_Object *view_create_conformant_without_indicator(Evas_Object *win)
{
	Evas_Object *conform = NULL;

	/* Conformant
	 * Create and initialize elm_conformant.
	 * elm_conformant is mandatory for base GUI to have proper size
	 * when indicator or virtual keypad is visible.
	 */

	if (win == NULL) {
		dlog_print(DLOG_ERROR, LOG_TAG, "window is NULL.");
		return NULL;
	}

	conform = elm_conformant_add(win);
	evas_object_size_hint_weight_set(conform, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_win_resize_object_add(win, conform);
	evas_object_show(conform);

	return conform;
}

/*
 * @brief Makes a layout to target parent object with edje file.
 * @param[in] parent The object to which you want to add this layout
 * @param[in] file_path File path of EDJ file will be used
 * @param[in] group_name Name of group in EDJ you want to set to
 * @param[in] cb_function The function will be called when back event is detected
 * @param[in] user_data The user data to be passed to the callback functions
 */
Evas_Object *view_create_layout(Evas_Object *parent, const char *file_path, const char *group_name, Eext_Event_Cb cb_function, void *user_data)
{
	Evas_Object *layout = NULL;

	if (parent == NULL) {
		dlog_print(DLOG_ERROR, LOG_TAG, "parent is NULL.");
		return NULL;
	}

	layout = elm_layout_add(parent);
	elm_layout_file_set(layout, file_path, group_name);

	evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

	if (cb_function)
		eext_object_event_callback_add(layout, EEXT_CALLBACK_BACK, cb_function, user_data);

	evas_object_show(layout);

	return layout;
}

/*
 * @brief Make layout with theme.
 * @param[in] parent The object to which you want to add this layout
 * @param[in] class The class of the group
 * @param[in] group The group
 * @param[in] style The style to use
 */
Evas_Object *view_create_layout_by_theme(Evas_Object *parent, const char *class, const char *group, const char *style)
{
	Evas_Object *layout = NULL;

	/*
	 * Layout
	 * Create and initialize elm_layout.
	 * view_create_layout_by_theme() is used to create layout by using pre-made edje file.
	 */

	if (parent == NULL) {
		dlog_print(DLOG_ERROR, LOG_TAG, "parent is NULL.");
		return NULL;
	}

	layout = elm_layout_add(parent);
	elm_layout_theme_set(layout, class, group, style);
	evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

	evas_object_show(layout);

	return layout;
}

/*
 * @brief Sets datetime to the part.
 * @param[in] parent The object to which you want to set datetime
 * @param[in] style Style of the datetime
 */
Evas_Object *view_create_datetime(Evas_Object *parent, const char *style)
{
	Evas_Object *circle_datetime = NULL;

	if (parent == NULL) {
		dlog_print(DLOG_ERROR, LOG_TAG, "failed to get parent.");
		return NULL;
	}

	if (s_info.circle_surface == NULL) {
		dlog_print(DLOG_ERROR, LOG_TAG, "failed to get circle_surface.");
		return NULL;
	}

	s_info.datetime = elm_datetime_add(parent);
	circle_datetime = eext_circle_object_datetime_add(s_info.datetime, s_info.circle_surface);

	eext_rotary_object_event_activated_set(s_info.datetime, EINA_TRUE);
	elm_datetime_format_set(s_info.datetime, FORMAT);

	elm_object_style_set(s_info.datetime, style);

	elm_object_part_content_set(parent, "elm.swallow.content", s_info.datetime);

	return s_info.datetime;
}

/*
 * @brief Destroys window and free important data to finish this application.
 */
void view_destroy(void)
{
	if (s_info.win == NULL)
		return;

	evas_object_del(s_info.win);
}

/*
 * @brief Frees data of genlist's item.
 */
void view_alarm_destroy(void)
{
	struct genlist_item_data *gendata;
	Elm_Object_Item *item;
	int item_count;

	item_count = elm_genlist_items_count(s_info.genlist);

	item_count -= 2;
	while (item_count--) {
		item = elm_genlist_nth_item_get(s_info.genlist, 1);

		gendata = elm_object_item_data_get(item);
		alarm_destroy_widget(gendata);

		elm_object_item_del(item);
		if (item) item = NULL;
	}
}

/*
 * @brief Sets a image to given part.
 * @param[in] parent The object has part to which you want to set this image
 * @param[in] part_name Part name to which you want to set this image
 * @param[in] image_path Path of the image file
 */
void view_set_image(Evas_Object *parent, const char *part_name, const char *image_path)
{
	Evas_Object *image = NULL;

	if (parent == NULL) {
		dlog_print(DLOG_ERROR, LOG_TAG, "parent is NULL.");
		return;
	}

	image = elm_object_part_content_get(parent, part_name);
	if (image == NULL) {
		image = elm_image_add(parent);
		if (image == NULL) {
			dlog_print(DLOG_ERROR, LOG_TAG, "failed to create an image object.");
			return;
		}

		elm_object_part_content_set(parent, part_name, image);
	}

	if (EINA_FALSE == elm_image_file_set(image, image_path, NULL)) {
		dlog_print(DLOG_ERROR, LOG_TAG, "failed to set image.");
		return;
	}

	evas_object_show(image);

	return;
}

/*
 * @brief Sets text to the part.
 * @param[in] parent Object has part to which you want to set text
 * @param[in] part_name Part name to which you want to set text
 * @param[in] text Text you want to set to the part
 */
void view_set_text(Evas_Object *parent, const char *part_name, const char *text)
{
	if (parent == NULL) {
		dlog_print(DLOG_ERROR, LOG_TAG, "parent is NULL.");
		return;
	}

	/* Set text of target part object */
	elm_object_part_text_set(parent, part_name, text);
}

/*
 * @brief Makes naviframe and set to parent.
 * @param[in] parent Object to which you want to set naviframe
 * @Add callback function will be operated when back key is pressed.
 */
Evas_Object *view_create_naviframe(Evas_Object *parent)
{
	/*
	 * Naviframe
	 * Create and initialize elm_naviframe.
	 * Naviframe stands for navigation frame.
	 * elm_naviframe is a views manager for applications.
	 * Naviframe make changing of view is easy and effectively.
	 */
	Evas_Object *nf = NULL;

	if (parent == NULL) {
		dlog_print(DLOG_ERROR, LOG_TAG, "parent is NULL.");
		return NULL;
	}

	nf = elm_naviframe_add(parent);

	elm_object_part_content_set(parent, "elm.swallow.content", nf);
	eext_object_event_callback_add(nf, EEXT_CALLBACK_BACK, _naviframe_back_cb, NULL);

	evas_object_show(nf);

	return nf;
}

/*
 * @brief Pushes item to naviframe.
 * @param[in] nf Naviframe
 * @param[in] item The object will be added to naviframe
 * @param[in] _pop_cb Function will be operated when this item is popped from naviframe
 * @param[in] cb_data Data needed to operate '_pop_cb' function
 */
Elm_Object_Item* view_push_item_to_naviframe(Evas_Object *nf, Evas_Object *item, Elm_Naviframe_Item_Pop_Cb _pop_cb, void *cb_data)
{
	Elm_Object_Item* nf_it = NULL;

	if (nf == NULL) {
		dlog_print(DLOG_ERROR, LOG_TAG, "naviframe is NULL.");
		return NULL;
	}

	if (item == NULL) {
		dlog_print(DLOG_ERROR, LOG_TAG, "item is NULL.");
		return NULL;
	}

	nf_it = elm_naviframe_item_push(nf, NULL, NULL, NULL, item, "empty");

	if (_pop_cb != NULL)
		elm_naviframe_item_pop_cb_set(nf_it, _pop_cb, cb_data);

	return nf_it;
}

/*
 * @brief Makes genlist for circular shape.
 * @param[in] parent Object to which you want to set genlist
 */
Evas_Object *view_create_circle_genlist(Evas_Object *parent)
{
	Evas_Object *genlist = NULL;
	Evas_Object *circle_genlist = NULL;

	if (parent == NULL) {
		dlog_print(DLOG_ERROR, LOG_TAG, "parent is NULL.");
		return NULL;
	}

	if (s_info.circle_surface == NULL) {
		dlog_print(DLOG_ERROR, LOG_TAG, "circle surface is NULL.");
		return NULL;
	}

	genlist = elm_genlist_add(parent);
	/*
	 * This make selected list item is shown compressed.
	 */
	elm_genlist_mode_set(genlist, ELM_LIST_COMPRESS);
	evas_object_smart_callback_add(genlist, "selected", _gl_selected_cb, NULL);

	/*
	 * This make genlist style circular.
	 */
	circle_genlist = eext_circle_object_genlist_add(genlist, s_info.circle_surface);
	eext_circle_object_genlist_scroller_policy_set(circle_genlist, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_AUTO);
	eext_rotary_object_event_activated_set(circle_genlist, EINA_TRUE);

	evas_object_show(genlist);

	return genlist;
}

/*
 * @brief Adds a item to genlist.
 * @param[in] genlist Genlist
 * @param[in] style Style of item determine how to show this item, such as "1text", "1text1icon" and so on
 * @param[in] data Item data that use item's callback function
 * @param[in] _clicked_cb Function will be operated when the item is clicked
 * @param[in] cb_data Data needed in '_clicked_cb' function
 * This make item's class and add item to genlist.
 */
Elm_Object_Item *view_append_item_to_genlist(Evas_Object *genlist, const char *style,
		const void *data, Evas_Smart_Cb _clicked_cb, const void *cb_data)
{
	Elm_Genlist_Item_Class *item_class;
	Elm_Object_Item *item;

	if (genlist == NULL) {
		dlog_print(DLOG_ERROR, LOG_TAG, "genlist is NULL.");
		return NULL;
	}

	if (style == NULL) {
		dlog_print(DLOG_ERROR, LOG_TAG, "item style is NULL.");
		return NULL;
	}

	item_class = _set_genlist_item_class(style);

	item = elm_genlist_item_append(genlist, item_class, data, NULL, ELM_GENLIST_ITEM_NONE, _clicked_cb, cb_data);

	elm_genlist_item_class_free(item_class);

	return item;
}

/*
 * @brief Finds a item from genlist.
 * @param[in] genlist Genlist
 * @param[in] val Value determine which of the items has to remove
 */
Elm_Object_Item *view_alarm_find_item_from_genlist(Evas_Object *genlist, int val)
{
	Elm_Object_Item *item = NULL;
	struct genlist_item_data *gendata = NULL;
	int item_count = 0;
	int alarm_id = 0;
	int i;

	if (genlist == NULL) {
		dlog_print(DLOG_ERROR, LOG_TAG, "genlist is NULL.");
		return NULL;
	}

	item_count = elm_genlist_items_count(genlist);

	/*
	 * The fist item and the last item are "padding".
	 */
	for (i = 1; i < item_count - 1; i++) {
		item = elm_genlist_nth_item_get(genlist, i);
		gendata = elm_object_item_data_get(item);

		alarm_id = gendata->alarm_id;

		if (alarm_id == val) {
			return item;
		}
	}

	return NULL;
}


void view_set_spinner(Evas_Object* parent, const char* part_name, double min, double max)
{
	Evas_Object *spinner = edje_object_part_external_object_get(elm_layout_edje_get(parent), "spinner.number_of_alarms");
	elm_spinner_min_max_set(spinner, min, max);
	double value = elm_spinner_value_get(spinner);
}

/*
 * @brief Makes and set a button.
 * @param[in] parent Object to which you want to set the button
 * @param[in] style Style of the button
 * @param[in] text Text will be written on the button
 * @param[in] image_path Path of image file will be used as button icon
 * @param[in] part_name Part name in EDJ to which you want to set the button
 * @param[in] down_cb Function will be operated when the button is pressed
 * @param[in] up_cb Function will be operated when the button is released
 * @param[in] clicked_cb Function will be operated when the button is clicked
 * @param[in] data Data needed in this function
 */
void view_set_button(Evas_Object *parent, const char *part_name, const char *style, const char *image_path, const char *text,
		Evas_Object_Event_Cb down_cb, Evas_Object_Event_Cb up_cb, Evas_Smart_Cb clicked_cb, void *data)
{
	Evas_Object *btn = NULL;

	if (parent == NULL) {
		dlog_print(DLOG_ERROR, LOG_TAG, "parent is NULL.");
		return;
	}

	btn = elm_button_add(parent);
	if (btn == NULL) {
		dlog_print(DLOG_ERROR, LOG_TAG, "failed to create button.");
		return;
	}

	if (style)
		elm_object_style_set(btn, style);

	evas_object_size_hint_weight_set(btn, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_object_part_content_set(parent, part_name, btn);

	if (text)
		elm_object_text_set(btn, text);

	if (image_path)
		view_set_image(btn, NULL, image_path);

	if (down_cb)
		evas_object_event_callback_add(btn, EVAS_CALLBACK_MOUSE_DOWN, down_cb, data);
	if (up_cb)
		evas_object_event_callback_add(btn, EVAS_CALLBACK_MOUSE_UP, up_cb, data);
	if (clicked_cb)
		evas_object_smart_callback_add(btn, "clicked", clicked_cb, data);

	evas_object_show(btn);
}

/*
 * @brief Makes popup with theme.
 * @param[in] parent The object to which you want to set popup
 * @param[in] timeout Timeout in seconds that control timer when the popup is hidden
 * @param[in] text Text will be written on the popup
 */
void view_create_text_popup(Evas_Object *parent, double timeout, const char *text)
{
	Evas_Object *popup = NULL;
	Evas_Object *popup_layout = NULL;

	if (parent == NULL) {
		dlog_print(DLOG_ERROR, LOG_TAG, "parent is NULL.");
		return;
	}

	popup = elm_popup_add(parent);
	elm_object_style_set(popup, "circle");
	evas_object_size_hint_weight_set(popup, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

	eext_object_event_callback_add(popup, EEXT_CALLBACK_BACK, _popup_hide_cb, NULL);
	/*
	 * Delete popup object in _popup_hide_finished_cb(), when the "dismissed" signal will be called.
	 */
	evas_object_smart_callback_add(popup, "dismissed", _popup_hide_finished_cb, NULL);

	popup_layout = elm_layout_add(popup);
	elm_layout_theme_set(popup_layout, "layout", "popup", "content/circle");

	elm_object_content_set(popup, popup_layout);

	elm_popup_timeout_set(popup, timeout);

	if (text) {
		view_set_text(popup_layout, "elm.text", text);
	}

	evas_object_show(popup);
}

/*
 * @brief Creates check box.
 * @param[in] parent Object to which you want to add check
 * @param[in] event Event's name
 * @param[in] cb_func Callback function
 * @param[in] data Data needed in this function
 */
Evas_Object *view_create_checkbox(Evas_Object *parent, const char *event, Evas_Smart_Cb cb_func, void *data)
{
	struct genlist_item_data *gendata = data;
	Evas_Object *check = NULL;
	Eina_Bool check_state = EINA_FALSE;

	check = elm_check_add(parent);
	check_state = gendata->check_state;
	elm_check_state_set(check, check_state);
	evas_object_smart_callback_add(check, event, cb_func, gendata);
	evas_object_show(check);

	return check;
}

/*
 * @brief Sets the content at a part of a given container.
 * @param[in] layout Layout as a container
 * @param[in] part_name The container's part name to set content
 * @param[in] content The new content for that part
 */
void view_set_content_to_part(Evas_Object *layout, const char *part_name, Evas_Object *content)
{
	elm_object_part_content_set(layout, part_name, content);
}

/*
 * @brief Sends a signal to the edje object.
 * @param[in] layout Layout that receives the signal
 * @param[in] signal Signal name
 * @param[in] source Signal source
 */
void view_send_signal_to_edje(Evas_Object *layout, const char *signal, const char *source)
{
	elm_object_signal_emit(layout, signal, source);
}

/*
 * @brief Gets the time that user sets and sets an alarm to be triggered at a specific time.
 * @param[in] gendata Data structure stored information of the alarm.
 */
void view_alarm_schedule_alarm(struct genlist_item_data *gendata)
{
	app_control_h app_control;
	struct tm *saved_time = NULL;
	int alarm_id = 0;

	if (gendata == NULL) {
		dlog_print(DLOG_ERROR, LOG_TAG, "Failed to get gendata.");
		return;
	}

	/*
	 * Get the time that user sets.
	 */
	saved_time = &gendata->saved_time;
	elm_datetime_value_get(s_info.datetime, saved_time);

	/*
	 * Initialize seconds.
	 */
	saved_time->tm_sec = 0;

	dlog_print(DLOG_INFO, LOG_TAG, "saved time (%d:%d)", saved_time->tm_hour, saved_time->tm_min);

	/*
	 * Set alarm by using alarm API.
	 */
	app_control = data_get_app_control();

	if (ALARM_ERROR_NONE != alarm_schedule_at_date(app_control, saved_time, 0, &alarm_id)) {
		dlog_print(DLOG_ERROR, LOG_TAG, "Failed at alarm_schedule_at_date(). Alarm is not set.");
	}
	dlog_print(DLOG_INFO, LOG_TAG, "alarm ID is [%d]", alarm_id);

	/*
	 * Store alarm ID in gendata with the generated alarm ID.
	 */
	gendata->alarm_id = alarm_id;
}


/*
 * @note
 * Below functions are static functions.
 */

/*
 * @brief Sets functions will be operated when this item is shown on the screen according to the style.
 * @param[in] style Style of item
 */
static Elm_Genlist_Item_Class *_set_genlist_item_class(const char *style)
{
	Elm_Genlist_Item_Class *item_class = NULL;

	if (style == NULL) {
		dlog_print(DLOG_ERROR, LOG_TAG, "style is NULL.");
		return NULL;
	}

	item_class = elm_genlist_item_class_new();

	/*
	 * If you want to add the item class of genlist, you should be add below.
	 * To see more genlist's styles click on the link below.
	 * https://developer.tizen.org/development/ui-practices/native-application/efl/ui-components/wearable-ui-components
	 */
	if (!strcmp(style, "title")) {
		/*
		 * This function will be operated when this item is shown on the screen to get the title.
		 */
		item_class->item_style = "title";
		item_class->func.text_get = data_get_title_text;
	} else if (!strcmp(style, "1text")) {
		item_class->item_style = "1text";
	} else if (!strcmp(style, "1text.1icon")) {
		item_class->item_style = "1text.1icon";
	} else if (!strcmp(style, "1text.1icon.1")) {
		item_class->item_style = "1text.1icon.1";
		item_class->func.text_get = data_get_saved_time_text;
		item_class->func.content_get = _get_check_icon;
		item_class->func.del = _del_data;
	} else if (!strcmp(style, "2text")) {
		item_class->item_style = "2text";
	} else if (!strcmp(style, "padding")) {
		/*
		 * "padding" style does nothing.
		 * But it makes genlist's item placed in the middle of the screen.
		 */
	}

	return item_class;
}

/*
 * @brief This function will be operated when window is deleted.
 * @param[in] data Data needed in this function
 * @param[in] obj Smart object
 * @param[in] event_info Information of event
 */
static void _win_delete_request_cb(void *data, Evas_Object *obj, void *event_info)
{
	ui_app_exit();
}

/*
 * @brief This function will be operated when genlist's item is selected.
 * @param[in] data Data needed in this function
 * @param[in] obj Smart object
 * @param[in] event_info Selected item
 */
static void _gl_selected_cb(void *data, Evas_Object *obj, void *event_info)
{
	Elm_Object_Item *it = event_info;

	elm_genlist_item_selected_set(it, EINA_FALSE);
}

/*
 * @brief This function will be operated when items are shown on the screen.
 * @param[in] data Data passed from 'elm_genlist_item_append' as third parameter
 * @param[in] obj Genlist
 * @param[in] part Name string of one of the existing text parts in the EDJ group implementing the item's theme
 */
static Evas_Object* _get_check_icon(void *data, Evas_Object *obj, const char *part)
{
	struct genlist_item_data *gendata = data;
	Evas_Object *content = NULL;

	if (strcmp(part, "elm.icon")) return NULL;

	content = view_create_checkbox(obj, "changed", _icon_clicked_cb, gendata);

	return content;
}

/*
 * @brief This function will be operated when the genlist's item is removed.
 * @param[in] data Data passed from 'elm_genlist_item_append' as third parameter
 * @param[in] obj Genlist
 */
static void _del_data(void *data, Evas_Object *obj)
{
	if (data == NULL) {
		dlog_print(DLOG_ERROR, LOG_TAG, "data is NULL");
		return;
	}

	if (obj == NULL) {
		dlog_print(DLOG_ERROR, LOG_TAG, "genlist is NULL");
		return;
	}

	data_alarm_destroy_genlist_item_data(data);
}

/*
 * @brief This function will be operated when check object is clicked.
 * @param[in] data Data needed in this function
 * @param[in] obj Smart object
 * @param[in] event_info Information of event
 */
static void _icon_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
	struct genlist_item_data *gendata = data;
	struct tm *saved_time = NULL;
	int alarm_id = 0;
	Evas_Object *check = obj;
	char buf[BUF_LEN] = {0, };
	Eina_Bool state = EINA_FALSE;
	Eina_Bool widget_alarm = EINA_FALSE;

	state = elm_check_state_get(check);

	saved_time = &gendata->saved_time;
	alarm_id = gendata->alarm_id;

	widget_alarm = data_check_exist_widget_alarm(gendata);

	if (state == EINA_TRUE) {
		app_control_h app_control;

		/*
		 * Store the current state of check box in gendata.
		 */
		gendata->check_state = state;

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

		if (widget_alarm) {
			alarm_set_widget_on_off("On", gendata);
		}

	} else {
		/*
		 * Store the current state of check box in gendata.
		 */
		gendata->check_state = state;

		/*
		 * Cancels the alarm with the specific alarm ID.
		 */
		alarm_cancel(alarm_id);

		/*
		 * Remove bundle using specific alarm ID as key.
		 */
		snprintf(buf, sizeof(buf), "%d", alarm_id);
		data_delete_bundle(buf);

		if (widget_alarm) {
			alarm_set_widget_on_off("Off", gendata);

		}
	}

	dlog_print(DLOG_INFO, LOG_TAG, "icon state is [%d], alarm ID [%d], saved time (%d:%d)",
			state, alarm_id, saved_time->tm_hour, saved_time->tm_min);
}

/*
 * @brief This function will be operated when the "dismissed" signal will be called.
 * @param[in] data Data needed in this function
 * @param[in] obj Smart object
 * @param[in] event_info Information of event
 */
static void _popup_hide_finished_cb(void *data, Evas_Object *obj, void *event_info)
{
	if (obj == NULL)
		return;

	/*
	 * Delete popup object in this function.
	 */
	evas_object_del(obj);
}

/*
 * @brief This function will be operated when the H/W Back Key event occurred.
 * @param[in] data Data needed in this function
 * @param[in] obj Smart object
 * @param[in] event_info Information of event
 */
static void _popup_hide_cb(void *data, Evas_Object *obj, void *event_info)
{
	if (obj == NULL)
		return;

	/*
	 * Use this function to dismiss the popup with hide effect.
	 * When the popup is dismissed, the "dismissed" signal will be emitted.
	 */
	elm_popup_dismiss(obj);
}

/*
 * @brief This function will be operated when the Back key is pressed.
 * @param[in] data Data has the same value passed to eext_object_event_callback_add() as the data parameter
 * @param[in] obj The evas object on which event occurred
 * @param[in] event_info Event information if the event passes an additional information
 */
static void _naviframe_back_cb(void *data, Evas_Object *obj, void *event_info)
{
	int count = 0;
	Elm_Object_Item *top_item = NULL;
	Elm_Object_Item *bottom_item = NULL;

	/*
	 * There are two genlist items when the alarm list is empty.
	 * (The remaining two genlist items, one is the title and another one is padding.)
	 *  Alarm app hides layout that shows alarm list if alarm list is empty using view_send_signal_to_edje().
	 */
	count = elm_genlist_items_count(s_info.genlist);
	if (count < 3) {
		view_send_signal_to_edje(s_info.layout, "genlist.hide", "alarm");
	}

	top_item = elm_naviframe_top_item_get(obj);
	bottom_item = elm_naviframe_bottom_item_get(obj);

	dlog_print(DLOG_ERROR, LOG_TAG, "%s[%d] top_item(%p), bottom_item(%p)" , __func__, __LINE__, top_item, bottom_item);

	/*
	 * There is no item to pop if the top of the naviframe item is the same as the bottom of the naviframe item.
	 */
	if (top_item == bottom_item) {
		evas_object_hide(s_info.win);
		return;
	}

	elm_naviframe_item_pop(obj);
}
/* End of file */
