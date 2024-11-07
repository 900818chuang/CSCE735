//
// Sorts a list using multiple threads
//

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <limits.h>

#define MAX_THREADS     65536
#define MAX_LIST_SIZE   268435460

#define DEBUG 0

// Thread variables
typedef struct Thread_data {
    int q;
    int index;
} thread_data;

pthread_t p_threads[MAX_THREADS];
pthread_mutex_t mutex_copy_phase, mutex_next_phase, mutex_start_phase;
pthread_cond_t cond_copy_phase_done, cond_next_phase_done, cond_start_phase_ready;
int copy_phase_count = 0, next_phase_count = 0, start_phase_count = 0;

// Global variables
int num_threads;       // Number of threads to create - user input 
int list_size;         // List size
int *list;             // List of values
int *work;             // Work array
int *list_orig;        // Original list of values, used for error checking

// Print list - for debugging
void print_list(int *list, int list_size) {
    int i;
    for (i = 0; i < list_size; i++) {
        printf("[%d] \t %16d\n", i, list[i]);
    }
    printf("--------------------------------------------------------------------\n");
}

// Comparison routine for qsort (stdlib.h)
int compare_int(const void *a0, const void *b0) {
    int a = *(int *)a0;
    int b = *(int *)b0;
    if (a < b) {
        return -1;
    } else if (a > b) {
        return 1;
    } else {
        return 0;
    }
}

// Binary search for the first element larger than or equal to v
int binary_search_lt(int v, int *list, int first, int last) {
    int left = first;
    int right = last - 1;

    if (list[left] >= v) return left;
    if (list[right] < v) return right + 1;

    int mid = (left + right) / 2;
    while (mid > left) {
        if (list[mid] < v) {
            left = mid;
        } else {
            right = mid;
        }
        mid = (left + right) / 2;
    }
    return right;
}

// Binary search for the first element larger than v
int binary_search_le(int v, int *list, int first, int last) {
    int left = first;
    int right = last - 1;

    if (list[left] > v) return left;
    if (list[right] <= v) return right + 1;

    int mid = (left + right) / 2;
    while (mid > left) {
        if (list[mid] <= v) {
            left = mid;
        } else {
            right = mid;
        }
        mid = (left + right) / 2;
    }
    return right;
}

// Helper function to synchronize threads using mutex and condition variables
void synchronize_threads(pthread_mutex_t *mutex, pthread_cond_t *cond, int *counter, int threshold) {
    pthread_mutex_lock(mutex);
    (*counter)++;
    if (*counter == threshold) {
        *counter = 0;
        pthread_cond_broadcast(cond);  // Notify all waiting threads
    } else {
        pthread_cond_wait(cond, mutex);  // Wait for notification
    }
    pthread_mutex_unlock(mutex);
}

// Getter functions
int get_thread_index(thread_data* data) {
    return data->index;
}

int get_merge_level(thread_data* data) {
    return data->q;
}
// Sort list via parallel merge sort
void* execute_parallel_sort(void* data) {
    thread_data* my_data = (thread_data*)data;
    int my_thread_id = get_thread_index(my_data);
    int my_merge_level = get_merge_level(my_data);

    int my_segment_size = list_size / num_threads;
    int my_segment_start = my_thread_id * my_segment_size;
    int my_segment_end = (my_thread_id + 1) * my_segment_size;
    int my_current_start = my_segment_start;
    int my_current_end = my_segment_end;
    int my_index;

    // Sort the local list
    qsort(&list[my_thread_id * my_segment_size], my_segment_size, sizeof(int), compare_int);

    // Synchronize start phase
    synchronize_threads(&mutex_start_phase, &cond_start_phase_ready, &start_phase_count, num_threads);

    // Merge at each level
    while (my_merge_level != 0) {
        if (my_thread_id % 2 == 0) {
            // Left half merge
            for (my_index = my_segment_start; my_index < my_segment_end; my_index++) {
                int my_binary_result = binary_search_lt(list[my_index], list, my_current_start + my_segment_size, my_current_end + my_segment_size);
                my_binary_result = my_binary_result -  (my_current_start + my_segment_size);
                work[my_index + my_binary_result] = list[my_index];
            }
            my_current_end += my_segment_size;
        } else {
            // Right half merge
            for (my_index = my_segment_start; my_index < my_segment_end; my_index++) {
                int my_binary_result = binary_search_le(list[my_index], list, my_current_start - my_segment_size, my_current_end - my_segment_size);
                my_binary_result = my_binary_result - (my_current_start - my_segment_size);
                work[my_index - my_segment_size + my_binary_result] = list[my_index];
            }
            my_current_start -= my_segment_size;
        }

        // Synchronize copy phase
        synchronize_threads(&mutex_copy_phase, &cond_copy_phase_done, &copy_phase_count, num_threads);

        // Copy the work array to the main list
        for (my_index = my_segment_start; my_index < my_segment_end; my_index++) {
            list[my_index] = work[my_index];
        }

        // Synchronize next phase
        synchronize_threads(&mutex_next_phase, &cond_next_phase_done, &next_phase_count, num_threads);

        my_thread_id /= 2;
        my_segment_size *= 2;
        my_merge_level = my_merge_level - 1;
    }

    return NULL;
}
// Main program - set up list of random integers and use threads to sort the list
int main(int argc, char *argv[]) {
    struct timespec start, stop, stop_qsort;
    double total_time, total_time_qsort;
    int k, q, j, error, i;

    // Read input, validate
    if (argc != 3) {
        printf("Need two integers as input\n");
        printf("Use: <executable_name> <log_2(list_size)> <log_2(num_threads)>\n");
        exit(0);
    }
    k = atoi(argv[argc-2]);
    if ((list_size = (1 << k)) > MAX_LIST_SIZE) {
        printf("Maximum list size allowed: %d.\n", MAX_LIST_SIZE);
        exit(0);
    }
    q = atoi(argv[argc-1]);
    if ((num_threads = (1 << q)) > MAX_THREADS) {
        printf("Maximum number of threads allowed: %d.\n", MAX_THREADS);
        exit(0);
    }
    if (num_threads > list_size) {
        printf("Number of threads (%d) < list_size (%d) not allowed.\n", num_threads, list_size);
        exit(0);
    }

    // Allocate list, list_orig, and work
    list = (int *) malloc(list_size * sizeof(int));
    list_orig = (int *) malloc(list_size * sizeof(int));
    work = (int *) malloc(list_size * sizeof(int));

    // Initialize list of random integers
    srand48(0); // seed the random number generator
    for (j = 0; j < list_size; j++) {
        list[j] = (int) lrand48();
        list_orig[j] = list[j];
    }
    // Duplicate first value at last location to test for repeated values
    list[list_size-1] = list[0];
    list_orig[list_size-1] = list_orig[0];

    // Initialize mutexes and condition variables
    pthread_mutex_init(&mutex_copy_phase, NULL);
    pthread_mutex_init(&mutex_next_phase, NULL);
    pthread_mutex_init(&mutex_start_phase, NULL);
    pthread_cond_init(&cond_copy_phase_done, NULL);
    pthread_cond_init(&cond_next_phase_done, NULL);
    pthread_cond_init(&cond_start_phase_ready, NULL);

    // Start time measurement
    clock_gettime(CLOCK_REALTIME, &start);

    // Create threads
    thread_data thread_data_array[num_threads];
    for (i = 0; i < num_threads; i++) {
        thread_data_array[i].index = i;
        thread_data_array[i].q = q;
        pthread_create(&p_threads[i], NULL, execute_parallel_sort, &thread_data_array[i]);
    }

    // Join threads
    for (i = 0; i < num_threads; i++) {
        pthread_join(p_threads[i], NULL);
    }

    // Stop time measurement after sorting
    clock_gettime(CLOCK_REALTIME, &stop);
    total_time = (stop.tv_sec - start.tv_sec) + 0.000000001 * (stop.tv_nsec - start.tv_nsec);

    // Check answer
    qsort(list_orig, list_size, sizeof(int), compare_int);

    // Measure qsort time
    clock_gettime(CLOCK_REALTIME, &stop_qsort);
    total_time_qsort = (stop_qsort.tv_sec - stop.tv_sec) + 0.000000001 * (stop_qsort.tv_nsec - stop.tv_nsec);

    error = 0;
    for (j = 1; j < list_size; j++) {
        if (list[j] != list_orig[j]) error = 1;
    }

    if (error != 0) {
        printf("Houston, we have a problem!\n");
    }

    // Print time taken
    printf("List Size = %d, Threads = %d, error = %d, time (sec) = %8.4f, qsort_time = %8.4f\n", list_size, num_threads, error, total_time, total_time_qsort);

    // Clean up
    pthread_mutex_destroy(&mutex_copy_phase);
    pthread_mutex_destroy(&mutex_next_phase);
    pthread_mutex_destroy(&mutex_start_phase);
    pthread_cond_destroy(&cond_copy_phase_done);
    pthread_cond_destroy(&cond_next_phase_done);
    pthread_cond_destroy(&cond_start_phase_ready);
    free(list);
    free(work);
    free(list_orig);

    return 0;
}
