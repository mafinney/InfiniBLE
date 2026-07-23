#pragma once

#include "systemtask/SystemTask.h"
#include "displayapp/apps/Apps.h"
#include "displayapp/screens/Screen.h"
#include "displayapp/Controllers.h"
#include "Symbols.h"

#include <tinycrypt/sha256.h>

enum PacketType {
	READY_TO_AUTH,
	CHECK_AUTH,
	CHECK_AUTH_RESP,
	AUTH_OK,
	AUTH_FAILED,
	UPDATE
};

namespace Pinetime::Controllers {
	class ESPService;
}

namespace Pinetime::Applications {
    namespace Screens {
        class CarControl : public Screen {
			public:
				CarControl(Pinetime::Controllers::ESPService& espService);
				~CarControl() override;
				void OnButtonEvent(lv_obj_t *obj, lv_event_t event);
				void Refresh() override;

			private:
				Controllers::ESPService& esp;
				uint8_t buf[MAX_PACKET_LEN]; // used to store raw read data from ESPService
				uint8_t key[16] = {0x22, 0x38, 0x9d, 0x03, 0xbf, 0x8c, 0xb7, 0x3d, 0x02, 0xc9, 0xfd, 0xf7, 0x67, 0xab, 0x69, 0x8b};

				static constexpr uint8_t MEDIUM_BUTTON_W = 115;
            	static constexpr uint8_t MEDIUM_BUTTON_H = 80;
            	static constexpr uint8_t SMALL_BUTTON_W = 80;
            	static constexpr uint8_t SMALL_BUTTON_H = 50;

				// [0] is auto, [1] is doors, [2] is windows
				bool status[3]; // stores the last read status from the car

				// Each button is made of a label and a button
				struct button {
					lv_obj_t *label;
					lv_obj_t *button;
				};

				lv_obj_t *car_screen = lv_obj_create(NULL, NULL);
				lv_obj_t *car_name;
				lv_obj_t *connected;
				button doors;
				button windows;
				lv_obj_t *auto_switch;

				// This task will update the screens value
				lv_task_t *refresh_task;

				/**
				 * SendPacket is given a packet type and any values that go with it and sends that packet to the car
				 */
				void SendPacket(PacketType packetType, uint8_t *arg);

				/**
				 * ComputeHash computes the hash using the shared key and the given random value
				 * Takes a key
				 * Takes an nonce value
				 * Takes an array to write the computed hash too
				 */
				void ComputeHash(uint8_t key[16], uint8_t nonce[16], uint8_t output[33]);

				/**
            	 * CreateButton is a wrapper function for all the calls needed to create a button struct object
            	 * Takes a pointer to a button
            	 * A parent screen
            	 * A function callback
            	 * A size
            	 * An alignment
            	 * An initial label
            	 */
            	void CreateButton(button *b, lv_obj_t *par, lv_event_cb_t event_cb, uint8_t w, uint8_t h, lv_align_t align, lv_coord_t x_ofs, lv_coord_t y_ofs, char *text);

            	/**
            	 * CreateLabel is a wrapper function for all the calls needed to create a label
            	 * Takes a pointer to a label
            	 * A parent screen
            	 * A size
            	 * An alignment
            	 * An initial label
            	 */
            	void CreateLabel(lv_obj_t **l, lv_obj_t *par, uint8_t w, uint8_t h, lv_align_t align, lv_coord_t x_ofs, lv_coord_t y_ofs, char *text);

				/**
				 * CreateSwitch is a wrapper function for all the calles needed to create a switch
				 */
				void CreateSwitch(lv_obj_t **s, lv_obj_t *par, lv_event_cb_t event_cb, uint8_t w, uint8_t h, lv_align_t align, lv_coord_t x_ofs, lv_coord_t y_ofs);
        };
    }

	template <>
	struct AppTraits<Apps::CarControl> {
		static constexpr Apps app = Apps::CarControl;
		static constexpr const char *icon = Screens::Symbols::bluetooth;

		static Screens::Screen *Create(AppControllers& controllers) {
			auto& esp = controllers.systemTask->nimble().esp();
			return new Screens::CarControl(esp);
		}
		
		static bool IsAvailable(Pinetime::Controllers::FS &) {
			return true;
		}
	};
}