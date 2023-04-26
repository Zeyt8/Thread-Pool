#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>

#include "os_graph.h"
#include "os_threadpool.h"
#include "os_list.h"

#define MAX_TASK 100
#define MAX_THREAD 4

int sum = 0;
os_graph_t *graph;

os_queue_t *node_queue;
pthread_mutex_t sum_mutex;
pthread_mutex_t neighbor_mutex;

void processNode(void *arg)
{
    os_threadpool_t *tp = (os_threadpool_t*)arg;
    // pop node from queue
    os_list_node_t *list_node = queue_get(node_queue);
    if (list_node == NULL)
        return;
    // get index and update sum
    unsigned int nodeIdx = *((int*)list_node->info);
    os_node_t *node = graph->nodes[nodeIdx];
    // addition is not atomic
    pthread_mutex_lock(&sum_mutex);
    sum += node->nodeInfo;
    pthread_mutex_unlock(&sum_mutex);
    // for each unvisited neighbor add it to the node queue and start a new task
    for (int i = 0; i < node->cNeighbours; i++) {
        // this is not atomic. mark node as visited, but not complete
        pthread_mutex_lock(&neighbor_mutex);
        int visited = graph->visited[node->neighbours[i]];
        if (visited == 0)
            graph->visited[node->neighbours[i]] = 1;
        pthread_mutex_unlock(&neighbor_mutex);
        // add node to queue and create task
        if (visited == 0) {
            queue_add(node_queue, &node->neighbours[i]);
            os_task_t *task = task_create(tp, processNode);
            add_task_in_queue(tp, task);
        }
    }
    // mark node as complete
    graph->visited[nodeIdx] = 2;
}

int graphDone(os_threadpool_t* tp)
{
    // graph is done if all the nodes are marked as complete
    for (int i = 0; i < graph->nCount; i++)
        if (graph->visited[i] == 0 || graph->visited[i] == 1)
            return 0;
    return 1;
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Usage: ./main input_file\n");
        exit(1);
    }

    FILE *input_file = fopen(argv[1], "r");

    if (input_file == NULL) {
        printf("[Error] Can't open file\n");
        return -1;
    }

    graph = create_graph_from_file(input_file);
    if (graph == NULL) {
        printf("[Error] Can't read the graph from file\n");
        return -1;
    }

    // init sync variables
    pthread_mutex_init(&sum_mutex, NULL);
    pthread_mutex_init(&neighbor_mutex, NULL);
    // create thread pool
    os_threadpool_t *tp = threadpool_create(MAX_TASK, MAX_THREAD);
    // create queue and add first node
    node_queue = queue_create();
    int start = graph->nodes[0]->nodeID;
    graph->visited[start] = 1;
    queue_add(node_queue, &start);
    // create task for first node and add it
    os_task_t *task = task_create(tp, processNode);
    add_task_in_queue(tp, task);
    // wait for all tasks to finish
    threadpool_stop(tp, graphDone);

    printf("%d", sum);
    return 0;
}
