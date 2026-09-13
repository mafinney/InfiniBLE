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
		
	} else if (obj == windows.button) {
		
	}
}

void CarControl::Refresh() {
	esp.read(buf, (uint8_t) MAX_PACKET_LEN); // safe cast, MAX_PACKET_LEN fits in a byte
	if (buf[0] == CHECK_AUTH && hash_sent == false) {
		// copy nonce to a local variable
		uint8_t nonce[16];
		memcpy(nonce, buf + 1, 16);

		// compute hash
		uint8_t hash[32];
		ComputeHash(key, nonce, hash);

		// send a CHECK_AUTH_RESP with the computed hash to the client
		WritePacket(CHECK_AUTH_RESP, hash);
		hash_sent = true;
	}

	if (esp.isConnected()) {
		lv_obj_set_style_local_text_color(connected, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_GREEN);
	} else {
		lv_obj_set_style_local_text_color(connected, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_RED);
	}
}

CarControl::CarControl(Pinetime::Controllers::ESPService& espService) : esp {espService} {
	CreateLabel(&car_name, car_screen, SMALL_BUTTON_W, SMALL_BUTTON_H, LV_ALIGN_IN_TOP_MID, 0, 0, (char *) "WRX3");
	CreateLabel(&connected, car_screen, SMALL_BUTTON_W, SMALL_BUTTON_H, LV_ALIGN_IN_TOP_RIGHT, 0, 0, (char *) Symbols::bluetooth);
	CreateButton(&doors, car_screen, ButtonEvent, SMALL_BUTTON_W, SMALL_BUTTON_H, LV_ALIGN_IN_LEFT_MID, 0, 0, (char *) "DOORS");
	CreateButton(&windows, car_screen, ButtonEvent, SMALL_BUTTON_W, SMALL_BUTTON_H, LV_ALIGN_IN_RIGHT_MID, 0, 0, (char *) "WINDOWS");
	CreateSwitch(&auto_switch, car_screen, ButtonEvent, SMALL_BUTTON_W, SMALL_BUTTON_H, LV_ALIGN_IN_TOP_LEFT, 0, 0);

	lv_obj_set_style_local_text_color(connected, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_GRAY);
	lv_obj_set_style_local_text_color(doors.label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_GRAY);
	lv_obj_set_style_local_text_color(windows.label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_GRAY);

	refresh_task = lv_task_create(RefreshTaskCallback, LV_DISP_DEF_REFR_PERIOD, LV_TASK_PRIO_MID, this);
	lv_scr_load(car_screen);

	WritePacket(READY_TO_AUTH, NULL);
	hash_sent = false;
}

CarControl::~CarControl() {
	lv_task_del(refresh_task);
  	lv_obj_clean(lv_scr_act());
}

void CarControl::WritePacket(PacketType packetType, uint8_t *data) {
	uint8_t packet[MAX_PACKET_LEN];
	uint8_t packetLen;
	packet[0] = packetType;
	packetLen = 1;

	switch (packetType) {
		case PacketType::READY_TO_AUTH:
			// there is no data to append, do nothing
			break;
		case PacketType::CHECK_AUTH:
			// the watch shouldn't send this
			return ;
		case PacketType::CHECK_AUTH_RESP:
			// send the generated hash (32 bytes)
			memcpy(packet + packetLen, data, 32); // copy the hash to the end of the packet
			packetLen += 32;
			break;
		case PacketType::AUTH_OK:
			// the watch shouldn't send this
			break;
		case PacketType::AUTH_FAILED:
			// the watch shouldn't send this
			break;
		case PacketType::COMMAND:
			// send the single byte command
			break;
		case PacketType::UPDATE:
			// the watch shouldn't send this
			break;
	}
	esp.write(packet, packetLen);
}

void CarControl::ComputeHash(uint8_t key[16], uint8_t nonce[16], uint8_t hash[32]) {
	uint8_t input[32];
	memcpy(input, key, 16);
	memcpy(input + 16, nonce, 16);

	struct tc_sha256_state_struct sha_ctx;
	tc_sha256_init(&sha_ctx);

	// tc_sha256_update(&sha_ctx, key, 16);
	// tc_sha256_update(&sha_ctx, nonce, 16);

	tc_sha256_update(&sha_ctx, input, 32);
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