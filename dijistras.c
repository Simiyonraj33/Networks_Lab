#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>

#define INF INT_MAX

//ADJACENCY LIST (Graph representation)
typedef struct Node 
{
    int target;
    int weight;
    struct Node* next;
} Node;

typedef struct Graph 
{
    int num_vertices;
    Node** adj_list;
} Graph;

Node* create_node(int target, int weight) {
    Node* new_node = (Node*)malloc(sizeof(Node));
    new_node->target = target;
    new_node->weight = weight;
    new_node->next = NULL;
    return new_node;
}

Graph* create_graph(int num_vertices) {
    Graph* graph = (Graph*)malloc(sizeof(Graph));
    graph->num_vertices = num_vertices;
    graph->adj_list = (Node**)malloc(num_vertices * sizeof(Node*));
    for (int i = 0; i < num_vertices; i++) {
        graph->adj_list[i] = NULL;
    }
    return graph;
}

void add_edge(Graph* graph, int src, int dest, int weight) {
    // Add edge from src to dest (Directed Graph)
    Node* new_node = create_node(dest, weight);
    new_node->next = graph->adj_list[src];
    graph->adj_list[src] = new_node;
}

void free_graph(Graph* graph) {
    for (int i = 0; i < graph->num_vertices; i++) {
        Node* current = graph->adj_list[i];
        while (current != NULL) {
            Node* temp = current;
            current = current->next;
            free(temp);
        }
    }
    free(graph->adj_list);
    free(graph);
}

// ==========================================
// 2. MIN HEAP (Priority Queue)
// ==========================================
typedef struct MinHeapNode 
{
    int v;
    int dist;
} MinHeapNode;

typedef struct MinHeap 
{
    int size;
    int capacity;
    int* pos; // Map to track vertex position in heap for DECREASEKEY
    MinHeapNode** array;
} MinHeap;

MinHeapNode* new_min_heap_node(int v, int dist) 
{
    MinHeapNode* node = (MinHeapNode*)malloc(sizeof(MinHeapNode));
    node->v = v;
    node->dist = dist;
    return node;
}

MinHeap* create_min_heap(int capacity) 
{
    MinHeap* min_heap = (MinHeap*)malloc(sizeof(MinHeap));
    min_heap->pos = (int*)malloc(capacity * sizeof(int));
    min_heap->size = 0;
    min_heap->capacity = capacity;
    min_heap->array = (MinHeapNode**)malloc(capacity * sizeof(MinHeapNode*));
    return min_heap;
}

void swap_min_heap_node(MinHeapNode** a, MinHeapNode** b) 
{
    MinHeapNode* t = *a;
    *a = *b;
    *b = t;
}

void min_heapify(MinHeap* min_heap, int idx) {
    int smallest = idx;
    int left = 2 * idx + 1;
    int right = 2 * idx + 2;

    if (left < min_heap->size && min_heap->array[left]->dist < min_heap->array[smallest]->dist)
        smallest = left;

    if (right < min_heap->size && min_heap->array[right]->dist < min_heap->array[smallest]->dist)
        smallest = right;

    if (smallest != idx) {
        MinHeapNode* smallest_node = min_heap->array[smallest];
        MinHeapNode* idx_node = min_heap->array[idx];

        // Swap positions in pos map
        min_heap->pos[smallest_node->v] = idx;
        min_heap->pos[idx_node->v] = smallest;

        // Swap nodes
        swap_min_heap_node(&min_heap->array[smallest], &min_heap->array[idx]);
        min_heapify(min_heap, smallest);
    }
}

bool is_empty(MinHeap* min_heap) {
    return min_heap->size == 0;
}

// Extracts the minimum distance vertex (EXTRACTMIN)
MinHeapNode* extract_min(MinHeap* min_heap) {
    if (is_empty(min_heap)) return NULL;

    MinHeapNode* root = min_heap->array[0];
    MinHeapNode* last_node = min_heap->array[min_heap->size - 1];

    min_heap->array[0] = last_node;

    // Update positions
    min_heap->pos[root->v] = min_heap->size - 1;
    min_heap->pos[last_node->v] = 0;

    --min_heap->size;
    min_heapify(min_heap, 0);

    return root;
}

// Decreases distance value of a vertex v (DECREASEKEY)
void decrease_key(MinHeap* min_heap, int v, int dist) {
    int i = min_heap->pos[v];
    min_heap->array[i]->dist = dist;

    // Percolate up
    while (i && min_heap->array[i]->dist < min_heap->array[(i - 1) / 2]->dist) {
        min_heap->pos[min_heap->array[i]->v] = (i - 1) / 2;
        min_heap->pos[min_heap->array[(i - 1) / 2]->v] = i;
        swap_min_heap_node(&min_heap->array[i], &min_heap->array[(i - 1) / 2]);

        i = (i - 1) / 2;
    }
}

bool is_in_min_heap(MinHeap* min_heap, int v) {
    if (min_heap->pos[v] < min_heap->size)
        return true;
    return false;
}

// Helper function to print path from source to a given vertex
void print_path(int prev[], int j) {
    if (prev[j] == -1) {
        printf("%d", j);
        return;
    }
    print_path(prev, prev[j]);
    printf(" -> %d", j);
}
// Helper function to find the first step (next hop) from source to destination
int get_next_hop(int prev[], int source, int destination) {
    // If it's the source itself, there is no next hop
    if (destination == source) {
        return source; 
    }
    
    // Trace backward until we find the node whose predecessor is the source
    int current = destination;
    while (prev[current] != source && prev[current] != -1) {
        current = prev[current];
    }
    
    // If the path breaks or is unreachable, return -1
    if (prev[current] == -1) {
        return -1;
    }
    
    return current;
}

// ==========================================
// 3. DIJKSTRA'S ALGORITHM
// ==========================================
void dijkstra(Graph* graph, int s) {
    int V = graph->num_vertices;
    int dist[V];
    int prev[V];

    for (int v = 0; v < V; v++) {
        dist[v] = INF;
        prev[v] = -1;
    }

    dist[s] = 0;

    MinHeap* Q = create_min_heap(V);
    for (int v = 0; v < V; v++) {
        Q->array[v] = new_min_heap_node(v, dist[v]);
        Q->pos[v] = v;
    }
    Q->size = V;

    decrease_key(Q, s, dist[s]);

    while (!is_empty(Q)) {
        MinHeapNode* min_node = extract_min(Q);
        int u = min_node->v;
        free(min_node);

        Node* pCrawl = graph->adj_list[u];
        while (pCrawl != NULL) {
            int v = pCrawl->target;
            int weight = pCrawl->weight;

            if (is_in_min_heap(Q, v) && dist[u] != INF && dist[v] > dist[u] + weight) {
                dist[v] = dist[u] + weight;
                decrease_key(Q, v, dist[v]);
                prev[v] = u;
            }
            pCrawl = pCrawl->next;
        }
    }

    printf("\n=========================================================================\n");
    printf("               DIJKSTRA ROUTING RESULTS (SOURCE: NODE %d)                 \n", s);
    printf("=========================================================================\n");
    printf("%-11s | %-10s | %-10s | %-20s\n", "Destination", "Min Cost", "Next Hop", "Shortest Path");
    printf("-------------------------------------------------------------------------\n");

    for (int i = 0; i < V; i++) 
    {
        if (dist[i] == INF) 
        {
            printf("%-11d | %-10s | %-10s | %-20s\n", i, "UNREACHABLE", "None", "None");
        } 
        else if (i == s)
        {
            printf("%-11d | %-10d | %-10s | ", i, dist[i], "-");
            print_path(prev, i);
            printf("\n");
        } 
        else 
        {
            int hop = get_next_hop(prev, s, i);
            if (hop == -1) 
            {
                printf("%-11d | %-10d | %-10s | %-20s\n", i, dist[i], "None", "None");
            } 
            else
            {
                printf("%-11d | %-10d | %-10d | ", i, dist[i], hop);
                print_path(prev, i);
                printf("\n");
            }
        }
    }
    printf("=========================================================================\n");


    // Cleanup Heap
    free(Q->array);
    free(Q->pos);
    free(Q);
}


int main() 
{
    int V, E, source;

    printf("=====================================\n");
    printf("     NETWORK GRAPH CONFIGURATION     \n");
    printf("=====================================\n");

    //Get Number of Vertices
    printf("Enter number of vertices (routers/nodes): ");
    if (scanf("%d", &V) != 1 || V <= 0) {
        printf("Error: Invalid number of vertices.\n");
        return 1;
    }

    Graph* graph = create_graph(V);

    //Get Number of Edges
    printf("Enter number of edges (links): ");
    if (scanf("%d", &E) != 1 || E < 0) 
    {
        printf("Error: Invalid number of edges.\n");
        free_graph(graph);
        return 1;
    }

    //Get Edge details from User
    printf("\nEnter edges in the format: [Source] [Destination] [Weight]\n");
    printf("(Note: Vertices should be indexed from 0 to %d)\n\n", V - 1);

    for (int i = 0; i < E; i++) {
        int u, v, w;
        printf("Edge %d: ", i + 1);
        if (scanf("%d %d %d", &u, &v, &w) != 3) {
            printf("Invalid input format. Exiting.\n");
            free_graph(graph);
            return 1;
        }

        //Basic Boundary Validation
        if (u < 0 || u >= V || v < 0 || v >= V) {
            printf("Error: Vertex out of bounds (0 to %d). Re-enter Edge %d.\n", V - 1, i + 1);
            i--; // Retring this edge
            continue;
        }
        if (w < 0) 
        {
            printf("Error: Dijkstra does not support negative weights. Re-enter Edge %d.\n", i + 1);
            i--; // Retring this edge
            continue;
        }

        add_edge(graph, u, v, w);
    }

       // ADD THIS LOOP IN main() INSTEAD:
    printf("\nGenerating routing profiles for ALL nodes...\n");
    for (int source = 0; source < V; source++) {
        dijkstra(graph, source);
    }


    //Cleanup Graph Memory
    free_graph(graph);

    return 0;
}
