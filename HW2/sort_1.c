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
void* sort_list_parallel(void* data) {
    thread_data* my_data = (thread_data*)data;  // my_data
    int my_thread_id = my_data->index;  // Thread ID
    int my_merge_level = my_data->q;    // Current merge level
    int my_segment_size = list_size / num_threads;  // Segment size per thread
    int my_segment_start = my_thread_id * my_segment_size;  // Start of segment
    int my_segment_end = (my_thread_id + 1) * my_segment_size;  // End of segment
    int my_current_start = my_segment_start;  // Current start for merging
    int my_current_end = my_segment_end;      // Current end for merging
    int my_index;  // Index for loop

    // Sort local list
    qsort(&list[my_segment_start], my_segment_size, sizeof(int), compare_int);

    // Synchronization for start phase
    pthread_mutex_lock(&lock_start);
    start_count++;
    if (start_count < num_threads) {
        pthread_cond_wait(&cond_start, &lock_start);
    } else {
        start_count = 0;
        pthread_cond_broadcast(&cond_start);
    }
    pthread_mutex_unlock(&lock_start);

    // Merge at each level
    while (my_merge_level > 0) {
        if ((my_thread_id & 1) == 0) {
            // Left half merge
            for (my_index = my_segment_start; my_index < my_segment_end; my_index++) {
                int my_binary_result = binary_search_lt(list[my_index], list, my_current_start + my_segment_size, my_current_end + my_segment_size);
                my_binary_result -= (my_current_start + my_segment_size);
                work[my_index + my_binary_result] = list[my_index];
            }
            my_current_end += my_segment_size;
        } else {
            // Right half merge
            for (my_index = my_segment_start; my_index < my_segment_end; my_index++) {
                int my_binary_result = binary_search_le(list[my_index], list, my_current_start - my_segment_size, my_current_end - my_segment_size);
                my_binary_result -= (my_current_start - my_segment_size);
                work[my_index - my_segment_size + my_binary_result] = list[my_index];
            }
            my_current_start -= my_segment_size;
        }

        my_thread_id >>= 1; // Division by 2 using bitwise right shift
        my_segment_size *= 2;
        my_merge_level--;

        // Synchronization for copy phase
        pthread_mutex_lock(&lock_copy);
        copy_count++;
        if (copy_count < num_threads) {
            pthread_cond_wait(&cond_copy, &lock_copy);
        } else {
            copy_count = 0;
            pthread_cond_broadcast(&cond_copy);
        }
        pthread_mutex_unlock(&lock_copy);

        // Copy the work array to the main list
        for (my_index = my_segment_start; my_index < my_segment_end; my_index++) {
            list[my_index] = work[my_index];
        }

        // Synchronization for next iteration
        pthread_mutex_lock(&lock_next);
        next_count++;
        if (next_count < num_threads) {
            pthread_cond_wait(&cond_next, &lock_next);
        } else {
            next_count = 0;
            pthread_cond_broadcast(&cond_next);
        }
        pthread_mutex_unlock(&lock_next);
    }

    return NULL;
}

void sort_list(int q) {
    int i, level, my_id; 
    int np, my_list_size; 
    int ptr[num_threads + 1];

    np = list_size / num_threads; // Sub list size

    // Initialize starting position for each sublist
    for (my_id = 0; my_id < num_threads; my_id++) {
        ptr[my_id] = my_id * np;
    }
    ptr[num_threads] = list_size;

    // Sort local lists
    for (my_id = 0; my_id < num_threads; my_id++) {
        my_list_size = ptr[my_id + 1] - ptr[my_id];
        qsort(&list[ptr[my_id]], my_list_size, sizeof(int), compare_int);
    }

    // Sort list
    for (level = 0; level < q; level++) {
        for (my_id = 0; my_id < num_threads; my_id++) {
            int my_blk_size = np << level; // Use left shift instead of multiplication
            int my_own_blk = my_id & ~(1 << level); // Use bitwise operation for own block
            int my_own_idx = ptr[my_own_blk];

            int my_search_blk = my_id ^ (1 << level);
            int my_search_idx = ptr[my_search_blk];
            int my_search_idx_max = my_search_idx + my_blk_size;

            int my_write_blk = my_id >> (level + 1);
            int my_write_idx = ptr[my_write_blk];

            int idx = my_search_idx;
            int my_search_count = 0;

            // Binary search for 1st element
            if (my_search_blk > my_own_blk) {
                idx = binary_search_lt(list[ptr[my_id]], list, my_search_idx, my_search_idx_max);
            } else {
                idx = binary_search_le(list[ptr[my_id]], list, my_search_idx, my_search_idx_max);
            }
            my_search_count = idx - my_search_idx;
            int i_write = my_write_idx + my_search_count + (ptr[my_id] - my_own_idx);
            work[i_write] = list[ptr[my_id]];

            // Linear search for subsequent elements
            for (i = ptr[my_id] + 1; i < ptr[my_id + 1]; i++) {
                if (my_search_blk > my_own_blk) {
                    while ((list[i] > list[idx]) && (idx < my_search_idx_max)) {
                        idx++;
                        my_search_count++;
                    }
                } else {
                    while ((list[i] >= list[idx]) && (idx < my_search_idx_max)) {
                        idx++;
                        my_search_count++;
                    }
                }
                i_write = my_write_idx + my_search_count + (i - my_own_idx);
                work[i_write] = list[i];
            }
        }

        // Copy work into list for next iteration
        for (my_id = 0; my_id < num_threads; my_id++) {
            for (i = ptr[my_id]; i < ptr[my_id + 1]; i++) {
                list[i] = work[i];
            }
        }
    }
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


 