/* 
 * File:   PinyinCloudClient.cpp
 * Author: WU Jun <quark@lihdd.net>
 *
 * see .h file for changelog
 */

#include "PinyinCloudClient.h"
#include <cstring>
#include <unistd.h>
#include <cstdlib>
#include <cstdio>
#include <dirent.h>
#include <sys/wait.h>
#include "defines.h"

using std::deque;

/**
 * data passed to thread which will fetch result
 */
struct RequestThreadData {
    PinyinCloudRequest* newRequest;
    PinyinCloudClient* client;
};

void* requestThreadFunc(void *data) {
    PinyinCloudRequest *request = ((RequestThreadData*) data)->newRequest;
    PinyinCloudClient *client = ((RequestThreadData*) data)->client;
    delete (RequestThreadData*) data;

    // this may takes time
    string responseString = request->fetchFunc(request->fetchParam, request->requestString);

    // write response back
    pthread_rwlock_wrlock(&client->requestsLock);

    // find that request according to request id (any better ways?)
    for (deque<PinyinCloudRequest>::reverse_iterator it = client->requests.rbegin(); it != client->requests.rend(); ++it) {
        if (it->requestId == request->requestId) {
            // set it to 'responsed'
            it->responseString = responseString;
            it->responsed = true;
            pthread_rwlock_unlock(&client->requestsLock);
            // callback, cleanning and done
            // note that unlock before callback
            if (request->callbackFunc) {
                (*request->callbackFunc)(request->callbackParam);
            }
            delete request;
            pthread_exit(0);
        }
    }
    // not found from list (this could happen if user call remove request...)
    // in this case, just do nothing
    pthread_rwlock_unlock(&client->requestsLock);
    delete request;

    pthread_exit(0);
}

/**
 * push a request to request queue, start a sub process to fetch result
 * callbackFunc can be NULL, fetchFunc can't
 */
void PinyinCloudClient::request(const string requestString, FetchFunc fetchFunc, void* fetchParam, ResponseCallbackFunc callbackFunc, void* callbackParam) {
    // ignore empty string request
    if (requestString.length() == 0) return;

    PinyinCloudRequest *request = new PinyinCloudRequest;
    request->requestString = requestString;
    request->callbackFunc = callbackFunc;
    request->callbackParam = callbackParam;
    request->requestId = (nextRequestId++);
    request->responsed = false;
    request->fetchFunc = fetchFunc;
    request->fetchParam = fetchParam;

    RequestThreadData *data = new RequestThreadData;
    data->newRequest = request;
    data->client = this;

    // push into queue first, lock down request for writing
    pthread_rwlock_wrlock(&requestsLock);
    requests.push_back(*request);
    pthread_rwlock_unlock(&requestsLock);

    // launch thread
    pthread_t request_thread;
    int ret;
    ret = pthread_create(&request_thread, NULL, &requestThreadFunc, (void*) data);
    if (ret != 0) {
        perror("can not create request thread");
        delete request;
        delete data;
    };
    // request and data will be deleted in requestThread.
}

PinyinCloudClient::PinyinCloudClient() {
    nextRequestId = 0;
    pthread_rwlock_init(&requestsLock, NULL);
}

/**
 * read lock
 * this should not take a long time until readUnlock() !
 */
void PinyinCloudClient::readLock() {
    pthread_rwlock_rdlock(&requestsLock);
}

void PinyinCloudClient::readUnlock() {
    pthread_rwlock_unlock(&requestsLock);
}

/**
 * perform a read lock op before calling this.
 */
const size_t PinyinCloudClient::getRequestCount() const {
    return requests.size();
}

/**
 * perform a read lock op before calling this.
 */
const PinyinCloudRequest& PinyinCloudClient::getRequest(size_t index) const {
    return requests[index];
}

void PinyinCloudClient::removeFirstRequest() {
    pthread_rwlock_wrlock(&requestsLock);
    if (requests.size() > 0) requests.pop_front();
    pthread_rwlock_unlock(&requestsLock);
}

void PinyinCloudClient::removeLastRequest() {
    pthread_rwlock_wrlock(&requestsLock);
    if (requests.size() > 0) requests.pop_back();
    pthread_rwlock_unlock(&requestsLock);
}

/**
 *  this is private and should not be used.
 */
PinyinCloudClient::PinyinCloudClient(const PinyinCloudClient& orig) {
}

PinyinCloudClient::~PinyinCloudClient() {
    pthread_rwlock_destroy(&requestsLock);
}

