#ifndef ESPNOWSENSOR_H_
#define ESPNOWSENSOR_H_

#define ESPNOW_WIFI_CHANNEL 6

#include "defines.h"
#include "esp_wifi.h"
#include "handyHelpers.h" // Include for currentConfig access
#include "Queue.h"

#include <assert.h>
#include <esp_now.h>
#include <sys/_stdint.h>
#include <WiFi.h>

template<typename T>
struct temp {
    T message;
    uint8_t source[6];
    int32_t rssi;
};

template<typename T> class EspNowSensor {
  private:
    static EspNowSensor *instance;

  private:
    void init();

    void addPeer(uint8_t *addr);

    void printMacAddress();
    void getMacAddress(uint8_t *addr);

    bool send(T message, uint8_t *target);
    bool poll(T *message, uint8_t *source, int32_t *rssi);

    void onDataRecv(const esp_now_recv_info_t *mac, const unsigned char *incomingData, int len);
    void onDataSend(const wifi_tx_info_t *tx_info, esp_now_send_status_t status);

  private:
    static void OnDataRecv(const esp_now_recv_info_t *mac, const unsigned char *incomingData,
                           int len) {
        assert(instance);
        instance->onDataRecv(mac, incomingData, len);
    }

    static void OnDataSend(const wifi_tx_info_t *tx_info, esp_now_send_status_t status) {
        assert(instance);
        instance->onDataSend(tx_info, status);
    }

  public:
    static void Init() {
        // Should only be initialized once
        assert(!instance);
        instance = new EspNowSensor<T>();
        instance->init();
    }

    static void AddPeer(uint8_t *addr) {
        assert(instance);
        instance->addPeer(addr);
    }

    static void PrintMacAddress() {
        assert(instance);
        instance->printMacAddress();
    }

    static void GetMacAddress(uint8_t *addr) {
        assert(instance);
        instance->getMacAddress(addr);
    }

    static bool Send(T message, uint8_t *target) {
        assert(instance);
        return instance->send(message, target);
    }

    static bool Poll(T *message, uint8_t *source, int32_t *rssi) {
        assert(instance);
        return instance->poll(message, source, rssi);
    }

  private:
    Queue<struct temp<T>> _messageQueue;
};

// ================================================================================
// =============================== IMPLEMENTATION
// =================================
// ================================================================================

typedef struct {
    unsigned frame_ctrl  : 16;
    unsigned duration_id : 16;
    uint8_t  addr1[6]; /* receiver address */
    uint8_t  addr2[6]; /* sender address */
    uint8_t  addr3[6]; /* filtering address */
    unsigned sequence_ctrl : 16;
    uint8_t  addr4[6]; /* optional */
} wifi_ieee80211_mac_hdr_t;

typedef struct {
    wifi_ieee80211_mac_hdr_t hdr;
    uint8_t                  payload[0]; /* network data ended with 4 bytes csum (CRC32) */
} wifi_ieee80211_packet_t;

template<typename T> EspNowSensor<T> *EspNowSensor<T>::instance = 0;

template<typename T> void EspNowSensor<T>::init() {

    // Initialize the Wi-Fi module
    WiFi.mode(WIFI_STA);
    delay(1000); // Give WiFi time to initialize

    esp_err_t result = esp_now_init();
    if (result != ESP_OK) {
        Serial.print("Error initializing ESP-NOW, error code: ");
        Serial.println(result);
        return;
    }

    // Register callback functions
    esp_now_register_send_cb(EspNowSensor<T>::OnDataSend);
    esp_now_register_recv_cb(EspNowSensor<T>::OnDataRecv);

    Serial.println("ESP-NOW initialized successfully!");

    printMacAddress();
}

template<typename T> void EspNowSensor<T>::addPeer(uint8_t *addr) {
    // Register peers (both sister and brother devices)
    esp_now_peer_info_t peerInfo;
    memset(&peerInfo, 0, sizeof(peerInfo));
    peerInfo.channel = 0;
    peerInfo.encrypt = false;

    memcpy(peerInfo.peer_addr, addr, 6);
    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("Failed to add brother peer");
    }
}

template<typename T> void EspNowSensor<T>::printMacAddress() {
    uint8_t addr[6];
    getMacAddress(addr);
    debug("MAC Address is : ");
    for (uint8_t i = 0; i < 6; i++) {
        Serial.print(addr[i] < 0x10 ? "0" : "");
        Serial.print(addr[i], HEX);
        if (i != 5) {
            debug(":");
        }
    }
    debug("\n");
}

template<typename T> void EspNowSensor<T>::getMacAddress(uint8_t *addr) {
    WiFi.macAddress(addr);
}

template<typename T> bool EspNowSensor<T>::send(T message, uint8_t *target) {
    T *toSend = (T *)malloc(sizeof(T));
    if (!toSend) {
        return false;
    }

    memcpy(toSend, &message, sizeof(T));

    esp_err_t result = esp_now_send(target, (uint8_t *)toSend, sizeof(T));
    return result == ESP_OK;
}

template<typename T> bool EspNowSensor<T>::poll(T *message, uint8_t *source, int32_t *rssi) {
    if (_messageQueue.isEmpty()) {
        return false;
    }

    struct temp<T> _temp = _messageQueue.pop();
    memcpy(message, &_temp.message, sizeof(T));
    memcpy(source, _temp.source, 6);
    *rssi = _temp.rssi;
    return true;
}

template<typename T>
void EspNowSensor<T>::onDataRecv(const esp_now_recv_info_t *mac, const unsigned char *incomingData,
                                 int len) {
    struct temp<T> _temp;
    memcpy(&_temp.message, incomingData, sizeof(T));
    memcpy(_temp.source, mac->src_addr, 6);
    _temp.rssi = mac->rx_ctrl->rssi;
    _messageQueue.push(_temp);
}

template<typename T>
void EspNowSensor<T>::onDataSend(const wifi_tx_info_t *tx_info, esp_now_send_status_t status) {
    static bool prevStatus = false;
    if (status != prevStatus) {
        debug("Last Packet Send Status: ");
        debugln(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
        prevStatus = status;
    }

    // Optional: You can now access additional information if needed:
    // tx_info->des_addr - destination MAC address
    // tx_info->src_addr - source MAC address
    // tx_info->tx_status - transmission status (alternative to 'status'
    // parameter)
}

/*
template<typename T>
void EspNowSensor<T>::promiscuousRxCb(void *buf, wifi_promiscuous_pkt_type_t type) {
    if (type != WIFI_PKT_MGMT) {
        return;
    }

    const wifi_promiscuous_pkt_t   *ppkt = (wifi_promiscuous_pkt_t *)buf;
    const wifi_ieee80211_packet_t  *ipkt = (wifi_ieee80211_packet_t *)ppkt->payload;
    const wifi_ieee80211_mac_hdr_t *hdr  = &ipkt->hdr;
    for (size_t i = 0; i < 6; i++) {
        if (hdr->addr2[i] != _rssiCmpAddr[i]) {
            return;
        }

        _rssi = ppkt->rx_ctrl.rssi;
    }
}*/

#endif /* ESPNOWSENSOR_H_ */
