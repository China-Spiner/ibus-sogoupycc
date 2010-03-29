/* 
 * File:   PinyinCloudClient.h
 * Author: WU Jun <quark@lihdd.net>
 *
 * this class mantains multi-thread requests to remote server.
 * a request is done via callback function (external script)
 * for flexibility.
 * 
 * as designed, it should be instantiated per engine session.
 */

#ifndef _PinyinCloudClient_H
#define	_PinyinCloudClient_H

#include <deque>
#include <string>
#include <vector>
#include <pthread.h>

using std::deque;
using std::vector;
using std::string;

typedef void (*ResponseCallbackFunc)(void*);
typedef string(*FetchFunc)(void*, const string&);

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

    /**
     * perform a read lock before calling this.
     */
    const size_t getRequestCount() const;
    /**
     * perform a read lock op before calling this.
     */
    const PinyinCloudRequest& getRequest(size_t index) const;

    /**
     * lock for reading purpose
     * this should not take a long time until readUnlock() !
     */
    void readLock();
    void unlock();

    /**
     * push a request to request queue, start a sub process to fetch result
     * callbackFunc can be NULL, fetchFunc can't
     */
    void request(const string requestString, FetchFunc fetchFunc, void* fetchParam, ResponseCallbackFunc callbackFunc, void* callbackParam);
    static void preRequest(const string requestString, FetchFunc fetchFunc, void* fetchParam, ResponseCallbackFunc callbackFunc, void* callbackParam);
    void removeFirstRequest(int count = 1);
    void removeLastRequest();
    vector<PinyinCloudRequest> exportAndRemoveAllRequest();
    
private:
    /**
     *  this is private and should not be used.
     */
    PinyinCloudClient(const PinyinCloudClient& orig);
    friend void* preRequestThreadFunc(void *data);
    friend void* requestThreadFunc(void *data);

    deque<PinyinCloudRequest> requests;
    pthread_rwlock_t requestsLock;
    unsigned int nextRequestId;
    static bool preRequestBusy;
};


#endif	/* _PinyinCloudClient_H */

