/* 
 * File:   PinyinCloudClient.h
 * Author: WU Jun <quark@lihdd.net>
 *
 * February 6, 2010
 *  created, reimplement original buggy c code (sogoupycc.{c,h})
 *  this class mantains multi-thread requests to remote server.
 *  a request is done via external script for flexibility.
 * as designed, it should instantiate a engine class.
 */

#ifndef _PinyinCloudClient_H
#define	_PinyinCloudClient_H

#include <dbus/dbus.h>
#include <glib.h>
#include <deque>
#include <string>
#include <pthread.h>

using std::deque;
using std::string;

typedef void (*ResponseCallbackFunc)(void*);
typedef string (*FetchFunc)(void*, const string&);

struct PinyinCloudRequest {
    bool responsed;
    string responseString;
    string requestString;
    ResponseCallbackFunc callbackFunc;
    FetchFunc fetchFunc;
    void* callbackParam, *fetchParam;
    unsigned int requestId;
};

class PinyinCloudClient {
public:
    PinyinCloudClient();
    virtual ~PinyinCloudClient();
    const size_t getRequestCount() const;
    const PinyinCloudRequest& getRequest(size_t index) const;
    void readLock();
    void readUnlock();
    void request(const string requestString, FetchFunc fetchFunc, void* fetchParam, ResponseCallbackFunc callbackFunc, void* callbackParam);
    void removeFirstRequest(int count = 1);
    void removeLastRequest();

private:
    PinyinCloudClient(const PinyinCloudClient& orig);
    friend void* requestThreadFunc(void *data);

    deque<PinyinCloudRequest> requests;
    pthread_rwlock_t requestsLock;
    unsigned int nextRequestId;
};


#endif	/* _PinyinCloudClient_H */

