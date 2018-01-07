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

#if !defined(_VIEW_H)
#define _VIEW_H

Evas_Object *view_get_window(void);
Evas_Object *view_get_base_layout(void);
void view_set_base_layout(Evas_Object *layout);
Evas_Object *view_get_genlist(void);
void view_set_genlist(Evas_Object *genlist);
Evas_Object *view_get_naviframe(void);

Eina_Bool view_create(void);
void view_alarm_create(void);

Evas_Object *view_create_win(const char *pkg_name);
Evas_Object *view_create_conformant_without_indicator(Evas_Object *win);
Evas_Object *view_create_layout(Evas_Object *parent, const char *file_path, const char *group_name, Eext_Event_Cb cb_function, void *user_data);
Evas_Object *view_create_layout_by_theme(Evas_Object *parent, const char *classname, const char *group, const char *style);
Evas_Object *view_create_datetime(Evas_Object *parent, const char *style);

void view_destroy(void);
void view_alarm_destroy(void);

void view_set_image(Evas_Object *parent, const char *part_name, const char *image_path);
void view_set_text(Evas_Object *parent, const char *part_name, const char *text);

Evas_Object *view_create_naviframe(Evas_Object *parent);
Elm_Object_Item *view_push_item_to_naviframe(Evas_Object *nf, Evas_Object *genlist, Elm_Naviframe_Item_Pop_Cb _pop_cb, void *cb_data);

Evas_Object *view_create_circle_genlist(Evas_Object *parent);
Elm_Object_Item *view_append_item_to_genlist(Evas_Object *genlist, const char *style, const void *data, Evas_Smart_Cb _clicked_cb, const void *cb_data);
Elm_Object_Item *view_alarm_find_item_from_genlist(Evas_Object *genlist, int val);

void view_set_button(Evas_Object *parent, const char *part_name, const char *style, const char *image_path, const char *text,
		Evas_Object_Event_Cb down_cb, Evas_Object_Event_Cb up_cb, Evas_Smart_Cb clicked_cb, void *data);

void view_create_text_popup(Evas_Object *parent, double timeout, const char *text);
Evas_Object *view_create_checkbox(Evas_Object *parent, const char *event, Evas_Smart_Cb func, void *data);

void view_set_content_to_part(Evas_Object *layout, const char *part_name, Evas_Object *content);
void view_send_signal_to_edje(Evas_Object *layout, const char *signal, const char *source);

void view_alarm_schedule_alarm(struct genlist_item_data *gendata);

#endif
