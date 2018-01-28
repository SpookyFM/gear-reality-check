/*
 * reality-check.c
 *
 *  Created on: Jan 7, 2018
 *      Author: Florian
 */

#include <tizen_error.h>
#include <time.h>
#include <stdlib.h>
#include <app_alarm.h>
#include <app_preference.h>
#include <haptic.h>
#include <dlog.h>
#include <Ecore.h>

#include "gear-reality-check.h"
#include "reality-check.h"

Eina_Bool alarm_vibrate(void* vp_counter);

const char* num_reminders_key = "num_reminders";
const char* start_time_hours_key = "start_time_hours";
const char* start_time_mins_key = "start_time_mins";
const char* end_time_hours_key = "end_time_hours";
const char* end_time_mins_key = "end_time_mins";
const char* last_handled_date_key = "last_handled_date";

const int num_times_vibrate = 3;
const int vibration_msec = 300;
const int vibration_pause_msec = 300;

typedef struct {
	haptic_device_h device_handle;
	haptic_effect_h effect_handle;
	Ecore_Timer* timer;
	int counter;
} vibration_data_s;



// Plan:
// To get my personal MVP, I will implement the following:
// * Fixed number of alarms
// * Fixed start and end daily
// * Function to generate random alarms for the active part of the day
// * Update function called when the last alarm of the day is called
//
// From then forward, it's features for more usability
// * Configurable number of alarms
// * Configurable start and stop times
// * Flashing display for alarm (to trigger reality checks related to devices such as Aurora)
// * Better vibration feature
// * Better "reality check" screen and dismiss button like the platform alarm app

/** The number of reminders to show per day */
static int get_target_num_reminders(int* num_reminders)
{
	bool exists = false;
	if (preference_is_existing(num_reminders_key, &exists) == PREFERENCE_ERROR_NONE && exists)
	{
		preference_get_int(num_reminders_key, num_reminders);
		dlog_print(DLOG_INFO, LOG_TAG, "Preferred number of reminders: %d ", *num_reminders);
	} else
	{
		*num_reminders = 5;
	}
	return TIZEN_ERROR_NONE;
}

/** The time of day before which no reality checks should be triggered. Only hours and minutes will be used. */
static int get_start_time(struct tm* result)
{
	//@@TODO: Refactor to use preferences
	result->tm_hour = 8;
	result->tm_min = 0;
	return TIZEN_ERROR_NONE;
}

/** The time of day after which no reality checks should be triggered. Only hours and minutes will be used. */
static int get_stop_time(struct tm* result)
{
	//@@TODO: Refactor to use preferences
	result->tm_hour = 22;
	result->tm_min = 0;

	return TIZEN_ERROR_NONE;
}

/** Get a random number between min and max */
static int rand_between(int min, int max)
{
	int limit = max - min;
	return rand() % limit + min;
}

/** Generate a random time on the specified date, after the time specified in start, before the time specified in end */
static int generate_time(struct tm date, struct tm start, struct tm end, struct tm* result)
{
	struct tm start_date = date;
	start_date.tm_hour = start.tm_hour;
	start_date.tm_min = start.tm_min;

	struct tm end_date = date;
	end_date.tm_hour = end.tm_hour;
	end_date.tm_min = end.tm_min;

	time_t start_time_t = mktime(&start_date);
	time_t end_time_t = mktime(&end_date);

	time_t rand_time_t = rand_between(start_time_t, end_time_t);

	localtime_r (&rand_time_t,result);

	return TIZEN_ERROR_NONE;
}

/** Generate the specified number of alarms on the given date */
static int generate_times(struct tm date, int num_times, struct tm** result)
{
	srand( (unsigned)time( NULL ) );
	// Initialize the array
	*result = malloc(sizeof(struct tm) * num_times);
	struct tm* current = *result;

	struct tm start_time;
	struct tm end_time;
	get_start_time(&start_time);
	get_stop_time(&end_time);

	for (int i = 0; i < num_times;i++)
	{
		generate_time(date, start_time, end_time, current);
		current++;
	}

	return TIZEN_ERROR_NONE;
}

/** Schedule alarms on the given dates */
static int schedule_alarms(app_control_h app_control, int num_alarms, struct tm* alarms)
{
	struct tm* current = alarms;
	int ret;
	for (int i = 0; i < num_alarms; i++)
	{
		int alarm_id;
		ret = alarm_schedule_at_date(app_control, current, 0, &alarm_id);
		if (ret != ALARM_ERROR_NONE)
		        dlog_print(DLOG_ERROR, LOG_TAG, "Get time Error: %d ", ret);
		dlog_print(DLOG_INFO, LOG_TAG, "New alarm scheduled at: %s ", asctime(current));
		current++;
	}

	// @@TODO: Better error handling
	return TIZEN_ERROR_NONE;
}

/** Save all registered alarms into an array of tm values. */
static bool on_foreach_registered_alarm(int alarm_id, void *user_data)
{
    int ret = 0;
    struct tm date;

    ret = alarm_get_scheduled_date(alarm_id, &date);
    if (ret != ALARM_ERROR_NONE)
        dlog_print(DLOG_ERROR, LOG_TAG, "Get time Error: %d ", ret);

    struct tm** array = (struct tm**)(user_data);
    // Write the current date into the array
    **array = date;
    // And point to the next element
    *array += 1;

    return true;
}

/** Function to count the number of registered alarms. user_data is an int*, must be set to 0 before */
static bool on_foreach_registered_alarm_count(int alarm_id, void *user_data)
{
	int* count = (int*) user_data;
	*count += 1;

    return true;
}


/** Get all times at which the app already has alarms set */
static int get_all_alarms(int* num_alarms, struct tm** result)
{
	// First, count the numbers of alarms by iterating once over them
	*num_alarms = 0;
	int ret = 0;
	ret = alarm_foreach_registered_alarm(on_foreach_registered_alarm_count, (void*) num_alarms);
	if (ret != ALARM_ERROR_NONE)
	    dlog_print(DLOG_ERROR, LOG_TAG, "Listing Error: %d ", ret);

	// Allocate an array large enough
	*result = malloc(sizeof(struct tm) * *num_alarms);

	struct tm* first_element = *result;
	struct tm** array_copy = &first_element;

	// And fill the array
	ret = alarm_foreach_registered_alarm(on_foreach_registered_alarm, (void*) array_copy);
	if (ret != ALARM_ERROR_NONE)
	    dlog_print(DLOG_ERROR, LOG_TAG, "Listing Error: %d ", ret);

	return TIZEN_ERROR_NONE;
}

/** Checks whether the two dates are the same day
 *
 */
static bool is_same_day(struct tm date, struct tm reference_date)
{
	return date.tm_yday == reference_date.tm_yday &&
			date.tm_mon == reference_date.tm_mon &&
			date.tm_year == reference_date.tm_year;
}

/** Retrieves the number of alarms the app has already scheduled on the specified date */
static int get_num_alarms_date(struct tm date, int* result)
{
	int num_alarms = 0;
	struct tm* array = NULL;
	get_all_alarms(&num_alarms, &array);
	struct tm* current = array;
	for (int i = 0; i < num_alarms; i++)
	{
		if (is_same_day(*current, date))
		{
			*result += 1;
		}
		current++;
	}
	free(array);
	return TIZEN_ERROR_NONE;
}

/** Gets tomorrow's date */
static int get_tomorrow(struct tm* result)
{
	int ret;
	ret = alarm_get_current_time(result);

	// Convert to Unix timestamp
	time_t result_t = mktime(result);

	// Add one day's equivalent in seconds
	result_t += 24 * 60 * 60;

	// And convert back
	localtime_r(&result_t, result);

	return TIZEN_ERROR_NONE;
}

/** Function for updating all alarms. Will check if the alarms for tomorrow are not set up correctly and, if so, set them up */
int update_alarms(app_control_h app_control)
{
	dlog_print(DLOG_INFO, LOG_TAG, "Updating alarms.");
	struct tm tomorrow;
	int ret = get_tomorrow(&tomorrow);

	// Check out if we have enough alarms scheduled for tomorrow
	int num_alarms_tomorrow = 0;
	get_num_alarms_date(tomorrow, &num_alarms_tomorrow);
	int target_num_alarms_tomorrow = 0;
	get_target_num_reminders(&target_num_alarms_tomorrow);
	if (num_alarms_tomorrow != target_num_alarms_tomorrow)
	{
		dlog_print(DLOG_INFO, LOG_TAG, "Wrong number of alarms scheduled for tomorrow, %d instead of %d.", num_alarms_tomorrow, target_num_alarms_tomorrow);
		// We need to schedule alarms for tomorrow
		struct tm* generated_times;
		generate_times(tomorrow, target_num_alarms_tomorrow, &generated_times);

		schedule_alarms(app_control, target_num_alarms_tomorrow, generated_times);
		free(generated_times);
	} else
	{
		dlog_print(DLOG_INFO, LOG_TAG, "Correct number of alarms scheduled for tomorrow (%d).", num_alarms_tomorrow);
	}

	// For debug purposes, also schedule today
	struct tm today;
	ret = alarm_get_current_time(&today);

	// Check out if we have enough alarms scheduled for today
	int num_alarms_today;
	get_num_alarms_date(today, &num_alarms_today);
	int target_num_alarms_today = 0;
	get_target_num_reminders(&target_num_alarms_today);
	if (num_alarms_tomorrow != target_num_alarms_today)
	{
		dlog_print(DLOG_INFO, LOG_TAG, "Wrong number of alarms scheduled for today, %d instead of %d.", num_alarms_today, target_num_alarms_today);
		// We need to schedule alarms for today
		struct tm* generated_times;
		generate_times(today, target_num_alarms_today, &generated_times);

		schedule_alarms(app_control, target_num_alarms_today, generated_times);
		free(generated_times);
	} else
	{
		dlog_print(DLOG_INFO, LOG_TAG, "Correct number of alarms scheduled for today (%d).", num_alarms_today);
	}



	// For testing purposes, schedule one in a few seconds
	bool debug_alarms = false;

	if (debug_alarms)
	{
		int alarm_id;
		struct tm soon;
		ret = alarm_get_current_time(&soon);
		soon.tm_sec += 20;
		ret = alarm_schedule_at_date(app_control, &soon, 0, &alarm_id);
	}


	return TIZEN_ERROR_NONE;
}


/*
 * @brief Starts the vibration pattern for the alarm
 * The parameter counter is incr
 */
void start_alarm_vibrate()
{
	// Prepare the vibration by opening the device
	// Check how many vibrators we have
	int device_haptic_count = 0;
	int ret = 0;
	ret = device_haptic_get_count(&device_haptic_count);

	if (ret != DEVICE_ERROR_NONE || device_haptic_count == 0)
	{
		dlog_print(DLOG_INFO, LOG_TAG, "Error or no haptic devices found.");
		// Nothing to do in case of error or no haptic devices present
		return;
	}

	vibration_data_s* vibration_data = malloc(sizeof(vibration_data_s));

	// Open the first vibrator
	ret = device_haptic_open(0, &vibration_data->device_handle);
	if (ret != DEVICE_ERROR_NONE)
	{
		dlog_print(DLOG_INFO, LOG_TAG, "Error opening haptic device.");
		free(vibration_data);
		return;
	}

	vibration_data->counter = 0;

	// Start the first vibration
	alarm_vibrate(vibration_data);
}


/*
 * @brief Callback for the alarm vibration.
 */
Eina_Bool alarm_vibrate(void* vp_vibration_data)
{
	vibration_data_s* vibration_data = (vibration_data_s*) vp_vibration_data;
	dlog_print(DLOG_INFO, LOG_TAG, "Entered alarm_vibrate, counter: %d", vibration_data->counter);
	if (vibration_data->counter < num_times_vibrate)
	{
		// Start vibrating
		int ret = device_haptic_vibrate(vibration_data->device_handle, vibration_msec, 100, &vibration_data->effect_handle);
		if (ret != DEVICE_ERROR_NONE)
		{
			dlog_print(DLOG_ERROR, LOG_TAG, "Error starting vibration.");
			return EINA_FALSE;
		}

		if (vibration_data->counter == 0)
		{
			dlog_print(DLOG_INFO, LOG_TAG, "Starting timer for %d msec", vibration_msec + vibration_pause_msec);
			// If this is the first time, we also need to set up the timer
			vibration_data->timer = ecore_timer_add(((double)(vibration_msec + vibration_pause_msec)) / 1000.0 , alarm_vibrate, vibration_data);
			if (!vibration_data->timer)
			{
				dlog_print(DLOG_INFO, LOG_TAG, "Error starting timer.");
			}
		}
	} else
	{
		// We need to stop vibrating
		int ret = device_haptic_close(vibration_data->device_handle);
		if (ret != DEVICE_ERROR_NONE)
		{
			dlog_print(DLOG_INFO, LOG_TAG, "Error closing haptic device.");
			return EINA_FALSE;
		}
		free(vibration_data);
		return EINA_FALSE;
	}

	vibration_data->counter += 1;
	return EINA_TRUE;
}


 void test()
{
	/* struct tm today;
	alarm_get_current_time(&today);
	int num_times = 0;
	get_target_num_reminders(&num_times);
	struct tm* result;
	generate_times(today, num_times, &result);

	struct tm* current = result;
	for (int i = 0; i < num_times; i++)
	{
		int hours = current->tm_hour;
		int minutes = current->tm_min;
		current++;
	}

	free(result); */
}


