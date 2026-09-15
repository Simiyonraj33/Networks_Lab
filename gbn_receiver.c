#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8080
#define PACKET_SIZE 3
#define MODULO 8

typedef struct
{
    int seq_num;
    unsigned short checksum;
    char data[PACKET_SIZE + 1];
} Frame;

typedef struct
{
    int ack_num;
} AckFrame;

unsigned short computeChecksum(const char *data)
{
    unsigned int sum = 0;

    for (int i = 0; data[i] != '\0'; i++)
        sum += (unsigned char)data[i];

    return (unsigned short)(sum & 0xFFFF);
}

int receive_all(int sock, void *buffer, size_t size)
{
    size_t received = 0;

    while (received < size)
    {
        ssize_t n = recv(sock, (char *)buffer + received,
                         size - received, 0);

        if (n <= 0)
            return 0;

        received += n;
    }

    return 1;
}

int send_all(int sock, const void *buffer, size_t size)
{
    size_t sent = 0;

    while (sent < size)
    {
        ssize_t n = send(sock, (const char *)buffer + sent,
                         size - sent, 0);

        if (n <= 0)
            return 0;

        sent += n;
    }

    return 1;
}

int main()
{
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    int window_size;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_fd < 0)
    {
        perror("Socket failed");
        return 1;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&server_addr,
             sizeof(server_addr)) < 0)
    {
        perror("Bind failed");
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, 1) < 0)
    {
        perror("Listen failed");
        close(server_fd);
        return 1;
    }

    printf("[RECEIVER] Waiting for connection on port %d...\n", PORT);

    client_fd = accept(server_fd,
                       (struct sockaddr *)&client_addr,
                       &addr_len);

    if (client_fd < 0)
    {
        perror("Accept failed");
        close(server_fd);
        return 1;
    }

    printf("[RECEIVER] Sender connected successfully!\n");

    printf("Enter Window Size N (must match sender, e.g., 4): ");
    scanf("%d", &window_size);

    /*
       Receiver keeps track of the next frame it expects.
    */
    int expected_seq = 0;

    while (1)
    {
        /*
           Sender sends the number of frames in the current
           transmission window first.
        */
        int frame_count;

        if (!receive_all(client_fd, &frame_count, sizeof(int)))
        {
            printf("\n[RECEIVER] Connection closed.\n");
            break;
        }

        /*
           frame_count = 0 means sender has finished.
        */
        if (frame_count == 0)
        {
            printf("\n==================================================\n");
            printf("     ALL DATA RECEIVED SUCCESSFULLY\n");
            printf("==================================================\n");
            break;
        }

        printf("\n==================================================\n");
        printf("[RECEIVER] Waiting to receive %d frame(s) from network...\n",
               frame_count);
        printf("==================================================\n");

        int batch_success = 1;
        int last_valid_ack = (expected_seq - 1 + MODULO) % MODULO;

        for (int i = 0; i < frame_count; i++)
        {
            Frame frame;

            if (!receive_all(client_fd, &frame, sizeof(Frame)))
            {
                printf("\n[RECEIVER] Connection closed.\n");
                close(client_fd);
                close(server_fd);
                return 0;
            }

            printf("\n  [Received Frame %d/%d] -> Seq: %d | Data: \"%s\" | Checksum: 0x%04X\n",
                   i + 1,
                   frame_count,
                   frame.seq_num,
                   frame.data,
                   frame.checksum);

            /*
               Once a frame in the batch is wrong,
               remaining frames are discarded.
            */
            if (!batch_success)
            {
                printf("  --> [Discarded Frame] -> Seq: %d | Data: \"%s\"\n",
                       frame.seq_num, frame.data);
                continue;
            }

            unsigned short calculated =
                computeChecksum(frame.data);

            if (calculated != frame.checksum)
            {
                printf("  --> [CORRUPTED FRAME] Checksum mismatch!\n");

                batch_success = 0;
                continue;
            }

            if (frame.seq_num != expected_seq)
            {
                printf("  --> [OUT-OF-ORDER FRAME] Expected Seq %d, got Seq %d.\n",
                       expected_seq,
                       frame.seq_num);

                batch_success = 0;
                continue;
            }

            printf("  --> [VERIFIED] Frame %d clean and in-order!\n",
                   frame.seq_num);

            last_valid_ack = frame.seq_num;

            expected_seq = (expected_seq + 1) % MODULO;
        }

        printf("\n--------------------------------------------------\n");

        AckFrame ack;

        if (batch_success)
        {
            /*
               All frames in the current window were received.
               Send ACK for the LAST frame.
            */
            ack.ack_num = last_valid_ack;

            printf("--> [BATCH SUCCESS]\n");
            printf("--> Sending cumulative ACK (%d)\n",
                   ack.ack_num);
        }
        else
        {
            /*
               Send ACK for the last frame received correctly.
            */
            ack.ack_num = last_valid_ack;

            printf("--> [BATCH FAILED]\n");
            printf("--> Sending last valid ACK (%d)\n",
                   ack.ack_num);
        }

        if (!send_all(client_fd, &ack, sizeof(AckFrame)))
        {
            printf("[RECEIVER] Failed to send ACK.\n");
            break;
        }
    }

    close(client_fd);
    close(server_fd);

    return 0;
}