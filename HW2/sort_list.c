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
//
// VS: ... declare thread variables, mutexes, condition varables, etc.,
// VS: ... as needed for this assignment 
//
typedef struct Thread_data{
    int q;
    int index;
}thread_data;

pthread_t p_threads[MAX_THREADS];
pthread_cond_t cond_copy, cond_next, cond_start;
pthread_mutex_t lock_copy, lock_next, lock_start;
int copy_count = 0, next_count = 0, start_count = 0;


// Global variables
int num_threads;		// Number of threads to create - user input 
int list_size;			// List size
int *list;			// List of values
int *work;			// Work array
int *list_orig;			// Original list of values, used for error checking

// Print list - for debugging
void print_list(int *list, int list_size) {
    int i;
    for (i = 0; i < list_size; i++) {
        printf("[%d] \t %16d\n", i, list[i]); 
    }
    printf("--------------------------------------------------------------------\n"); 
}

// Comparison routine for qsort (stdlib.h) which is used to 
// a thread's sub-list at the start of the algorithm
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

// Return index of first element larger than or equal to v in sorted list
// ... return last if all elements are smaller than v
// ... elements in list[first], list[first+1], ... list[last-1]
//
//   int idx = first; while ((v > list[idx]) && (idx < last)) idx++;
//
int binary_search_lt(int v, int *list, int first, int last) {
   
    // Linear search code
    // int idx = first; while ((v > list[idx]) && (idx < last)) idx++; return idx;

    int left = first; 
    int right = last-1; 

    if (list[left] >= v) return left;
    if (list[right] < v) return right+1;
    int mid = (left+right)/2; 
    while (mid > left) {
        if (list[mid] < v) {
	    left = mid; 
	} else {
	    right = mid;
	}
	mid = (left+right)/2;
    }
    return right;
}
// Return index of first element larger than v in sorted list
// ... return last if all elements are smaller than or equal to v
// ... elements in list[first], list[first+1], ... list[last-1]
//
//   int idx = first; while ((v >= list[idx]) && (idx < last)) idx++;
//
int binary_search_le(int v, int *list, int first, int last) {

    // Linear search code
    // int idx = first; while ((v >= list[idx]) && (idx < last)) idx++; return idx;
 
    int left = first; 
    int right = last-1; 

    if (list[left] > v) return left; 
    if (list[right] <= v) return right+1;
    int mid = (left+right)/2; 
    while (mid > left) {
        if (list[mid] <= v) {
	    left = mid; 
	} else {
	    right = mid;
	}
	mid = (left+right)/2;
    }
    return right;
}

// Sort list via parallel merge sort
//
// VS: ... to be parallelized using threads ...
//
void* parallel_sort(void* args) { 
    thread_data* thread_info = (thread_data*)args;
    int tid = thread_info->index;               // Thread index
    int merge_depth = thread_info->q;           // Depth of the merge process
    int chunk_size = list_size / num_threads;   // Size of each segment to sort
    int start_idx = tid * chunk_size;           // Start of this thread's segment
    int end_idx = (tid + 1) * chunk_size;       // End of this thread's segment
    int left_bound = start_idx;
    int right_bound = end_idx;

    // Sort local segment using quicksort
    qsort(&list[start_idx], chunk_size, sizeof(int), compare_int);

    // Synchronization at the start phase
    pthread_mutex_lock(&mutex_start);
    start_ready++;
    if (start_ready == num_threads) {
        start_ready = 0;
        pthread_cond_broadcast(&cond_start);
    } else {
        pthread_cond_wait(&cond_start, &mutex_start);
    }
    pthread_mutex_unlock(&mutex_start);

    // Merge at each level of depth
    while (merge_depth > 0) {
        int is_even_thread = (tid % 2 == 0);
        
        if (is_even_thread) {
            // Merge left half
            for (int i = start_idx; i < end_idx; i++) {
                int pos_in_right = binary_search_lt(list[i], list, left_bound + chunk_size, right_bound + chunk_size);
                int offset = pos_in_right - (left_bound + chunk_size);
                temp_work[i + offset] = list[i];
            }
            right_bound += chunk_size;
        } else {
            // Merge right half
            for (int i = start_idx; i < end_idx; i++) {
                int pos_in_left = binary_search_le(list[i], list, left_bound - chunk_size, right_bound - chunk_size);
                int offset = pos_in_left - (left_bound - chunk_size);
                temp_work[i - chunk_size + offset] = list[i];
            }
            left_bound -= chunk_size;
        }

        tid /= 2;
        chunk_size *= 2;
        merge_depth--;

        // Synchronization for copying phase
        pthread_mutex_lock(&mutex_copy);
        copy_ready++;
        if (copy_ready == num_threads) {
            copy_ready = 0;
            pthread_cond_broadcast(&cond_copy);
        } else {
            pthread_cond_wait(&cond_copy, &mutex_copy);
        }
        pthread_mutex_unlock(&mutex_copy);

        // Copy sorted data back to main list
        for (int i = start_idx; i < end_idx; i++) {
            list[i] = temp_work[i];
        }

        // Synchronization for next merge iteration
        pthread_mutex_lock(&mutex_next);
        next_ready++;
        if (next_ready == num_threads) {
            next_ready = 0;
            pthread_cond_broadcast(&cond_next);
        } else {
            pthread_cond_wait(&cond_next, &mutex_next);
        }
        pthread_mutex_unlock(&mutex_next);
    }

    return NULL;
}
// Main program - set up list of random integers and use threads to sort the list
//
// Input: 
//	k = log_2(list size), therefore list_size = 2^k
//	q = log_2(num_threads), therefore num_threads = 2^q
//
int main(int argc, char *argv[]) {

    struct timespec start, stop, stop_qsort;
    double total_time, time_res, total_time_qsort;
    int k, q, j, error, i; 

    // Read input, validate
    if (argc != 3) {
	printf("Need two integers as input \n"); 
	printf("Use: <executable_name> <log_2(list_size)> <log_2(num_threads)>\n"); 
	exit(0);
    }
    k = atoi(argv[argc-2]);
    if ((list_size = (1 << k)) > MAX_LIST_SIZE) {
	printf("Maximum list size allowed: %d.\n", MAX_LIST_SIZE);
	exit(0);
    }; 
    q = atoi(argv[argc-1]);
    if ((num_threads = (1 << q)) > MAX_THREADS) {
	printf("Maximum number of threads allowed: %d.\n", MAX_THREADS);
	exit(0);
    }; 
    if (num_threads > list_size) {
	printf("Number of threads (%d) < list_size (%d) not allowed.\n", 
	   num_threads, list_size);
	exit(0);
    }; 

    // Allocate list, list_orig, and work

    list = (int *) malloc(list_size * sizeof(int));
    list_orig = (int *) malloc(list_size * sizeof(int));
    work = (int *) malloc(list_size * sizeof(int));

//
// VS: ... May need to initialize mutexes, condition variables, 
// VS: ... and their attributes
//

    // Initialize list of random integers; list will be sorted by 
    // multi-threaded parallel merge sort
    // Copy list to list_orig; list_orig will be sorted by qsort and used
    // to check correctness of multi-threaded parallel merge sort
    srand48(0); 	// seed the random number generator
    for (j = 0; j < list_size; j++) {
	list[j] = (int) lrand48();
	list_orig[j] = list[j];
    }
    // duplicate first value at last location to test for repeated values
    list[list_size-1] = list[0]; list_orig[list_size-1] = list_orig[0];

    // Create threads; each thread executes find_minimum
    clock_gettime(CLOCK_REALTIME, &start);

//
// VS: ... may need to initialize mutexes, condition variables, and their attributes
//

// Serial merge sort 
// VS: ... replace this call with multi-threaded parallel routine for merge sort
// VS: ... need to create threads and execute thread routine that implements 
// VS: ... parallel merge sort
    thread_data thread_data_array[num_threads];

    pthread_mutex_init(&lock_copy, NULL);
    pthread_mutex_init(&lock_next, NULL);
    pthread_mutex_init(&lock_start, NULL);
    pthread_cond_init(&cond_copy, NULL);
    pthread_cond_init(&cond_next, NULL);
    pthread_cond_init(&cond_start, NULL);

    for(i = 0; i < num_threads; i++){
        (thread_data_array[i]).index = i;
        (thread_data_array[i]).q = q;
        pthread_create(&p_threads[i], NULL, sort_list_parallel, &thread_data_array[i]);
    }

    for(i = 0; i < num_threads; i ++){
        pthread_join(p_threads[i], NULL);
    }
    // print_list(list, list_size);
    // sort_list(q);

    // Compute time taken
    clock_gettime(CLOCK_REALTIME, &stop);
    total_time = (stop.tv_sec-start.tv_sec)
	+0.000000001*(stop.tv_nsec-start.tv_nsec);

    // Check answer
    qsort(list_orig, list_size, sizeof(int), compare_int);
    clock_gettime(CLOCK_REALTIME, &stop_qsort);
    total_time_qsort = (stop_qsort.tv_sec-stop.tv_sec)
	+0.000000001*(stop_qsort.tv_nsec-stop.tv_nsec);
    // print_list(list_orig, list_size);
    error = 0; 
    for (j = 1; j < list_size; j++) {
	if (list[j] != list_orig[j]) error = 1; 
    }

    if (error != 0) {
	printf("Houston, we have a problem!\n"); 
    }
    
    // Print time taken
    printf("List Size = %d, Threads = %d, error = %d, time (sec) = %8.4f, qsort_time = %8.4f\n", 
	    list_size, num_threads, error, total_time, total_time_qsort);

// VS: ... destroy mutex, condition variables, etc.
    pthread_mutex_destroy(&lock_copy);
    pthread_mutex_destroy(&lock_next);
    pthread_mutex_destroy(&lock_start);
    pthread_cond_destroy(&cond_copy);
    pthread_cond_destroy(&cond_next);
    pthread_cond_destroy(&cond_start);

    free(list); free(work); free(list_orig); 

}


 
