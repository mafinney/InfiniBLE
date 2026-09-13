#include "components/ble/ESPService.h"
#include "systemtask/SystemTask.h"
#include "components/ble/BleController.h"

namespace Pinetime::Controllers {
    constexpr ble_uuid16_t ESPService::espServiceUuid;
    constexpr ble_uuid16_t ESPService::espCharUuid;

    int Callback(uint16_t /*conn_handle*/, uint16_t /*attr_handle*/, struct ble_gatt_access_ctxt *ctxt, void *arg) {
        auto espService = static_cast<ESPService*>(arg);

        return espService->ESPServiceCallback(ctxt);
    }

    ESPService::ESPService(Pinetime::System::SystemTask& systemTask, Pinetime::Controllers::Ble& bleController, Pinetime::Controllers::NimbleController& nimble) : systemTask {systemTask}, bleController {bleController}, nimble {nimble} {
        characteristicDefinition[0] = {
            .uuid = &espCharUuid.u,
            .access_cb = Callback,
            .arg = this,
            .flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE_NO_RSP | BLE_GATT_CHR_F_NOTIFY,
            .val_handle = &eventHandle
        };
        characteristicDefinition[1] = {0};

        serviceDefinition[0] = {
            .type = BLE_GATT_SVC_TYPE_PRIMARY,
            .uuid = &espServiceUuid.u,
            .characteristics = characteristicDefinition
        };
        serviceDefinition[1] = {0};
    }

    void ESPService::Init() {
        int res;
        res = ble_gatts_count_cfg(serviceDefinition);
        ASSERT(res == 0);

        res = ble_gatts_add_svcs(serviceDefinition);
        ASSERT(res == 0);
    }

    bool ESPService::isConnected() {
        return bleController.IsConnected();
    }

    void ESPService::read(uint8_t *buf, uint8_t len) {
        // User level apps can call this to read what was last written to this characteristic by the client
        for (int i = 0; i < len; i++) {
            buf[i] = readBuf[i];
        }
    }

    void ESPService::write(uint8_t *buf, uint8_t len) {        
        // This write notifies the client of a write. It also caches whatever was written last in the service (for HandleClientRead)
        memcpy(writeBuf, buf, len);
        writeLen = len;

        auto *om = ble_hs_mbuf_from_flat(buf, len);

        uint16_t connectionHandle = nimble.connHandle();
        if (connectionHandle == 0 || connectionHandle == BLE_HS_CONN_HANDLE_NONE) {
            return ;
        }

        ble_gattc_notify_custom(connectionHandle, eventHandle, om);
    }

    int ESPService::ESPServiceCallback(struct ble_gatt_access_ctxt *ctxt) {
        switch (ctxt->op) {
            case BLE_GATT_ACCESS_OP_READ_CHR:
                return HandleClientRead(ctxt);
            case BLE_GATT_ACCESS_OP_WRITE_CHR:
                return HandleClientWrite(ctxt);
            default:
                return BLE_ATT_ERR_UNLIKELY;
        }
    }

    int ESPService::HandleClientRead(struct ble_gatt_access_ctxt *ctxt) {
        // When the client requests to read, we rewrite the last thing written by the user level app
        int res = os_mbuf_append(ctxt->om, writeBuf, writeLen);
        return res;
    }

    int ESPService::HandleClientWrite(struct ble_gatt_access_ctxt *ctxt) {
        // When the client writes we read the message immediatly into readBuf. The user level app can then call read() when they are ready to read
        int packetLen = OS_MBUF_PKTLEN(ctxt->om);
        packetLen = std::min(packetLen, MAX_PACKET_LEN);

        int res = ble_hs_mbuf_to_flat(ctxt->om, readBuf, packetLen, NULL);
        return res;
    }
}