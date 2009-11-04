/*
 * File:   sogoupycc.c
 * Author: Jun WU <quark@lihdd.com>
 *
 * GNUv2 Licence
 *
 * Created on November 2, 2009
 */

#define _REENTRANT

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>

#include "sogoupycc.h"
#include "doublepinyin_scheme.h"

#define _SOGOUPYCC_REQUEST_QUEUE_ITEM_MAX 256

// simple protect when write sogoupycc_request_queue and
// sogoupycc_request_queue_back, sogoupycc_request_queue[]
// no read is protected
static pthread_mutex_t sogoupycc_request_queue_mutex;
static char * external_request_script;

struct sgpycc_request_t {
    int responsed;
    char *request_string;
    int response_item_count;
    char *response_string;
    commit_string_func callback_func;
    void* callback_para_first;
};

#define SOGOUPYCC_REQUEST_QUEUE_LOCK_BEGIN if(0)fprintf(stderr, "LOCK AT %d\n", __LINE__); pthread_mutex_lock(&sogoupycc_request_queue_mutex);
#define SOGOUPYCC_REQUEST_QUEUE_LOCK_END if(0)fprintf(stderr, "UNLOCK AT %d\n", __LINE__); pthread_mutex_unlock(&sogoupycc_request_queue_mutex);

// a queue for requests
static struct sgpycc_request_t sogoupycc_request_queue[_SOGOUPYCC_REQUEST_QUEUE_ITEM_MAX];
static int sogoupycc_request_queue_front = 0, sogoupycc_request_queue_back = 0;

/**
 * make specified callback invalid
 * if an ime engine handle become invalid (ie. exits), this is necessary.
 * @param count how many requests should be muted, if it is non-positive, then mute all
 */
void sogoupycc_mute_queued_requests_by_callback_para_first(void* callback_para_first, int count) {
    int i, muted_count = 0;
    for (i = sogoupycc_request_queue_front - 1;; i--) {
        if (i < 0) i = _SOGOUPYCC_REQUEST_QUEUE_ITEM_MAX - 1;
        if (sogoupycc_request_queue[i].callback_para_first == callback_para_first &&
                sogoupycc_request_queue[i].callback_func != 0) {
            sogoupycc_request_queue[i].callback_func =
                    sogoupycc_request_queue[i].callback_para_first = 0;
            if (++muted_count == count) break;
        };
        if (i == sogoupycc_request_queue_back) break;
    }
}

int sogoupycc_get_queued_request_count(void) {
    int r = (sogoupycc_request_queue_front + 2 * _SOGOUPYCC_REQUEST_QUEUE_ITEM_MAX - sogoupycc_request_queue_back)
            % _SOGOUPYCC_REQUEST_QUEUE_ITEM_MAX;
    return r;
}

int sogoupycc_get_queued_request_count_by_callback_para_first(void* callback_para_first) {
    int i, count = 0;
    for (i = sogoupycc_request_queue_back; i != sogoupycc_request_queue_front; i++) {
        if (i == _SOGOUPYCC_REQUEST_QUEUE_ITEM_MAX) i = 0;
        if (sogoupycc_request_queue[i].callback_para_first == callback_para_first) count++;
    }
    return count;
}

void sogoupycc_get_queued_request_string_begin(void) {
    // reserved for advanced lock
}

void sogoupycc_get_queued_request_string_end(void) {
    // reserved for advanced lock
}

/**
 * call this between sogoupycc_get_queued_request_string_begin() and
 * sogoupycc_get_queued_request_string_end()
 * @param index start from 0
 * @param *requesting if is still reauesting, set it to 1, otherwise 0
 * @return return a string, no need to free it. 0 if index out of range
 */
char* sogoupycc_get_queued_request_string_by_index(void* callback_para_first, int index, int *requesting) {
    int i, count = 0;

    for (i = sogoupycc_request_queue_back; i != sogoupycc_request_queue_front; i++) {
        if (i == _SOGOUPYCC_REQUEST_QUEUE_ITEM_MAX) i = 0;
        if (sogoupycc_request_queue[i].callback_para_first == callback_para_first) {
            if (count++ == index) {
                *requesting = 1 - sogoupycc_request_queue[i].responsed;
                if (sogoupycc_request_queue[i].responsed)
                    return sogoupycc_request_queue[i].response_string;
                else
                    return sogoupycc_request_queue[i].request_string;
            }
        }
    }
    return 0;
}

inline static void _sogoupycc_request_queue_pre_pop(void) {
    sogoupycc_request_queue[sogoupycc_request_queue_back].responsed = 0;
    if (++sogoupycc_request_queue_back == _SOGOUPYCC_REQUEST_QUEUE_ITEM_MAX)
        sogoupycc_request_queue_back = 0;
}

inline static void _sogoupycc_request_queue_post_pop(void) {
    if (--sogoupycc_request_queue_back == -1)
        sogoupycc_request_queue_back = _SOGOUPYCC_REQUEST_QUEUE_ITEM_MAX - 1;
    free(sogoupycc_request_queue[sogoupycc_request_queue_back].response_string);
    free(sogoupycc_request_queue[sogoupycc_request_queue_back].request_string);
    if (++sogoupycc_request_queue_back == _SOGOUPYCC_REQUEST_QUEUE_ITEM_MAX)
        sogoupycc_request_queue_back = 0;
}

static void* _sogoupycc_request_queue_fetch_thread(void *pindex) {
    int index = *((int*) pindex);
    if (sogoupycc_request_queue[index].responsed) return 0;

    // fetch via commandline
    int command_length = strlen(sogoupycc_request_queue[index].request_string) +
            strlen(external_request_script) + 7;
    char *command = malloc(command_length);

    snprintf(command, command_length, "%s '%s'", external_request_script,
            sogoupycc_request_queue[index].request_string);

    FILE* fp = popen(command, "r");

#define _SOUGOUPYCC_REQUEST_QUEUE_FETCH_RESPONSE_LENGTH_MAX 1024
    char response[_SOUGOUPYCC_REQUEST_QUEUE_FETCH_RESPONSE_LENGTH_MAX];

    fgets(response, _SOUGOUPYCC_REQUEST_QUEUE_FETCH_RESPONSE_LENGTH_MAX, fp);

    sogoupycc_request_queue[index].response_string = strdup(response);
    sogoupycc_request_queue[index].responsed = 1;

    fclose(fp);
#undef _SOUGOUPYCC_REQUEST_QUEUE_FETCH_RESPONSE_LENGTH_MAX

    // check and commit
    while (sogoupycc_get_queued_request_count() > 0 && sogoupycc_request_queue[sogoupycc_request_queue_back].responsed) {
        SOGOUPYCC_REQUEST_QUEUE_LOCK_BEGIN;

        int popping_index = sogoupycc_request_queue_back;
        _sogoupycc_request_queue_pre_pop();

        if (sogoupycc_request_queue[popping_index].callback_func) {
            (*sogoupycc_request_queue[popping_index].callback_func)
                    (sogoupycc_request_queue[popping_index].callback_para_first,
                    sogoupycc_request_queue[popping_index].response_string);
        }
        // cleanning and count
        _sogoupycc_request_queue_post_pop();
        SOGOUPYCC_REQUEST_QUEUE_LOCK_END;
    }
    free(pindex);
    pthread_exit(0);
}

static void _sogoupycc_request_queue_fetch_thread_create(int index) {
    pthread_t request_thread;

    int *queue_index = malloc(sizeof (int));
    *queue_index = index;

    int res = pthread_create(&request_thread, NULL, &_sogoupycc_request_queue_fetch_thread, (void*) queue_index);
    if (res != 0) {
        perror("request thread creation failed");
        exit(EXIT_FAILURE);
    };
}

void sogoupycc_commit_request(char* request_string, commit_string_func callback_func, void* callback_para_first) {
    // refuse commit if queue full
    if (sogoupycc_get_queued_request_count() == _SOGOUPYCC_REQUEST_QUEUE_ITEM_MAX - 1) return;
    int process_index = sogoupycc_request_queue_front;

    SOGOUPYCC_REQUEST_QUEUE_LOCK_BEGIN;
    // push into queue
    sogoupycc_request_queue[sogoupycc_request_queue_front].request_string = strdup(request_string);
    sogoupycc_request_queue[sogoupycc_request_queue_front].responsed = 0;
    sogoupycc_request_queue[sogoupycc_request_queue_front].callback_func = callback_func;
    sogoupycc_request_queue[sogoupycc_request_queue_front].callback_para_first = callback_para_first;
    if (++sogoupycc_request_queue_front == _SOGOUPYCC_REQUEST_QUEUE_ITEM_MAX)
        sogoupycc_request_queue_front = 0;
    SOGOUPYCC_REQUEST_QUEUE_LOCK_END;

    // process it
    _sogoupycc_request_queue_fetch_thread_create(process_index);

    SOGOUPYCC_REQUEST_QUEUE_LOCK_BEGIN;
    if (sogoupycc_request_queue_front == _SOGOUPYCC_REQUEST_QUEUE_ITEM_MAX)
        sogoupycc_request_queue_front = 0;
    SOGOUPYCC_REQUEST_QUEUE_LOCK_END;
}

void sogoupycc_commit_string(char *string, commit_string_func callback_func, void* callback_para_first, int process_now) {
    if (sogoupycc_get_queued_request_count() == _SOGOUPYCC_REQUEST_QUEUE_ITEM_MAX - 1) return;
    int process_index = sogoupycc_request_queue_front;

    SOGOUPYCC_REQUEST_QUEUE_LOCK_BEGIN;
    sogoupycc_request_queue[sogoupycc_request_queue_front].request_string = strdup("");
    sogoupycc_request_queue[sogoupycc_request_queue_front].response_string = strdup(string);
    sogoupycc_request_queue[sogoupycc_request_queue_front].responsed = 1;
    sogoupycc_request_queue[sogoupycc_request_queue_front].callback_func = callback_func;
    sogoupycc_request_queue[sogoupycc_request_queue_front].callback_para_first = callback_para_first;
    if (++sogoupycc_request_queue_front == _SOGOUPYCC_REQUEST_QUEUE_ITEM_MAX)
        sogoupycc_request_queue_front = 0;
    SOGOUPYCC_REQUEST_QUEUE_LOCK_END;

    // process it
    if (process_now)
        _sogoupycc_request_queue_fetch_thread_create(process_index);
}


#ifndef PKGDATADIR
#define PKGDATADIR "/usr/share/ibus-sogoupycc"
#endif

void sogoupycc_init(void) {
    int res;

    external_request_script = getenv("SOUGOUPYCC_REQUEST_SCRIPT");
    if (external_request_script == NULL) {
        external_request_script = (char*) malloc(sizeof (PKGDATADIR "/sgccget.sh "));
        sprintf(external_request_script, "%s/sgccget.sh", PKGDATADIR);
    }

    res = pthread_mutex_init(&sogoupycc_request_queue_mutex, NULL);
    if (res != 0) {
        perror("mutex initialization failed");
        exit(EXIT_FAILURE);
    }
}

void sogoupycc_exit(void) {
    pthread_mutex_destroy(&sogoupycc_request_queue_mutex);
    if (external_request_script != getenv("SOUGOUPYCC_REQUEST_SCRIPT")) {
        free(external_request_script);
    }
}
