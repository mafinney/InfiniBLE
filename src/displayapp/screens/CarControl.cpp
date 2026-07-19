#include "displayapp/screens/CarControl.h"
#include "components/ble/ESPService.h"
#include "Symbols.h"

using namespace Pinetime::Applications::Screens;

static void ButtonEvent(lv_obj_t *obj, lv_event_t event) {
    auto *screen = static_cast<CarControl*>(obj->user_data);
    screen->OnButtonEvent(obj, event);
}

void CarControl::OnButtonEvent(lv_obj_t *obj, lv_event_t event) {
    if (event != LV_EVENT_CLICKED) {
        return ;
	}
	if (obj == doors.button) {
		if (status[1] == LOCKED) {
			buf[0] = UNLOCKDOORS;
		} else if (status[1] == UNLOCKED) {
			buf[0] = LOCKDOORS;
		}
		esp.write(buf, 1);
	} else if (obj == windows.button) {
		if (status[2] == LOCKED) {
			buf[0] = ROLLDOWNWINDOWS;
		} else if (status[2] == UNLOCKED) {
			buf[0] = ROLLUPWINDOWS;
		}
		esp.write(buf, 1);
	}
}

void CarControl::Refresh() {
	esp.read(buf, 17);
	if (buf[0] == CHECK_HASH) {
		check_hash();
	}

	// Figure out if we are still connected
	status[0] = esp.isConnected();

	// Figure out the status of the doors and windows
	status[1] = GetDoorStatus();
	status[2] = GetWindowStatus();

	// Update the connected status label
	switch (status[0]) {
		case UNKNOWN:
			lv_obj_set_style_local_text_color(connected, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_GRAY);
			break;
		case LOCKED:
			lv_obj_set_style_local_text_color(connected, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_RED);
			break;
		case UNLOCKED:
			lv_obj_set_style_local_text_color(connected, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_GREEN);
			break;
	}

	// Update the doors status label
	switch (status[1]) {
		case UNKNOWN:
			lv_obj_set_style_local_text_color(doors.label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_GRAY);
			break;
		case LOCKED:
			lv_obj_set_style_local_text_color(doors.label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_RED);
			break;
		case UNLOCKED:
			lv_obj_set_style_local_text_color(doors.label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_GREEN);
			break;
	}

	// Update the windows status label
	switch (status[2]) {
		case UNKNOWN:
			lv_obj_set_style_local_text_color(windows.label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_GRAY);
			break;
		case LOCKED:
			lv_obj_set_style_local_text_color(windows.label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_RED);
			break;
		case UNLOCKED:
			lv_obj_set_style_local_text_color(windows.label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_GREEN);
			break;
	}
}

CarControl::CarControl(Pinetime::Controllers::ESPService& espService) : esp {espService} {
	status[0] = UNKNOWN;
	status[1] = UNKNOWN;
	status[2] = UNKNOWN;

	CreateLabel(&car_name, car_screen, SMALL_BUTTON_W, SMALL_BUTTON_H, LV_ALIGN_IN_TOP_MID, 0, 0, (char *) "WRX");
	CreateLabel(&connected, car_screen, SMALL_BUTTON_W, SMALL_BUTTON_H, LV_ALIGN_IN_TOP_RIGHT, 0, 0, (char *) Symbols::bluetooth);
	CreateButton(&doors, car_screen, ButtonEvent, SMALL_BUTTON_W, SMALL_BUTTON_H, LV_ALIGN_IN_LEFT_MID, 0, 0, (char *) "DOORS");
	CreateButton(&windows, car_screen, ButtonEvent, SMALL_BUTTON_W, SMALL_BUTTON_H, LV_ALIGN_IN_RIGHT_MID, 0, 0, (char *) "WINDOWS");
	CreateSwitch(&auto_switch, car_screen, ButtonEvent, SMALL_BUTTON_W, SMALL_BUTTON_H, LV_ALIGN_IN_TOP_LEFT, 0, 0);

	lv_obj_set_style_local_text_color(connected, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_GRAY);
	lv_obj_set_style_local_text_color(doors.label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_GRAY);
	lv_obj_set_style_local_text_color(windows.label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_GRAY);

	refresh_task = lv_task_create(RefreshTaskCallback, LV_DISP_DEF_REFR_PERIOD, LV_TASK_PRIO_MID, this);
	lv_scr_load(car_screen);
}

CarControl::~CarControl() {
	lv_task_del(refresh_task);
  	lv_obj_clean(lv_scr_act());
}

int8_t CarControl::GetDoorStatus() {
	esp.read(buf, 2);
	// if (buf[0] != LOCKED || buf[0] != UNLOCKED) {
	// 	return UNKNOWN;
	// }
	return buf[0];
}

int8_t CarControl::GetWindowStatus() {
	esp.read(buf, 2);
	// if (buf[1] != LOCKED || buf[1] != UNLOCKED) {
	// 	return UNKNOWN;
	// }
	return buf[1];
}

void CarControl::check_hash() {
	uint8_t nonce[16];
	for (int i = 0; i < 16; i++) {
		nonce[i] = buf[i + 1];
	}
	uint8_t input[32];
	memcpy(input, key, 16);
	memcpy(input + 16, nonce, 16);
	uint8_t hash[32];

	struct tc_sha256_state_struct sha_ctx;
	tc_sha256_init(&sha_ctx);
	tc_sha256_update(&sha_ctx, input, sizeof(input));
	tc_sha256_final(hash, &sha_ctx);

	uint8_t output[33];
	output[0] = CHECK_HASH_RESP;
	for (int i = 0; i < 32; i++) {
		output[i + 1] = hash[i];
	}

	esp.write((int8_t *) output, 33);
}

void CarControl::CreateButton(button *b, lv_obj_t *par, lv_event_cb_t event_cb, uint8_t w, uint8_t h, lv_align_t align, lv_coord_t x_ofs, lv_coord_t y_ofs, char *text) {
    b->button = lv_btn_create(par, nullptr);
    b->button->user_data = this;
    lv_obj_set_event_cb(b->button, event_cb);
    lv_obj_set_size(b->button, w, h);
    lv_obj_align(b->button, par, align, x_ofs, y_ofs);
    b->label = lv_label_create(b->button, nullptr);
    lv_label_set_text_static(b->label, text);
}

void CarControl::CreateLabel(lv_obj_t **l, lv_obj_t *par, uint8_t w, uint8_t h, lv_align_t align, lv_coord_t x_ofs, lv_coord_t y_ofs, char *text) {
    *l = lv_label_create(par, nullptr);
    lv_obj_set_size(*l, w, h);
    lv_obj_align(*l, par, align, x_ofs, y_ofs);
    lv_label_set_text_static(*l, text);
}

void CarControl::CreateSwitch(lv_obj_t **s, lv_obj_t *par, lv_event_cb_t event_cb, uint8_t w, uint8_t h, lv_align_t align, lv_coord_t x_ofs, lv_coord_t y_ofs) {
	*s = lv_switch_create(par, nullptr);
	(*s)->user_data = this;
	lv_obj_set_event_cb(*s, event_cb);
	lv_obj_set_size(*s, w, h);
	lv_obj_align(*s, par, align, x_ofs, y_ofs);
}