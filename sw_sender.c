// sender.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <errno.h>

#define PORT 8000
#define PACKET_SIZE 3
#define TIMEOUT_SEC 10

// Clean Frame Struct
typedef struct {
    int seq_num;                // Sequence Number (0 or 1)
    int checksum;               // ASCII sum for error detection
    char data[PACKET_SIZE + 1]; // Payload chunk (+1 for null terminator)
} Frame;

// Clean ACK Struct
typedef struct {
    int ack_num;                // ACK Sequence Number (0 or 1)
} AckFrame;

// Simple Checksum Calculation
int calculate_checksum(const char *data) {
    int sum = 0;
    for (int i = 0; data[i] != '\0'; i++) {
        sum += (unsigned char)data[i];
    }
    return sum;
}

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;

    // 1. Create TCP Socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        perror("Invalid address");
        return -1;
    }

    // 2. Set Socket Receive Timeout (3 Seconds)
    struct timeval tv;
    tv.tv_sec = TIMEOUT_SEC;
    tv.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));

    // 3. Connect to Receiver
    printf("[SENDER] Connecting to Receiver on port %d...\n", PORT);
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection Failed");
        return -1;
    }
    printf("[SENDER] Connected successfully!\n\n");

    // 4. Read Full String from stdin
    char message[1024];
    printf("Enter string message to send: ");
    fgets(message, sizeof(message), stdin);
    message[strcspn(message, "\n")] = 0; // Strip trailing newline

    int total_len = strlen(message);
    int current_seq = 0;
    int offset = 0;

    // Outer Loop: Process string chunk by chunk (PACKET_SIZE = 3)
    while (offset < total_len) {
        Frame base_frame;
        base_frame.seq_num = current_seq;

        // Slice next PACKET_SIZE bytes from input string
        strncpy(base_frame.data, message + offset, PACKET_SIZE);
        base_frame.data[PACKET_SIZE] = '\0'; // Ensure null terminator
        base_frame.checksum = calculate_checksum(base_frame.data);

        int ack_received = 0;

        // Inner Loop: Retransmits current chunk until valid ACK is received
        while (!ack_received) {
            printf("\n==================================================\n");
            printf("PROCESSING CHUNK: \"%s\" | Seq: %d | Checksum: %d\n", 
                   base_frame.data, current_seq, base_frame.checksum);
            
            // Dynamic Interactive Prompt for Fault Cases
            char action;
            printf("Choose Action (n = Normal Send, f = Lose Frame, c = Corrupt Data): ");
            scanf(" %c", &action);

            Frame tx_frame = base_frame; // Copy clean base frame

            // Case 2: Frame Loss Simulation
            if (action == 'f' || action == 'F') {
                printf("--> [SIMULATION] Frame dropped! Skipping send()...\n");
            } 
            // Case 3: Corrupted Frame Simulation
            else {
                if (action == 'c' || action == 'C') {
                    printf("--> [SIMULATION] Corrupting Checksum value...\n");
                    tx_frame.checksum += 99; // Intentionally corrupt checksum
                }

                printf("--> Transmitting Frame [Seq: %d] over network...\n", tx_frame.seq_num);
                send(sock, &tx_frame, sizeof(Frame), 0);
            }

            // Case 1 & Recovery: Wait for ACK with Timeout
            AckFrame ack;
            printf("--> Waiting for ACK %d (Timeout = %ds)...\n", current_seq, TIMEOUT_SEC);
            
            int bytes_read = recv(sock, &ack, sizeof(AckFrame), 0);

            if (bytes_read < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    printf("--> [TIMEOUT EXPIRED] No ACK received! Retransmitting same chunk...\n");
                } else {
                    perror("recv error");
                    break;
                }
            } else {
                printf("--> [ACK RECEIVED] Received ACK %d from Receiver.\n", ack.ack_num);
                if (ack.ack_num == current_seq) {
                    printf("--> [SUCCESS] Chunk acknowledged cleanly! Moving to next chunk.\n");
                    ack_received = 1;
                    current_seq = 1 - current_seq; // Toggle sequence bit (0 <-> 1)
                    offset += PACKET_SIZE;         // Move forward in string
                } else {
                    printf("--> [WRONG ACK] Expected ACK %d, got ACK %d. Retransmitting...\n", current_seq, ack.ack_num);
                }
            }
        }
    }

    printf("\n==================================================\n");
    printf("[SENDER] Success! Entire message transmitted and acknowledged.\n");
    close(sock);
    return 0;
}