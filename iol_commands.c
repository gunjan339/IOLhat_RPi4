#include "../include/iol_client.h"
int iol_port_power_on(iol_client_t *client, uint8_t port_index) {
    uint8_t data[3] = {port_index,2,0x01};                      //0x01 for Power ON
    
    int result = iol_client_send_command(client, CMD_PORT_POWER, data, 2);
    if (result != IOL_SUCCESS) {
        return result;
    }
    
    // Wait for response
    uint8_t response[BUFFER_SIZE];
    size_t response_len;
    
    result = iol_client_receive_response(client, response, &response_len);
    if (result == IOL_SUCCESS) {
        printf("Port %d powered on successfully\n", port_index);
    }
    
    return result;
}

 int iol_port_power_off(iol_client_t *client, uint8_t port_index) {
    uint8_t data[3] = {port_index,2,0x00};                  //0x00 for Power OFF
    
    int result = iol_client_send_command(client, CMD_PORT_POWER, data, 2);
    if (result != IOL_SUCCESS) {
        return result;
    }
    
    // Wait for response
    uint8_t response[BUFFER_SIZE];
    size_t response_len;
    
    result = iol_client_receive_response(client, response, &response_len);
    if (result == IOL_SUCCESS) {
        printf("Port %d powered off successfully\n", port_index);
    }
    
    return result;
 }


// Case 1: Turn green LED on/off
//    led_on = 1 → green on; 0 → green off
int iol_led_green(iol_client_t *client, uint8_t port_index, uint8_t led_on) {
    uint8_t data[3] = {
        port_index,2,    // Byte 1
        led_on ? 0x01  // Byte 3 payload = green mask
               : 0x00
    };
    // data_len = 2: [port][mask], mask=0→off,1→green
    return iol_client_send_command(client, CMD_LED, data, 2);
}

// Case 2: Turn red LED on/off
//    led_on = 1 → red on; 0 → red off
int iol_led_red(iol_client_t *client, uint8_t port_index, uint8_t led_on) {
    uint8_t data[3] = {
        port_index,2,
        led_on ? 0x02 : 0x00  // 0x02 = red mask
    };
    return iol_client_send_command(client, CMD_LED, data, 2);
}

// Case 3: Turn both LEDs on/off
//    leds_on = 1 → both on; 0 → both off
int iol_led_both(iol_client_t *client, uint8_t port_index, uint8_t leds_on) {
    uint8_t data[3] = {
        port_index,2,
        leds_on ? 0x03 : 0x00  // 0x03 = green+red mask
    };
    return iol_client_send_command(client, CMD_LED, data, 2);
}

