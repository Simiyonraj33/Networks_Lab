#include <stdio.h>
#include <string.h>
 
#define INF 999999      // representing infinity
#define MAX 100         // max nodes / edges
#define MSG_SIZE 1000   // max message length
 
// ---------- structures ----------
typedef struct {
    int id;
    char data[50];      // data carried by the node (e.g. router name)
} Node;
 
typedef struct {
    int from;
    int to;
    int weight;          // link cost, can be negative
} Edge;

// Queue structure for shortest path optimization
typedef struct {
    int data[MAX * MAX];
    int front;
    int rear;
} Queue;
 
// ---------- global data ----------
int V, E;                  // number of vertices and edges
Node nodes[MAX];           // list of nodes
Edge edges[MAX];           // edge list (links between nodes)
int dist[MAX], parent[MAX];    // shortest distance and parent arrays
int s;                      // source node id
int d;                      // destination node id
int hasNegativeCycle;       // flag for negative cycle
char message[MSG_SIZE];     // message read from input file
 
// ---------- function prototypes ----------
void initialize();
void relaxEdges();
void checkNegativeCycle();
void printResult();
void printRoutingTable();
void printNextHop();
int  buildPath(int path[]);
void routeMessage();
 
int main() {
    // ----- input: nodes -----
    printf("Enter number of nodes: ");
    scanf("%d", &V);
 
    printf("Enter data for each node:\n");
    for (int i = 0; i < V; i++) {
        nodes[i].id = i;
        printf("Node %d data (e.g. router name): ", i);
        scanf("%s", nodes[i].data);
    }
 
    // ----- input: edges -----
    printf("Enter number of edges: ");
    scanf("%d", &E);
 
    printf("Enter each edge as: fromNode toNode weight\n");
    printf("(NOTE: Use node IDs 0 to %d)\n", V-1);
    printf("(weight can be negative, e.g. 0 1 -4 means Node0--(-4)-->Node1)\n");
    for (int i = 0; i < E; i++) {
        printf("Edge %d: ", i + 1);
        scanf("%d %d %d", &edges[i].from, &edges[i].to, &edges[i].weight);
    }
 
    // ----- input: source / destination -----
    printf("Enter source node (0-%d): ", V-1);
    scanf("%d", &s);
 
    printf("Enter destination node (0-%d): ", V-1);
    scanf("%d", &d);
 
    // ----- input: message file -----
    char filename[100];
    printf("Enter message file name: ");
    scanf("%s", filename);
 
    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        printf("Could not open file: %s\n", filename);
        return 1;
    }
    fgets(message, MSG_SIZE, fp);
    fclose(fp);
 
    // remove trailing newline character, if any
    int len = strlen(message);
    if (len > 0 && message[len - 1] == '\n') {
        message[len - 1] = '\0';
    }
 
    // ----- algorithm -----
    initialize();
    relaxEdges(); 
    checkNegativeCycle();
    printResult();
    
    // ----- new functions -----
    if (!hasNegativeCycle) {
        printRoutingTable();
        printNextHop();
    }
    
    routeMessage();
 
    return 0;
}
 
void initialize() {
    for (int v = 0; v < V; v++) {
        dist[v] = INF;
        parent[v] = -1;
    }
    dist[s] = 0;
    parent[s] = s;
}
 
// Main body: Uses a Queue data structure to find shortest paths dynamically
void relaxEdges() {
    Queue q;
    q.front = 0;
    q.rear = 0;

    int inQueue[MAX] = {0};
    int enterCount[MAX] = {0};
    int iterationCounter = 1;

    q.data[q.rear++] = s;
    inQueue[s] = 1;
    enterCount[s] = 1;

    printf("\n================================================");
    printf("\nStarting Queue-Based Shortest Path Routing");
    printf("\n================================================\n");

    while (q.front < q.rear) {
        int u = q.data[q.front++];
        inQueue[u] = 0; 

        printf("\n[Queue Pop] Processing Node %d (%s)\n", u, nodes[u].data);
        printf("  Current Distance to Node %d: %d\n", u, dist[u]);

        // Relax only the edges that leave from node u
        for (int j = 0; j < E; j++) {
            if (edges[j].from != u) continue;

            int v = edges[j].to;
            int w = edges[j].weight;

            printf("\n  --- Checking Edge %d -> %d (weight = %d) ---\n", u, v, w);
            
            if (dist[u] == INF) {
                printf("  Source Node %d unreachable. Skipping.\n", u);
                continue;
            }

            printf("  Current Distance[%d] = ", v);
            if(dist[v] == INF) printf("INF\n");
            else printf("%d\n", dist[v]);

            printf("  New Possible Distance = dist[%d](%d) + weight(%d) = %d\n",
                    u, dist[u], w, dist[u]+w);

            if (dist[u] + w < dist[v]) {
                printf("  [UPDATE] %d < %d? YES!\n", dist[u]+w, dist[v]);
                printf("  UPDATED! Changing distance[%d] from %s to %d\n", 
                       v, (dist[v] == INF) ? "INF" : "VALID", dist[u] + w);
                printf("  Parent[%d] changed from %d to %d\n", v, parent[v], u);
                
                dist[v] = dist[u] + w;
                parent[v] = u;

                if (!inQueue[v]) {
                    q.data[q.rear++] = v;
                    inQueue[v] = 1;
                    enterCount[v]++;
                    printf("  Enqueued Node %d for further processing.\n", v);

                    if (enterCount[v] > V) {
                        hasNegativeCycle = 1;
                        printf("\n[ALERT] Infinite negative loop detected at Node %d!\n", v);
                        return;
                    }
                }
            } else {
                printf("  [NO UPDATE] %d < %d? NO.\n", dist[u]+w, dist[v]);
            }
        }

        // Print intermediate status table after processing this queue element
        printf("\n  >>> Distance Table After Step %d <<<\n", iterationCounter++);
        printf("  --------------------------------------------\n");
        printf("  Node\tDistance\tParent\n");
        for (int k = 0; k < V; k++) {
            printf("  %d(%s)\t", k, nodes[k].data);
            if (dist[k] == INF) printf("INF\t\t");
            else printf("%d\t\t", dist[k]);

            if (parent[k] == -1) printf("NIL\n");
            else printf("%d(%s)\n", parent[k], nodes[parent[k]].data);
        }
    }
}
 
void checkNegativeCycle() {
    if (hasNegativeCycle) return;

    hasNegativeCycle = 0;
    for (int j = 0; j < E; j++) {
        int u = edges[j].from;
        int v = edges[j].to;
        int w = edges[j].weight;
 
        if (dist[u] != INF && dist[u] + w < dist[v]) {
            hasNegativeCycle = 1;
            break;
        }
    }
}
 
void printResult() {
    if (hasNegativeCycle) {
        printf("\nNegative weight cycle exists! Shortest paths are undefined.\n");
    } else {
        printf("\nShortest distances from source node %d (%s):\n", s, nodes[s].data);
        for (int v = 0; v < V; v++) {
            if (dist[v] == INF)
                printf("Node %d (%s): unreachable\n", v, nodes[v].data);
            else
                printf("Node %d (%s): distance = %d, parent = %d (%s)\n",
                       v, nodes[v].data, dist[v], parent[v], nodes[parent[v]].data);
        }
    }
}

// NEW FUNCTION: Prints the complete Routing Table
void printRoutingTable() {
    printf("\n================================================");
    printf("\nROUTING TABLE (From Source Node %d)");
    printf("\n================================================\n");
    printf("%-10s %-15s %-10s %-10s\n", "Destination", "Node Name", "Cost", "Via (Parent)");
    printf("------------------------------------------------------------\n");
    
    for (int i = 0; i < V; i++) {
        if (dist[i] == INF) {
            printf("%-10d %-15s %-10s %-10s\n", i, nodes[i].data, "INF", "-");
        } else {
            printf("%-10d %-15s %-10d %-10d(%s)\n", 
                   i, nodes[i].data, dist[i], parent[i], nodes[parent[i]].data);
        }
    }
}

// NEW FUNCTION: Prints the Next Hop for each destination
void printNextHop() {
    printf("\n================================================");
    printf("\nNEXT HOP TABLE (From Source Node %d)");
    printf("\n================================================\n");
    printf("%-15s %-15s %-15s\n", "Destination", "Next Hop", "Next Hop Name");
    printf("------------------------------------------------------------\n");
    
    for (int i = 0; i < V; i++) {
        if (dist[i] == INF) {
            printf("%-15d %-15s %-15s\n", i, "None", "-");
        } else if (i == s) {
            printf("%-15d %-15s %-15s\n", i, "Self", nodes[i].data);
        } else {
            // Trace back from destination to find the immediate neighbor of source
            int current = i;
            while (parent[current] != s && parent[current] != -1) {
                current = parent[current];
            }
            
            if (parent[current] == s) {
                printf("%-15d %-15d %-15s\n", i, current, nodes[current].data);
            } else {
                printf("%-15d %-15s %-15s\n", i, "Direct", nodes[current].data);
            }
        }
    }
}
 
int buildPath(int path[]) {
    if (dist[d] == INF) return 0;
 
    int temp[MAX], len = 0;
    int v = d;
    while (v != s) {
        temp[len++] = v;
        v = parent[v];
    }
    temp[len++] = s;
 
    for (int i = 0; i < len; i++) {
        path[i] = temp[len - 1 - i];
    }
    return len;
}
 
void routeMessage() {
    if (hasNegativeCycle) {
        printf("\nCannot route message: negative weight cycle in network.\n");
        return;
    }
 
    int path[MAX];
    int len = buildPath(path);
 
    if (len == 0) {
        printf("\nDestination %d is unreachable from source %d. Message dropped.\n", d, s);
        return;
    }
 
    printf("\nRouting message: \"%s\"\n", message);
    printf("Path (total cost = %d):\n", dist[d]);
    for (int i = 0; i < len; i++) {
        int nid = path[i];
        if (i == 0)
            printf("Node %d (%s) [source]\n", nid, nodes[nid].data);
        else if (i == len - 1)
            printf("  --> Node %d (%s) [destination]\n", nid, nodes[nid].data);
        else
            printf("  --> Node %d (%s)\n", nid, nodes[nid].data);
    }
    printf("\nMessage \"%s\" successfully delivered to Node %d (%s)\n",
           message, d, nodes[d].data);
}
