#pragma once
#define min
#define max
#include <host/ble_gap.h>
#undef max
#undef min

#define PACKETLEN 2

namespace Pinetime::System {
    class SystemTask;
}

namespace Pinetime::Controllers {
    class NimbleController;
    class Ble;

    class ESPService {
        public:
            ESPService(Pinetime::System::SystemTask& systemTask, Pinetime::Controllers::Ble& bleController, Pinetime::Controllers::NimbleController& nimble);
            void Init();
            int ESPServiceCallback(struct ble_gatt_access_ctxt *ctxt);

            bool isConnected();
            void read(int8_t buf[PACKETLEN]);
            void write(int8_t *buf, int8_t len);
            int8_t lastValueRead[PACKETLEN];
            bool hasValue = false;
            
        private:
            Pinetime::System::SystemTask& systemTask;
            Pinetime::Controllers::Ble& bleController;
            Pinetime::Controllers::NimbleController& nimble;

            // int HandleClientRead(struct ble_gatt_access_ctxt *ctxt);
            int HandleClientWrite(struct ble_gatt_access_ctxt *ctxt);

            int8_t writeBuf[PACKETLEN];
            int8_t readBuf[PACKETLEN];

            static constexpr uint16_t espServiceId {0x181D};
            static constexpr uint16_t espCharId {0x2AFF};

            static constexpr ble_uuid16_t espServiceUuid {.u {.type = BLE_UUID_TYPE_16}, .value = espServiceId};
            static constexpr ble_uuid16_t espCharUuid {.u {.type = BLE_UUID_TYPE_16}, .value = espCharId};
            
            struct ble_gatt_svc_def serviceDefinition[2];
            struct ble_gatt_chr_def characteristicDefinition[2];

            uint16_t eventHandle;
    };
}