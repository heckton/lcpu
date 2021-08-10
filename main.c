#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <stdint.h>
#include <time.h>
#include <math.h>
#include <stddef.h>
#include <string.h>

#define DEF_MEMSIZE 1024

enum MAIN_RET_CODES {
	SUCCESS,
	USAGE_ERR,
	MEMORY_ERR,
	TIME_ERR,
	THREAD_ERR
};

struct load_info {
	double time;
	char *memory;
	size_t memsize;
	struct timespec begin;
};

static void terminate_threads(const pthread_t *const threads, const size_t n);
static void *load_thread(void *arg);

int main(int argc, char *argv[])
{
	int ret = SUCCESS;
	size_t n_threads;
	double time;
	char *time_end;
	char *memory;
	size_t memsize = DEF_MEMSIZE;
	pthread_t *threads;
	struct load_info linfo;
	size_t i;

	/* Read parameters */
	if (argc < 3 || argc > 4 || sscanf(argv[1], "%zu", &n_threads) != 1 ||
			n_threads <= 0 ||
			(time = strtod(argv[2], &time_end)) <= 0 ||
			(argc > 3 &&  (sscanf(argv[3], "%zu", &memsize) != 1 ||
				memsize <= 0))) {
		fprintf(stderr, "Usage: %s threads time [memory]\n",
			argv[0]);
		ret = USAGE_ERR;
		goto OUT;
	}

	/* Allocations */
	if (!(threads = malloc(n_threads * sizeof *threads))) {
		fprintf(stderr, "Allocation failed: Not enough memory\n");
		ret = MEMORY_ERR;
		goto OUT;
	}
	if (!(memory = malloc(memsize))) {
		fprintf(stderr, "Allocation failed: Not enough memory\n");
		ret = MEMORY_ERR;
		goto OUT_FREE_THREADS;
	}

	/* Load begins */

	/* Set info about load on thread */
	linfo.time = time;
	linfo.memory = memory;
	linfo.memsize = memsize;
	if (clock_gettime(CLOCK_REALTIME, &linfo.begin)) {
		perror("Time getting failed");
		ret = TIME_ERR;
		goto OUT_FREE_MEMORY;
	}
	/* Run threads */
	for (i = 0; i < n_threads; i++) {
		const int create_res = pthread_create(threads + i, NULL,
				load_thread, &linfo);
		if (create_res) {
			errno = create_res;
			perror("Thread creation failed");
			/* Terminate all created threads */
			terminate_threads(threads, i);
			ret = THREAD_ERR;
			goto OUT_FREE_MEMORY;
		}
	}
	/* Waiting for terminating of all threads */
	for (i = 0; i < n_threads; i++) {
		int load_res, join_res;
		join_res = pthread_join(threads[i], (void *)&load_res);
		if (join_res) {
			errno = join_res;
			perror("Thread joining failed");
			/* Terminate all remaining threads */
			terminate_threads(threads + i + 1, n_threads - i - 1);
			ret = THREAD_ERR;
			goto OUT_FREE_MEMORY;
		}
		if (load_res) {
			errno = (int)(intptr_t)load_res;
			perror("Time getting failed");
			/* Terminate all remaining threads */
			terminate_threads(threads + i + 1, n_threads - i - 1);
			ret = TIME_ERR;
			goto OUT_FREE_MEMORY;
		}
	}

OUT_FREE_MEMORY:
	free(memory);
OUT_FREE_THREADS:
	free(threads);
OUT:
	return ret;
}

/*
 * Terminate all threads from array.
 * Attention: function ignore all errors!
 */
static void terminate_threads(const pthread_t *const threads, const size_t n)
{
	size_t i;
	for (i = 0; i < n; i++) {
		/* Ignore return values */
		pthread_cancel(threads[i]);
		pthread_join(threads[i], NULL);
	}
}

/*
 * Load one thread for "time" seconds.
 * Argument: pointer to struct load_info
 */
static void *load_thread(void *arg)
{
	const struct load_info *info = arg;
	double isec;
	long nsec = modf(info->time, &isec) * 1e9;
	struct timespec cur;
	size_t i = 0;
	do {
		memset(info->memory, i, info->memsize);
		i++;
		if (clock_gettime(CLOCK_REALTIME, &cur)) {
			pthread_exit((void *)(intptr_t)errno);
		}
		/* while curr - begin < time */
	} while (cur.tv_sec - info->begin.tv_sec < isec ||
			(cur.tv_sec - info->begin.tv_sec == (time_t)isec &&
				cur.tv_nsec - info->begin.tv_nsec <= nsec));
	pthread_exit(0);
}
