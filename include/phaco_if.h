#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

#include <zephyr/kernel.h>
#include <zephyr/net/ieee802154_radio.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_uart.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/logging/log.h>
#include <nrf_802154.h>
#include <nrf_802154_const.h>
#include <dk_buttons_and_leds.h>

class IEEE802154Parser {
public:
    static constexpr size_t MAX_BUFFER_SIZE = 256;
    static constexpr size_t BEACON_PACKET_SIZE = 64;
    static constexpr size_t FRAME_CONTROL_SIZE = 2;
    static constexpr size_t SEQ_NUM_SIZE = 1;
    static constexpr size_t ADDRESS_INFO_SIZE = 11;
    static constexpr size_t CRC_SIZE = 2;

private:
    
    uint8_t decode_bits(uint8_t byte_val, uint8_t start_bit, uint8_t end_bit);

    struct beacon_fields {
        uint8_t network_state;         // byte 0, bits 0-2
        uint8_t beacon_source;         // byte 0, bits 3-5
        uint16_t footswitch_id;        // bytes 4-5
        bool sm_overlay;               // byte 6, bit 7
        bool sg_enable;                // byte 7, bit 7
        uint8_t step_type;             // byte 9, bits 0-4
        uint8_t packet_slot_id;        // byte 10, bits 0-4
        bool payload_type;             // byte 10, bit 5
        uint8_t step_sub_type;         // byte 11, bits 0-5
        bool sm_active;                // byte 12, bit 0
        bool sg_active;                // byte 12, bit 1
        bool sm_fsw_active;            // byte 12, bit 3
        uint8_t wireless_channel;      // byte 16
    };

    struct config_fields {
        uint16_t range2_vacuum_limit_end;
    };

    struct status_fields {
        uint8_t footswitch_treadle_range;
        uint8_t iop;
        int16_t irrigation_pressure;
        bool reflux_active;
        bool continuous_irrigation;
        uint8_t range_penetration;
        int16_t aspiration_pressure;
        uint8_t uls_longitudinal_power;
        uint8_t uls_torsional_amplitude;
        uint8_t message_counter;
    };

    char tx_buffer[MAX_BUFFER_SIZE];
    uint8_t network_state;
    uint8_t beacon_source;
    uint16_t footswitch_id;
    bool payload_type;
    uint8_t step_type;
    uint8_t channel;

public:
    IEEE802154Parser() : payload_type(false), step_type(0) {}

    static const struct device *cdc_dev;

    void phaco_feed_tx(const uint8_t* data, 
                        size_t length,
                        uint8_t lqi,
                        int8_t rssi,
                        uint64_t timestamp);
};