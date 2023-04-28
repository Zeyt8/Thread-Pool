# Thread Pool

## Managing Tasks

Managing tasks in the queue uses a trivial approach. Getting tasks is just getting the first element of the list. Adding tasks is just appending to the list.

This behaves like a queue.

Getting and adding operations use the mutex threadpool has to make the operations atomic.

## Running the Tasks

When created, the threadpool start a number of threads.

Each thread does busy waiting while threadpool isn't marked as *should_stop*.

When a threadpool is marked as *should_stop*, each thread will finish the task it is currently running and then exit.

The thread checks the task list for tasks to run. If it finds any, it will run the function in that task and repeat the process.

When threadpool_stop is called, the threadpool does busy waiting until the condition is met and the task list is empty. After that it marks the threadpool as *should_stop* to make the thread break from their infinite loop. Then it joins all the threads to let them complete the task they are running and exit.

# Calculating the Sum of Nodes in a Graph

Each node can be in 3 states: 0 - unvisited, 1 - visited, 2 - complete.

I start a thread pool, mark node 0 as visited and add a task for node 0.

The task adds the value of the node to the sum. After that it marks all it's unvisited neighbours as visited and creates a new task for them.

The node is marked as complete. This is happening at the end, so that the stop condition doesn't require a mutex.

To treat the case when a graph has multiple conex components, during the task, after a node is marked as complete, it also marks another random node (the first unvisited node it finds) as visited and creates a task for it.

The stop condition is when all nodes are marked as complete.