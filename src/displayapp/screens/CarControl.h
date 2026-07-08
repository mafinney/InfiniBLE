#pragma once

#include "systemtask/SystemTask.h"
#include "displayapp/apps/Apps.h"
#include "displayapp/screens/Screen.h"
#include "displayapp/Controllers.h"
#include "Symbols.h"

#define UNKNOWN -1
#define LOCKED 0 // Up, for windows
#define UNLOCKED 1 // Down, for windows

enum Command {
	LOCKDOORS,
	UNLOCKDOORS,
	ROLLUPWINDOWS,
	ROLLDOWNWINDOWS
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
				int8_t buf[PACKETLEN];

				static constexpr uint8_t MEDIUM_BUTTON_W = 115;
            	static constexpr uint8_t MEDIUM_BUTTON_H = 80;
            	static constexpr uint8_t SMALL_BUTTON_W = 80;
            	static constexpr uint8_t SMALL_BUTTON_H = 50;

				// [0] is connected, [1] is doors, [2] is windows, [3] is auto. Uses defines above
				int8_t status[3];

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
				 * GetDoorStatus requests the door status from the car
				 * Returns either LOCKED, UNLOCKED, or UNKNOWN
				 */
				int8_t GetDoorStatus();

				/**
				 * GetWindowStatus requests the window status from the car
				 * Returns either LOCKED, UNLOCKED, or UNKNOWN
				 */
				int8_t GetWindowStatus();

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