#include "phaco_if.h"
#include "beacon_maps.h"

LOG_MODULE_REGISTER(phaco_if, CONFIG_UART_LOG_LEVEL);

// CDC-ACM device from the device tree
const struct device *IEEE802154Parser::cdc_dev = DEVICE_DT_GET_ONE(zephyr_cdc_acm_uart);

uint8_t IEEE802154Parser::decode_bits(uint8_t byte_val, uint8_t start_bit, uint8_t end_bit) {
    uint8_t mask = ((1 << (end_bit - start_bit + 1)) - 1) << start_bit;
    return (byte_val & mask) >> start_bit;
}

void IEEE802154Parser::phaco_feed_tx(const uint8_t* psdu, 
                                     size_t length, 
                                     uint8_t lqi, 
                                     int8_t rssi, 
                                     uint64_t timestamp) {
    size_t header_size = FRAME_CONTROL_SIZE + SEQ_NUM_SIZE + ADDRESS_INFO_SIZE;
    if (length < header_size + CRC_SIZE) return;
    
    // Calculate the payload size based on 802.15.4 offsets and total buffer size
    size_t payload_size = length - header_size - CRC_SIZE;
    const size_t buf_size = sizeof(tx_buffer);

    // Packet ID 
    int beacon_tx = 0;
    if (length < (header_size + BEACON_PACKET_SIZE + CRC_SIZE)) {
        beacon_tx = snprintk(tx_buffer, buf_size, "<MALFORMED PACKET> ");
    } else {
        beacon_tx = snprintk(tx_buffer, buf_size, "<BEACON PACKET> ");
    }

    // Packet QoS 
    beacon_tx += snprintk(tx_buffer + beacon_tx, buf_size - beacon_tx,
                        "TS: %lluus|PS: %zu|LQI: %u|RSSI: %ddBm\n",
                        (unsigned long long)timestamp,
                        payload_size,
                        (unsigned int)lqi,
                        (int)rssi);

    // Beacon Packet
    const uint8_t* beacon_data = psdu + header_size;

    beacon_fields beacon{
        .network_state = decode_bits(beacon_data[0], 0, 2),
        .beacon_source = decode_bits(beacon_data[0], 3, 5),
        .footswitch_id = static_cast<uint16_t>((beacon_data[4] << 8) | beacon_data[5]),
        .sm_overlay = static_cast<bool>(decode_bits(beacon_data[6], 7, 7)),
        .sg_enable = static_cast<bool>(decode_bits(beacon_data[7], 7, 7)),
        .step_type = decode_bits(beacon_data[9], 0, 4),
        .packet_slot_id = decode_bits(beacon_data[10], 0, 4),
        .payload_type = static_cast<bool>(decode_bits(beacon_data[10], 5, 5)),
        .step_sub_type = decode_bits(beacon_data[11], 0, 5),
        .sm_active = static_cast<bool>(decode_bits(beacon_data[12], 0, 0)),
        .sg_active = static_cast<bool>(decode_bits(beacon_data[12], 1, 1)),
        .sm_fsw_active = static_cast<bool>(decode_bits(beacon_data[12], 3, 3)),
        .wireless_channel = beacon_data[16]
    };

    beacon_tx += snprintk(tx_buffer + beacon_tx, buf_size - beacon_tx,
        "NS: %s\n"
        "FSID: %u\n"
        "ST: %s\n"
        "PT: %s\n",
        beacon_maps::network_state_map.at(beacon.network_state),
        beacon.footswitch_id,
        beacon_maps::step_type_map.at(beacon.step_type),
        beacon.payload_type ? "Status" : "Setup"
    );

    // Config Packet
    if (beacon.packet_slot_id == 8) {
        const uint8_t* config_payload = beacon_data + 22;

        config_fields config{
            .range2_vacuum_limit_end = static_cast<uint16_t>((config_payload[6] << 8) | config_payload[7])
        };

        beacon_tx += snprintk(tx_buffer + beacon_tx, buf_size - beacon_tx,
            "VacLE: %ummHg\n",
            config.range2_vacuum_limit_end
        );
    }
                        
    // Status Packet
    if (beacon.payload_type) {
        const uint8_t* status_payload = beacon_data  + 38; // Status payload starts at byte 38
        static uint8_t message_counter_ = 0; 

        status_fields status{
            .footswitch_treadle_range = decode_bits(status_payload[1], 4, 5),
            .iop = status_payload[10],
            .irrigation_pressure = static_cast<int16_t>((status_payload[12] << 8) | status_payload[13]),
            .reflux_active = static_cast<bool>(decode_bits(status_payload[14], 0, 0)),
            .continuous_irrigation = static_cast<bool>(decode_bits(status_payload[14], 5, 5)),
            .range_penetration = decode_bits(status_payload[15], 0, 6),
            .aspiration_pressure = static_cast<int16_t>((status_payload[16] << 8) | status_payload[17]),
            .uls_longitudinal_power = status_payload[2],
            .uls_torsional_amplitude = status_payload[3],
            .message_counter = static_cast<uint8_t>(message_counter_ % 16)
        };
        message_counter_++;
        
        beacon_tx += snprintk(tx_buffer + beacon_tx, buf_size - beacon_tx,
            "FSTrdl: %u|"
            "IOPr: %ummHg|"
            "IrrPr: %dmmHg|"
            "Reflux: %s|"
            "CIrr: %s|"
            "FSPen: %u|"
            "AsPr: %dmmHg|"
            "LonPwr: %u%%|"
            "TorAmp: %u%%\n"
            "MC: %u\n",
            status.footswitch_treadle_range,
            status.iop,
            status.irrigation_pressure,
            (status.reflux_active ? "Active" : "Inactive"),
            (status.continuous_irrigation ? "Active" : "Not Active"),
            status.range_penetration,
            status.aspiration_pressure,
            status.uls_longitudinal_power,
            status.uls_torsional_amplitude,
            status.message_counter
        );
    }

    // Transmit data to the USB CDC driver (non-blocking)
    size_t to_send = MIN((size_t)beacon_tx, buf_size - 1);
    int num_written = uart_fifo_fill(cdc_dev, reinterpret_cast<const uint8_t *>(tx_buffer), to_send);

    // Error checks
    if (num_written < (int)to_send) {
        LOG_ERR("UART FIFO accepted only %d of %zu bytes", num_written, to_send);
    }

    if (beacon_tx < 0) {
        LOG_ERR("Beacon decode failed with error: %d", beacon_tx);
        return;
    }

    if ((size_t)beacon_tx >= buf_size) {
        LOG_ERR("Buffer full, skipped packet");
        return;
    }
    
    k_sleep(K_MSEC(5));
}