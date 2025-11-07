#include "../include/iol_client.h"
int iol_client_init(iol_client_t *client, int tcp_port) {
    // Initialize client structure
    memset(client, 0, sizeof(iol_client_t));
    client->socket_fd = -1;     // by default file descriptor is 1
    client->connected = 0;      // by default its not connected
    
    // Create TCP socket
    client->socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client->socket_fd == -1) {
        perror("Socket creation failed");
        return IOL_ERROR_SOCKET;
    }
    
    // Configure server address
    memset(&client->server_addr, 0, sizeof(client->server_addr));
    client->server_addr.sin_family = AF_INET;
    client->server_addr.sin_port = htons(tcp_port);
    
    // Convert IP address from string to binary
    if (inet_pton(AF_INET, IOL_HAT_HOST, &client->server_addr.sin_addr) <= 0) {
        perror("Invalid address");
        close(client->socket_fd);
        client->socket_fd = -1;
        return IOL_ERROR_SOCKET;
    }
    
    printf("IOL Client initialized for port %d\n", tcp_port);
    return IOL_SUCCESS;
}

int iol_client_connect(iol_client_t *client) {
    if (client->socket_fd == -1) {
        fprintf(stderr, "Client not initialized\n");
        return IOL_ERROR_SOCKET;
    }
    
    // Attempt connection
    if (connect(client->socket_fd, (struct sockaddr *)&client->server_addr, sizeof(client->server_addr)) < 0) {
        perror("Connection failed");
        return IOL_ERROR_CONNECT;
    }
    
    client->connected = 1;
    printf("Connected to IOL HAT Master Application at %s\n", IOL_HAT_HOST);
    return IOL_SUCCESS;
}

void iol_client_close(iol_client_t *client) {
    if (client->socket_fd != -1) {
        close(client->socket_fd);
        client->socket_fd = -1;
    }
    client->connected = 0;
    printf("IOL Client connection closed\n");
}


void print_hex_data(const char *label, uint8_t *data, size_t len) {
    printf("%s (%zu bytes): ", label, len);
    for (size_t i = 0; i < len; i++) {
        printf("%02X ", data[i]);
    }
    printf("\n");
}



int iol_client_send_command(iol_client_t *client, uint8_t command_id, uint8_t *data, size_t data_len) {
    if (!client->connected) {
        fprintf(stderr, "Client not connected\n");
        return IOL_ERROR_CONNECT;
    }

    // Frame: [CMD][PORT][LEN][DATA…]
    size_t total = 3 + data_len;
    uint8_t buf[3 + BUFFER_SIZE];
    buf[0] = command_id;
    buf[1] = data ? data[0] : 0;      // data[0] is always port index for all cmds
    buf[2] = (uint8_t)data_len;

    if (data_len > 1) {
        memcpy(&buf[3], &data[2], data_len - 1);
    }

    ssize_t sent = send(client->socket_fd, buf, total, 0);
    if (sent != (ssize_t)total) {
        perror("Send failed");
        return IOL_ERROR_SEND;
    }
    
    printf("Sent command 0x%02X: ", command_id);
    print_hex_data("CMD", buf, total);
    
    return IOL_SUCCESS;
}


int iol_client_receive_response(iol_client_t *client, uint8_t *response, size_t *response_len) {
    if (!client->connected) {
        fprintf(stderr, "Client not connected\n");
        return IOL_ERROR_CONNECT;
    }
    
    // Receive header (minimum 2 bytes)
    ssize_t bytes_received = recv(client->socket_fd, response, 2, 0);
    if (bytes_received < 2) {
        perror("Failed to receive response header");
        return IOL_ERROR_RECEIVE;
    }
    
    // Check for error response (0xFF indicates error)
    if (response[0] == 0xFF) {
        printf("Error response received: Error code 0x%02X\n", response[1]);
        *response_len = 2;
        return IOL_ERROR_PROTOCOL;
    }
    
    // For successful responses, we need to determine the expected length
    // based on the command type. Since we don't know the command here,
    // we'll read a reasonable amount of data and let command-specific
    // parsers handle the actual structure.
    
    // Read remaining available data (up to a reasonable limit)
    uint8_t temp_buffer[BUFFER_SIZE - 2];
    ssize_t additional_bytes = recv(client->socket_fd, temp_buffer, sizeof(temp_buffer), MSG_DONTWAIT);
    
    if (additional_bytes > 0) {
        // Copy additional data to response buffer
        memcpy(&response[2], temp_buffer, additional_bytes);
        *response_len = 2 + additional_bytes;
    } else {
        // No additional data available
        *response_len = 2;
    }
    
    printf("Received response: ");
    print_hex_data("RSP", response, *response_len);
    
    return IOL_SUCCESS;
}


int iol_handle_error(uint8_t error_code)
{
    switch (error_code)
    {
    case 0x01:
        printf("Error 0x01: Message Length Error\n");
        break;
    case 0x02:
        printf("Error 0x02: Function ID unknown\n");
        break;
    case 0x03:
        printf("Error 0x03: Port power error\n");
        break;
    case 0x04:
        printf("Error 0x04: Port ID error (port ID >1)\n");
        break;
    case 0x05:
        printf("Error 0x05: Internal error\n");
        break;
    case 0x06:
        printf("Error 0x06: Wrong status\n");
        break;
    default:
        printf("Error 0x%02X: Unknown error code\n", error_code);
        break;
    }
    return IOL_ERROR_PROTOCOL;
}

