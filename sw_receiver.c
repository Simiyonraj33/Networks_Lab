// receiver.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8000
#define PACKET_SIZE 3

// Clean Frame Struct (No fake error flags!)
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
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    
    // 1. Create TCP Socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // 2. Bind and Listen
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("[RECEIVER] Waiting for connection on port %d...\n", PORT);
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
        perror("Accept failed");
        exit(EXIT_FAILURE);
    }
    printf("[RECEIVER] Sender connected successfully!\n");

    int expected_seq = 0;
    Frame frame;

    while (1) {
        int bytes_read = recv(new_socket, &frame, sizeof(Frame), 0);
        if (bytes_read <= 0) {
            printf("\n[RECEIVER] Connection closed by Sender.\n");
            break;
        }

        printf("\n--------------------------------------------------\n");
        printf("[RECEIVER] Received Frame -> Seq: %d | Data: \"%s\" | Checksum: %d\n", 
               frame.seq_num, frame.data, frame.checksum);

        // Case 3 (Receiver Side): Checksum Verification for Corrupted Frame
        int computed_checksum = calculate_checksum(frame.data);
        if (computed_checksum != frame.checksum) {
            printf("--> [CORRUPTION DETECTED] Computed Checksum (%d) != Received Checksum (%d)\n", 
                   computed_checksum, frame.checksum);
            printf("--> Silently dropping frame to force Sender Timeout...\n");
            continue; // Do NOT send ACK
        }

        // Duplicate Frame Handling
        if (frame.seq_num != expected_seq) {
            printf("--> [DUPLICATE FRAME] Expected Seq %d, but got Seq %d.\n", expected_seq, frame.seq_num);
            printf("--> Resending ACK %d to recover Sender...\n", frame.seq_num);
            
            AckFrame ack;
            ack.ack_num = frame.seq_num;
            send(new_socket, &ack, sizeof(AckFrame), 0);
            continue;
        }

        printf("--> [SUCCESS] Frame verified cleanly! Payload: \"%s\"\n", frame.data);

        // Case 1 (Receiver Side): Interactive Prompt for ACK Loss Simulation / Normal ACK
        char ack_choice;
        printf("Prompt: Send ACK %d? (y = Yes, a = Simulate ACK Loss): ", expected_seq);
        scanf(" %c", &ack_choice);

        if (ack_choice == 'a' || ack_choice == 'A') {
            printf("--> [SIMULATION] ACK lost! Skipping send()...\n");
            // Do NOT toggle expected_seq because Sender will time out and retransmit
        } else {
            AckFrame ack;
            ack.ack_num = expected_seq;
            send(new_socket, &ack, sizeof(AckFrame), 0);
            printf("--> [ACK SENT] ACK %d transmitted successfully.\n", expected_seq);
            
            // Toggle Sequence Number (0 -> 1 or 1 -> 0)
            expected_seq = 1 - expected_seq;
        }
    }

    close(new_socket);
    close(server_fd);
    return 0;
}