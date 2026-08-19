#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char url[100];
    char ip[100];
    char mac[100];
    int is_occ;
} HashT;

typedef struct {
    int packet_no;
    char src_ip[100];
    char dest_ip[100];
    char data[33];
    int size;
} Packet;

typedef struct {
    int frame_no;
    char src_mac[100];
    char dest_mac[100];
    int packet_no;
    char payload16[17];
    char checksum16[17];
    char tx_data_chk[33];
} Frame;

int hash(const char *url) {
    int sum = 0, i;
    for (i = 0; url[i] != '\0'; i++)
        sum += url[i];
    return sum % 20;
}

void initialise_hash(HashT table[]) {
    int i;
    for (i = 0; i < 20; i++)
        table[i].is_occ = 0;
}

void insert(HashT table[], const char *url, const char *ip, const char *mac) {
    int index = hash(url);
    int st = index;
    while (table[index].is_occ) {
        index = (index + 1) % 20;
        if (index == st) return;
    }
    strcpy(table[index].url, url);
    strcpy(table[index].ip, ip);
    strcpy(table[index].mac, mac);
    table[index].is_occ = 1;
}

int search(HashT table[], const char *url, HashT *result) {
    int index = hash(url);
    int st = index;
    while (table[index].is_occ) {
        if (strcmp(table[index].url, url) == 0) {
            *result = table[index];
            return 1;
        }
        index = (index + 1) % 20;
        if (index == st) break;
    }
    return 0;
}

void random_ip(char *ip) {
    sprintf(ip, "%d.%d.%d.%d", rand() % 223 + 1, rand() % 255, rand() % 255, rand() % 254 + 1);
}

void random_mac(char *mac) {
    sprintf(mac, "%02X:%02X:%02X:%02X:%02X:%02X", rand() % 256, rand() % 256, rand() % 256, rand() % 256, rand() % 256, rand() % 256);
}

void map(HashT table[], const char *url, HashT *entry) {
    if (search(table, url, entry)) {
        printf("Search Hash Table for '%s': Found matching entry.\n", url);
    } else {
        printf(" -> Search Hash Table for '%s': Match Not Found! Generating Dynamic Values...\n", url);
        char new_ip[100], new_mac[100];
        random_ip(new_ip);
        random_mac(new_mac);
        insert(table, url, new_ip, new_mac);
        search(table, url, entry);
        printf("[Allocated] IP: %s | MAC: %s\n", entry->ip, entry->mac);
    }
}

void preload(HashT table[]) {
    insert(table, "google.com", "142.250.190.46", "00:1A:2B:3C:4D:5E");
    insert(table, "youtube.com", "208.65.153.238", "1A:2B:3C:4D:5E:6F");
    insert(table, "facebook.com", "157.240.22.35", "3C:4D:5E:6F:7A:8B");
    insert(table, "amazon.com", "54.239.28.85", "5E:6F:7A:8B:9C:0D");
    insert(table, "wikipedia.org", "198.35.26.96", "7A:8B:9C:0D:1E:2F");
}

int read_file(const char *filename, char *msg, int max_len) {
    FILE *file = fopen(filename, "r");
    if (!file) return 0;
    if (fgets(msg, max_len, file) != NULL) {
        int len = strlen(msg);
        if (len > 0 && (msg[len - 1] == '\n' || msg[len - 1] == '\r'))
            msg[len - 1] = '\0';
    } else {
        fclose(file);
        return 0;
    }
    fclose(file);
    return 1;
}

void char_to_bin(char ch, char *bin) {
    int i;
    for (i = 7; i >= 0; i--)
        bin[7 - i] = ((int)ch) & (1 << i) ? '1' : '0';
    bin[8] = '\0';
}

void app_bitstream(const char *msg, char *stream) {
    stream[0] = '\0';
    char temp[9];
    int i;
    for (i = 0; msg[i] != '\0'; i++) {
        char_to_bin(msg[i], temp);
        strcat(stream, temp);
    }
}

int random_port() {
    return (rand() % (65535 - 1024 + 1)) + 1024;
}

void int_to_bin(int val, char *bin) {
    int i;
    for (i = 15; i >= 0; i--)
        bin[15 - i] = (val & (1 << i)) ? '1' : '0';
    bin[16] = '\0';
}

void ip_to_bin(const char *ip, char *bin) {
    int octets[4];
    sscanf(ip, "%d.%d.%d.%d", &octets[0], &octets[1], &octets[2], &octets[3]);
    bin[0] = '\0';
    char temp[9];
    int i;
    for (i = 0; i < 4; i++) {
        char_to_bin((char)octets[i], temp);
        strcat(bin, temp);
    }
}

void mac_to_binary_48bit(const char *mac_str, char *binary_str) {
    unsigned int bytes[6];
    sscanf(mac_str, "%x:%x:%x:%x:%x:%x", &bytes[0], &bytes[1], &bytes[2], &bytes[3], &bytes[4], &bytes[5]);
    binary_str[0] = '\0';
    char temp[9];
    int i;
    for (i = 0; i < 6; i++) {
        char_to_bin((char)bytes[i], temp);
        strcat(binary_str, temp);
    }
}

void compute_checksum16(const char *payload16, char *checksumOut) {
    unsigned int val = 0;
    for (int i = 0; i < 16; i++) {
        val = (val << 1) | (payload16[i] - '0');
    }
    unsigned int chkVal = (~val) & 0xFFFF;
    for (int i = 15; i >= 0; i--) {
        checksumOut[15 - i] = ((chkVal >> i) & 1) ? '1' : '0';
    }
    checksumOut[16] = '\0';
}

int build_packets(const char *app_stream, const char *src_ip_bin, const char *dest_ip_bin, Packet packets[]) {
    int app_len = strlen(app_stream);
    int p_idx = 0;
    int curr_bit = 0;

    char padded_stream[16384];
    strcpy(padded_stream, app_stream);
    if (app_len % 16 != 0) {
        int pad_bits = 16 - (app_len % 16);
        for (int b = 0; b < pad_bits; b++) strcat(padded_stream, "0");
        app_len += pad_bits;
    }

    while (curr_bit < app_len) {
        packets[p_idx].packet_no = p_idx + 1;
        strcpy(packets[p_idx].src_ip, src_ip_bin);
        strcpy(packets[p_idx].dest_ip, dest_ip_bin);
        
        strncpy(packets[p_idx].data, padded_stream + curr_bit, 16);
        packets[p_idx].data[16] = '\0';
        packets[p_idx].size = 16;

        curr_bit += 16;
        p_idx++;
    }
    return p_idx;
}

int build_frames(Packet packets[], int total_packets, const char *src_mac_bin, const char *dest_mac_bin, Frame frames[]) {
    for (int p = 0; p < total_packets; p++) {
        frames[p].frame_no = p + 1;
        frames[p].packet_no = packets[p].packet_no;
        strcpy(frames[p].src_mac, src_mac_bin);
        strcpy(frames[p].dest_mac, dest_mac_bin);
        strcpy(frames[p].payload16, packets[p].data);

        compute_checksum16(frames[p].payload16, frames[p].checksum16);

        strcpy(frames[p].tx_data_chk, frames[p].payload16);
        strcat(frames[p].tx_data_chk, frames[p].checksum16);
    }
    return total_packets;
}

int main() {
    srand(72);
    HashT table[20];
    initialise_hash(table);
    preload(table);

    char src_url[100], dest_url[100];
    printf("Enter Source URL : ");
    scanf("%s", src_url);
    printf("Enter Destination URL: ");
    scanf("%s", dest_url);
    printf("\n");

    HashT src, dest;
    printf("--- PART 1: URL MAPPING ---\n");
    map(table, src_url, &src);
    map(table, dest_url, &dest);
    printf("\nResolved Source: %s -> IP: %s -> MAC: %s\n", src.url, src.ip, src.mac);
    printf("Resolved Dest:   %s -> IP: %s -> MAC: %s\n", dest.url, dest.ip, dest.mac);
    printf("--------------------------------------------------------------------\n\n");

    char msg[1000] = {0};
    if (read_file("message.txt", msg, sizeof(msg)) == 0) {
        printf("ERROR! message.txt could not be read. Please create message.txt\n");
        return 1;
    }

    char app_stream[16384] = {0};
    app_bitstream(msg, app_stream);
    printf("=====================================================\n");
    printf("                APPLICATION LAYER                    \n");
    printf("=====================================================\n");
    printf("Original Message Read: \"%s\"\n", msg);
    printf("Application Bitstream:\n%s\n\n", app_stream);

    int src_port = random_port();
    int dest_port = random_port();
    printf("=====================================================\n");
    printf("                 TRANSPORT LAYER                     \n");
    printf("=====================================================\n");
    printf("Source Port: %d | Dest Port: %d\n\n", src_port, dest_port);

  
    char src_ip_bin[33], dest_ip_bin[33];
    ip_to_bin(src.ip, src_ip_bin);
    ip_to_bin(dest.ip, dest_ip_bin);

    Packet packets[500];
    int total_packets = build_packets(app_stream, src_ip_bin, dest_ip_bin, packets);

    printf("=====================================================\n");
    printf("                  NETWORK LAYER                      \n");
    printf("=====================================================\n\n");

    for (int i = 0; i < total_packets; i++) {
        printf("--- Packet %d ---\n", packets[i].packet_no);
        printf("+---------------+--------------+----------------------------------+\n");
        printf("| Network       | Src IP       | %s |\n", packets[i].src_ip);
        printf("| Network       | Dst IP       | %s |\n", packets[i].dest_ip);
        printf("| Network       | 16-bit Data  | %s |\n", packets[i].data);
        printf("+---------------+--------------+----------------------------------+\n\n");
    }

    char src_mac_bin[49], dest_mac_bin[49];
    mac_to_binary_48bit(src.mac, src_mac_bin);
    mac_to_binary_48bit(dest.mac, dest_mac_bin);

    Frame frames[500];
    int total_frames = build_frames(packets, total_packets, src_mac_bin, dest_mac_bin, frames);

    printf("=====================================================\n");
    printf("          DATA LINK LAYER       \n");
    printf("=====================================================\n\n");

    FILE *t_file = fopen("transmission.txt", "w");

    for (int i = 0; i < total_frames; i++) {
        printf("--- Frame %d ---\n", frames[i].frame_no);
        printf("+---------------+--------------+--------------------------------------------------+\n");
        printf("| Data Link     | Src MAC      | %s |\n", frames[i].src_mac);
        printf("| Data Link     | Dst MAC      | %s |\n", frames[i].dest_mac);
        printf("| Data Link     | 16-bit Payload | %s |\n", frames[i].payload16);
        printf("| Checksum      | 16-bit Checksum| %s |\n", frames[i].checksum16);
        printf("| Data Link     | Tx Data+Chk  | %s |\n", frames[i].tx_data_chk);
        printf("+---------------+--------------+--------------------------------------------------+\n\n");

        if (t_file) {
            fprintf(t_file, "%s\n", frames[i].tx_data_chk);
        }
    }

    if (t_file) fclose(t_file);

    printf("Transmitted frames written to 'transmission.txt' successfully.\n");
    return 0;
}
