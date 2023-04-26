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

os_threadpool_t *tp;

void processNode(void *arg)
{
    unsigned int nodeIdx = *(unsigned int*)arg;
    os_node_t *node = graph->nodes[nodeIdx];
    sum += node->nodeInfo;
    for (int i = 0; i < node->cNeighbours; i++)
        if (graph->visited[node->neighbours[i]] == 0) {
            graph->visited[node->neighbours[i]] = 1;
            os_task_t *task = task_create(&node->neighbours[i], processNode);
            add_task_in_queue(tp, task);
        }
}

int graphDone(os_threadpool_t* tp)
{
    for (int i = 0; i < graph->nCount; i++)
        if (graph->visited[i] == 0)
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

    // create thread pool and traverse the graf
    tp = threadpool_create(MAX_TASK, MAX_THREAD);
    int start = graph->nodes[0]->nodeID;
    graph->visited[start] = 1;
    os_task_t *task = task_create(&start, processNode);
    add_task_in_queue(tp, task);
    threadpool_stop(tp, graphDone);

    printf("%d", sum);
    return 0;
}
