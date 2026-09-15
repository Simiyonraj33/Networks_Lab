#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <signal.h>

#define MAX_SUBNETS 128

struct Subnet {
    char network_id[30];
    char first_ip[30];
    char last_ip[30];
    int total_hosts;
    int allocated_hosts;
};

unsigned int ip_to_int(const char *ip) {
    unsigned int a, b, c, d;
    sscanf(ip, "%u.%u.%u.%u", &a, &b, &c, &d);
    return (a << 24) | (b << 16) | (c << 8) | d;
}

void int_to_ip(unsigned int ip, char *str) {
    sprintf(str, "%u.%u.%u.%u", (ip >> 24) & 0xFF, (ip >> 16) & 0xFF, (ip >> 8) & 0xFF, ip & 0xFF);
}

int next_power_of_2(int n) {
    int p = 1;
    while (p < n) {
        p <<= 1;
    }
    return p;
}

void handle_client_process(struct sockaddr_in client_addr, char *initial_buffer) {
    int child_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (child_fd < 0) {
        perror("Child socket creation failed");
        exit(1);
    }

    // FIX 1: Explicitly connect child socket to lock onto this client
    if (connect(child_fd, (struct sockaddr *)&client_addr, sizeof(client_addr)) < 0) {
        perror("Connect failed in child");
        close(child_fd);
        exit(1);
    }

    int i; // FIX 2: Loop counter declared outside for older GCC versions
    char base_ip_str[30];
    int req_subnets;
    sscanf(initial_buffer, "%s %d", base_ip_str, &req_subnets);

    if (req_subnets > MAX_SUBNETS) req_subnets = MAX_SUBNETS;

    int actual_subnets = next_power_of_2(req_subnets);
    if (actual_subnets > MAX_SUBNETS) actual_subnets = MAX_SUBNETS;

    int hosts_per_subnet = 256 / actual_subnets;
    if (hosts_per_subnet < 2) hosts_per_subnet = 2;

    struct Subnet subnets[MAX_SUBNETS];
    unsigned int base_ip = ip_to_int(base_ip_str);
    unsigned int current_ip = base_ip;

    for (i = 0; i < actual_subnets; i++) {
        subnets[i].total_hosts = hosts_per_subnet;
        subnets[i].allocated_hosts = 0;

        int_to_ip(current_ip, subnets[i].network_id);
        int_to_ip(current_ip, subnets[i].first_ip);
        int_to_ip(current_ip + hosts_per_subnet - 1, subnets[i].last_ip);

        current_ip += hosts_per_subnet;
    }

    char info_msg[16384] = "";
    char header[256];
    sprintf(header, "Note: Requested %d subnets adjusted to nearest power of 2 (%d subnets). Block size = %d IPs.\n\n",
            req_subnets, actual_subnets, hosts_per_subnet);
    strcat(info_msg, header);

    for (i = 0; i < actual_subnets; i++) {
        unsigned int net_int = ip_to_int(subnets[i].network_id);
        char first_usable[30], last_usable[30];

        int_to_ip(net_int + 1, first_usable);
        int_to_ip(net_int + subnets[i].total_hosts - 2, last_usable);

        char temp[512];
        sprintf(temp, "Subnet %d:\n  - Network ID (First IP): %s\n  - Broadcast (Last IP): %s\n  - Usable Host Range: %s to %s\n  - Capacity: %d hosts (Usable Slots: %d)\n\n",
                i + 1, subnets[i].network_id, subnets[i].last_ip,
                first_usable, last_usable, subnets[i].total_hosts, subnets[i].total_hosts - 2);
        strcat(info_msg, temp);
    }

    // Using standard send() instead of sendto() since the socket is connected
    send(child_fd, info_msg, strlen(info_msg), 0);

    while (1) {
        int target_subnet;
        int bytes = recv(child_fd, &target_subnet, sizeof(target_subnet), 0);
        if (bytes <= 0 || target_subnet == -1) {
            break;
        }

        char result_msg[256];
        if (target_subnet >= 1 && target_subnet <= actual_subnets) {
            int idx = target_subnet - 1;
            int usable_capacity = subnets[idx].total_hosts > 2 ? subnets[idx].total_hosts - 2 : 0;

            if (subnets[idx].allocated_hosts < usable_capacity) {
                subnets[idx].allocated_hosts++;
                sprintf(result_msg, "SUCCESS: Slot allotted in Subnet %d (Allocated: %d/%d usable slots)",
                        target_subnet, subnets[idx].allocated_hosts, usable_capacity);
            } else {
                sprintf(result_msg, "FULL: Subnet %d is full!", target_subnet);
            }
        } else {
            sprintf(result_msg, "ERROR: Invalid subnet choice.");
        }
        send(child_fd, result_msg, strlen(result_msg), 0);
    }

    printf("Child [PID %d]: Client session ended.\n", getpid());
    close(child_fd);
    exit(0);
}

int main() {
    int main_server_fd;
    struct sockaddr_in address, client_addr;
    socklen_t addrlen = sizeof(client_addr);
    char buffer[4096];

    signal(SIGCHLD, SIG_IGN);

    main_server_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (main_server_fd < 0) {
        perror("Main socket failed");
        return 1;
    }

    int opt = 1;
    setsockopt(main_server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);

    if (bind(main_server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        close(main_server_fd);
        return 1;
    }

    printf("Concurrent UDP Subnet Server using fork() running on port 8080...\n");

    while (1) {
        memset(buffer, 0, sizeof(buffer));
        int recv_len = recvfrom(main_server_fd, buffer, sizeof(buffer) - 1, 0,
                                (struct sockaddr *)&client_addr, &addrlen);
        if (recv_len < 0) continue;

        pid_t pid = fork();

        if (pid < 0) {
            perror("Fork failed");
        } else if (pid == 0) {
            close(main_server_fd);
            printf("Child process created (PID: %d) for client %s:%d\n",
                   getpid(), inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
            handle_client_process(client_addr, buffer); // Fixed parameter
        }
    }

    close(main_server_fd);
    return 0;
}