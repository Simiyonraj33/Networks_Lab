#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>

#define PORT 8080
#define PACKET_SIZE 3
#define MODULO 8
#define TIMEOUT_SEC 10

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

int main()
{
    int sock;
    struct sockaddr_in server_addr;

    sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock < 0)
    {
        perror("Socket failed");
        return 1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1",
                  &server_addr.sin_addr) <= 0)
    {
        perror("Invalid address");
        close(sock);
        return 1;
    }

    if (connect(sock, (struct sockaddr *)&server_addr,
                sizeof(server_addr)) < 0)
    {
        perror("Connection failed");
        close(sock);
        return 1;
    }

    printf("[SENDER] Connected to receiver successfully!\n");

    int window_size;

    printf("Enter Go-Back-N Window Size N (e.g., 4): ");
    scanf("%d", &window_size);

    getchar();

    char message[1000];

    printf("Enter message: ");
    fgets(message, sizeof(message), stdin);

    message[strcspn(message, "\n")] = '\0';

    int length = strlen(message);

    int total_frames =
        (length + PACKET_SIZE - 1) / PACKET_SIZE;

    Frame *frames =
        (Frame *)malloc(total_frames * sizeof(Frame));

    if (frames == NULL)
    {
        printf("Memory allocation failed.\n");
        close(sock);
        return 1;
    }

    /*
       Create all frames.
    */
    for (int i = 0; i < total_frames; i++)
    {
        frames[i].seq_num = i % MODULO;

        int start = i * PACKET_SIZE;
        int remaining = length - start;

        int copy_size =
            remaining < PACKET_SIZE ? remaining : PACKET_SIZE;

        strncpy(frames[i].data,
                message + start,
                copy_size);

        frames[i].data[copy_size] = '\0';

        frames[i].checksum =
            computeChecksum(frames[i].data);
    }

    printf("\n[SENDER] Message: \"%s\"\n", message);
    printf("[SENDER] Total chunks: %d\n", total_frames);

    /*
       base = first unacknowledged frame.
    */
    int base = 0;

    while (base < total_frames)
    {
        int end = base + window_size;

        if (end > total_frames)
            end = total_frames;

        int frame_count = end - base;

        /*
           Tell receiver how many frames are
           present in this current window.
        */
        if (!send_all(sock, &frame_count, sizeof(int)))
        {
            printf("Failed to send window information.\n");
            break;
        }

        printf("\n==================================================\n");
        printf("[SENDER] Current Window: Frames %d to %d\n",
               base, end - 1);
        printf("==================================================\n");

        /*
           Send only frames in the CURRENT window.
           Therefore, after ACK 1, frame 0 and 1
           will NOT be retransmitted.
        */
        for (int i = base; i < end; i++)
        {
            Frame temp = frames[i];

            char action;

            printf("\n--- TRANSMITTING FRAME ---\n");
            printf("Payload: \"%s\" | Seq: %d | Checksum: 0x%04X\n",
                   temp.data,
                   temp.seq_num,
                   temp.checksum);

            printf("Action for Frame %d (n = Normal Send, f = Lose Frame, c = Corrupt Data): ",
                   temp.seq_num);

            scanf(" %c", &action);

            if (action == 'f' || action == 'F')
            {
                printf("--> [FRAME LOST] Frame [Seq: %d] was NOT transmitted.\n",
                       temp.seq_num);

                /*
                   To keep TCP stream synchronized,
                   send a special LOST frame.
                   Receiver recognizes it as out-of-order.
                */
                Frame lost_frame = temp;

                lost_frame.seq_num =
                    (temp.seq_num + 5) % MODULO;

                strcpy(lost_frame.data, "XXX");

                lost_frame.checksum =
                    computeChecksum(lost_frame.data);

                if (!send_all(sock, &lost_frame, sizeof(Frame)))
                {
                    printf("Send failed.\n");
                    free(frames);
                    close(sock);
                    return 1;
                }
            }
            else if (action == 'c' || action == 'C')
            {
                temp.checksum ^= 0xFFFF;

                printf("--> [CORRUPTED] Sending Frame [Seq: %d] with wrong checksum...\n",
                       temp.seq_num);

                if (!send_all(sock, &temp, sizeof(Frame)))
                {
                    printf("Send failed.\n");
                    free(frames);
                    close(sock);
                    return 1;
                }
            }
            else
            {
                printf("--> Transmitting Frame [Seq: %d] over network...\n",
                       temp.seq_num);

                if (!send_all(sock, &temp, sizeof(Frame)))
                {
                    printf("Send failed.\n");
                    free(frames);
                    close(sock);
                    return 1;
                }
            }
        }

        printf("\n[ SENDER ] Waiting for cumulative ACK...\n");

        struct timeval tv;

        tv.tv_sec = TIMEOUT_SEC;
        tv.tv_usec = 0;

        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO,
                   &tv, sizeof(tv));

        AckFrame ack;

        if (!receive_all(sock, &ack, sizeof(AckFrame)))
        {
            printf("\n==================================================\n");
            printf("[TIMEOUT] ACK not received within %d seconds!\n",
                   TIMEOUT_SEC);
            printf("[GO-BACK-N] Retransmission triggered.\n");
            printf("==================================================\n");

            /*
               base remains unchanged.
               Therefore the SAME window is retransmitted.
            */
            continue;
        }

        printf("\n--> [ACK RECEIVED] Cumulative ACK: %d\n",
               ack.ack_num);

        /*
           Find which frame the ACK belongs to.
        */
        int ack_index = -1;

        for (int i = base; i < end; i++)
        {
            if (frames[i].seq_num == ack.ack_num)
            {
                ack_index = i;
                break;
            }
        }

        if (ack_index == -1)
        {
            /*
               Old/stale ACK.
               Do NOT slide the window.
            */
            printf("--> [OLD ACK] ACK %d does not belong to current window.\n",
                   ack.ack_num);

            printf("--> Current window remains unchanged.\n");

            continue;
        }

        /*
           ACK is valid.
           Remove everything up to and including ACKed frame.
        */
        printf("--> Frames %d to %d acknowledged.\n",
               base, ack_index);

        base = ack_index + 1;

        printf("--> Window slides forward.\n");
    }

    /*
       Tell receiver that transmission is finished.
    */
    int finish = 0;

    send_all(sock, &finish, sizeof(int));

    printf("\n==================================================\n");
    printf("     ALL DATA TRANSMITTED SUCCESSFULLY\n");
    printf("==================================================\n");

    printf("Message: %s\n", message);

    free(frames);
    close(sock);

    printf("[SENDER] Connection closed.\n");

    return 0;
}