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
    DEBUG_PRINT(2, "[CLOUD] enter request thread\n");
    PinyinCloudRequest *request = ((RequestThreadData*) data)->newRequest;
    PinyinCloudClient *client = ((RequestThreadData*) data)->client;
    delete (RequestThreadData*) data;

    DEBUG_PRINT(3, "[CLOUD.REQTHREAD] prepare to call fetch func\n");

    // this may takes time
    string responseString = request->fetchFunc(request->fetchParam, request->requestString);

    DEBUG_PRINT(4, "[CLOUD.REQTHREAD] waiting to wrie back response: %s\n", responseString.c_str());
    // write response back

    pthread_rwlock_wrlock(&client->requestsLock);


    DEBUG_PRINT(4, "[CLOUD.REQTHREAD] writing response: %s\n", responseString.c_str());
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
                DEBUG_PRINT(4, "[CLOUD.REQTHREAD] prepare execute callback\n");
                (*request->callbackFunc)(request->callbackParam);
            }

            delete request;
            DEBUG_PRINT(3, "[CLOUD.REQTHREAD] Exiting...\n");
            pthread_exit(0);
        }
    }


    // not found from list (this could happen if user call remove request...)
    // in this case, just do nothing
    DEBUG_PRINT(3, "[CLOUD.REQTHREAD] request invalid. ignore\n");
    pthread_rwlock_unlock(&client->requestsLock);
    delete request;

    DEBUG_PRINT(3, "[CLOUD.REQTHREAD] Exiting...\n");
    pthread_exit(0);
}

void PinyinCloudClient::request(const string requestString, FetchFunc fetchFunc, void* fetchParam, ResponseCallbackFunc callbackFunc, void* callbackParam) {

    // ignore empty string request
    if (requestString.length() == 0) return;

    DEBUG_PRINT(2, "[CLOUD] new request: %s\n", requestString.c_str());
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

    DEBUG_PRINT(4, "[CLOUD.REQUEST] going to create thread\n");
    // launch thread
    pthread_t request_thread;
    pthread_attr_t requestThreadAttr;
    int ret;

    pthread_attr_init(&requestThreadAttr);
    pthread_attr_setdetachstate(&requestThreadAttr, PTHREAD_CREATE_DETACHED);

    ret = pthread_create(&request_thread, &requestThreadAttr, &requestThreadFunc, (void*) data);
    pthread_attr_destroy(&requestThreadAttr);
    DEBUG_PRINT(1, "[CLOUD.REQUEST] new thread: 0x%x\n", (int) request_thread);

    if (ret != 0) {
        perror("[ERROR] can not create request thread");
        // find and remove that request (no reverse iterator, better ways?)

        pthread_rwlock_wrlock(&requestsLock);
        for (deque<PinyinCloudRequest>::iterator it = requests.begin(); it != requests.end(); ++it) {
            if (it->requestId == request->requestId) {
                requests.erase(it);
                break;
            }
        }
        pthread_rwlock_unlock(&requestsLock);


        delete request;
        delete data;
    };
    // request and data will be deleted in requestThread.
}

PinyinCloudClient::PinyinCloudClient() {
    DEBUG_PRINT(1, "[CLOUD] Init\n");
    
    nextRequestId = 0;
    pthread_rwlock_init(&requestsLock, NULL);
}

void PinyinCloudClient::readLock() {

    DEBUG_PRINT(2, "[CLOUD] Read lock\n");
    pthread_rwlock_rdlock(&requestsLock);
}

void PinyinCloudClient::readUnlock() {
    DEBUG_PRINT(2, "[CLOUD] Read unlock\n");
    pthread_rwlock_unlock(&requestsLock);
}

const size_t PinyinCloudClient::getRequestCount() const {

    DEBUG_PRINT(3, "[CLOUD] getRequestCount: %d\n", requests.size());
    return requests.size();
}

const PinyinCloudRequest& PinyinCloudClient::getRequest(size_t index) const {

    DEBUG_PRINT(5, "[CLOUD] getRequest # %d\n", index);
    return requests[index];
}

void PinyinCloudClient::removeFirstRequest(int count) {
    if (count > 0) {
        DEBUG_PRINT(3, "[CLOUD] Remove first %d request\n", count);
        pthread_rwlock_wrlock(&requestsLock);
        for (int i = 0; i < count; ++i) {
            if (requests.size() > 0) requests.pop_front();
            else break;
        }
        pthread_rwlock_unlock(&requestsLock);
    }
}

void PinyinCloudClient::removeLastRequest() {
    DEBUG_PRINT(3, "[CLOUD] Remove last request\n");
    pthread_rwlock_wrlock(&requestsLock);
    if (requests.size() > 0) requests.pop_back();
    pthread_rwlock_unlock(&requestsLock);
}

PinyinCloudClient::PinyinCloudClient(const PinyinCloudClient& orig) {
}

PinyinCloudClient::~PinyinCloudClient() {

    DEBUG_PRINT(1, "[CLOUD] Destroy\n");
    pthread_rwlock_destroy(&requestsLock);
}

