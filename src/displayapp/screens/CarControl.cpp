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
		buf[0] = status[0];
		buf[1] = !status[1];
		buf[2] = status[2];
		SendPacket(PacketType::UPDATE, buf);
	} else if (obj == windows.button) {
		buf[0] = status[0];
		buf[1] = status[1];
		buf[2] = !status[2];
		SendPacket(PacketType::UPDATE, buf);
	}
}

void CarControl::Refresh() {
	// Read the packet type
	esp.read(buf, 1);

	switch (buf[0]) {
		case PacketType::READY_TO_AUTH:
			// shouldn't happen
			break;
		case PacketType::CHECK_AUTH:
			// Setup variables for hash computation
			uint8_t nonce[16];
			uint8_t hash[32];

			// Read the nonce, compute hash and send the packet
			esp.read(nonce, 16);
			ComputeHash(key, nonce, hash);
			SendPacket(CHECK_AUTH_RESP, hash);
			break;
		case PacketType::CHECK_AUTH_RESP:
			// shouldn't happen
			break;
		case PacketType::AUTH_OK:
			// do nothing
			break;
		case PacketType::AUTH_FAILED:
			// do nothing
			break;
		case PacketType::UPDATE:
			esp.read(buf, 3);
			status[0] = buf[0];
			status[1] = buf[1];
			status[2] = buf[2];
			break;
		default:
			// shouldn't happen
			break;
	}

	switch (esp.isConnected()) {
		case false:
			lv_obj_set_style_local_text_color(connected, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_RED);
			break;
		case true:
			lv_obj_set_style_local_text_color(connected, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_GREEN);
			break;
	}

	// Update the doors status label
	switch (status[1]) {
		case false:
			lv_obj_set_style_local_text_color(doors.label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_RED);
			break;
		case true:
			lv_obj_set_style_local_text_color(doors.label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_GREEN);
			break;
	}

	// Update the windows status label
	switch (status[2]) {
		case false:
			lv_obj_set_style_local_text_color(windows.label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_RED);
			break;
		case true:
			lv_obj_set_style_local_text_color(windows.label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_GREEN);
			break;
	}
}

CarControl::CarControl(Pinetime::Controllers::ESPService& espService) : esp {espService} {
	// Setup screen and refresh task
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

	// Tell the car we are ready to authorize now
	SendPacket(PacketType::READY_TO_AUTH, NULL);
}

CarControl::~CarControl() {
	lv_task_del(refresh_task);
  	lv_obj_clean(lv_scr_act());
}

void CarControl::SendPacket(PacketType packetType, uint8_t *arg) {
	uint8_t packet[MAX_PACKET_LEN];
	packet[0] = packetType;
	uint8_t packetLen = 1;

	switch (packetType) {
		case PacketType::READY_TO_AUTH:
			// do nothing
			break;
		case PacketType::CHECK_AUTH:
			// should never send CHECK_AUTH to the car
			break;
		case PacketType::CHECK_AUTH_RESP:
			// copy argOne into hash
			for (int i = 0; i < 32; i++) {
				packet[i + packetLen] = arg[i];
			}
			packetLen += 32;
			break;
		case PacketType::AUTH_OK:
			// should never send AUTH_OK to the car
			break;
		case PacketType::AUTH_FAILED:
			// should never send AUTH_FAILED to the car
			break;
		case PacketType::UPDATE:
			packet[1] = arg[0];
			packet[2] = arg[1];
			packet[3] = arg[2];
			packetLen += 3;
			break;
		default:
			// shouldn't happen, dont write anything
			return ;
	}

	esp.write(packet, packetLen);
}

void CarControl::ComputeHash(uint8_t key[16], uint8_t nonce[16], uint8_t hash[32]) {
	uint8_t input[32];
	memcpy(input, key, 16);
	memcpy(input + 16, nonce, 16);

	struct tc_sha256_state_struct sha_ctx;
	tc_sha256_init(&sha_ctx);
	tc_sha256_update(&sha_ctx, input, sizeof(input));
	tc_sha256_final(hash, &sha_ctx);
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