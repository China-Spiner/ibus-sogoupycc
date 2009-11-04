/* 
 * File:   sogoupycc.h
 * Author: Jun WU <quark@lihdd.com>
 *
 * GNUv2 Licence
 *
 * Created on November 3, 2009
 */

#ifndef _SOGOUPYCC_H
#define	_SOGOUPYCC_H

#ifdef	__cplusplus
extern "C" {
#endif
    
typedef void (*commit_string_func)(void*, const char *);

int sogoupycc_get_queued_request_count(void);
int sogoupycc_get_queued_request_count_by_callback_para_first(void* callback_para_first);
void sogoupycc_get_queued_request_string_begin(void);
void sogoupycc_get_queued_request_string_end(void);
char* sogoupycc_get_queued_request_string_by_index(void* callback_para_first, int index, int *requesting);
void sogoupycc_mute_queued_requests_by_callback_para_first(void* callback_para_first, int count);
void sogoupycc_commit_request(char* request_string, commit_string_func callback_func, void* callback_para_first);
void sogoupycc_commit_string(char *string, commit_string_func callback_func, void* callback_para_first, int process_now);
void sogoupycc_init(void);
void sogoupycc_exit(void);

#ifdef	__cplusplus
}
#endif

#endif	/* _SOGOUPYCC_H */

