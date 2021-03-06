#include "MediasoupEngine.hpp"
#include "Logger.hpp"
#include "EventEmitter.hpp"
#include "Promise.hpp"
#include "Worker.hpp"

using namespace promise;

namespace mediasoup
{

mediasoup::IMediasoupEngine* CreateMediasoupEngine() {
    MS_lOGGERI("CreateMediasoupEngine");
    return &mediasoup::MediasoupEngine::GetInstance();
}

void DestroyMediasoupEngine(mediasoup::IMediasoupEngine *engine) {
    if (!engine) {
        return;
    }
    MS_lOGGERI("DestroyMediasoupEngine");
}

MediasoupEngine::MediasoupEngine() {

}

MediasoupEngine::~MediasoupEngine() {
   
}

void MediasoupEngine::Test() {
    return;
    MS_lOGGERD("Support for int: {0:d};  hex: {0:x};  oct: {0:o}; bin: {0:b}", 42);
    MS_lOGGERW("Some error message with arg: {}", 1);
    MS_lOGGERE("Easy padding in numbers like {:08d}", 12);

    // test EventEmitter
    EventEmitter emitter;
    emitter.on(0, [](const char *text, int n) { 
        MS_lOGGERD("recv emitter evnet 0:{} {}", text, n);
    });

    MS_lOGGERD("emit evnet 0");
    emitter.emit(0, "from emit", 777);

    // test Promise
    newPromise([](Defer d){
        MS_lOGGERD("promise http request 1");
        int ret = 0;
        //ret = 1;
        if (0 == ret) {
            d.resolve();
        } else {
            d.reject(ret);
        }
    }).then([](){ // on resolve
        MS_lOGGERD("http request 1 success");
        Defer next = newPromise([](Defer d) {
            MS_lOGGERD("promise http request 2");
            int ret = 0;
            ret = 1;
            if (0 == ret) {
                d.resolve();
            } else {
                d.reject(ret);
            }
        });
        //Will call next.resole() or next.reject() later
        return next;
    }, [](int ret){ // on reject
        MS_lOGGERE("http request 1 failed:{}", ret);
    }).then([](){ // on resolve
        MS_lOGGERE("http request 2 success");
    }, [](int ret){ // on reject
        MS_lOGGERE("http request 2 failed:{}", ret);
    }).always([](){
        MS_lOGGERD("http request finish");
    });
}

void StaticWorkerFun(void *arg) {
    static_cast<MediasoupEngine*>(arg)->WorkerFun();
}

void StaticAsync(uv_async_t *handle) {
    static_cast<MediasoupEngine*>(handle->loop->data)->Async(handle);
}

bool MediasoupEngine::Init() {
    if (nullptr != m_workThread) {
        MS_lOGGERI("already Init");
        return true;
    }

    int ret = uv_thread_create(&m_workThread, StaticWorkerFun, this);
    if (0 != ret) {
        MS_lOGGERE("uv_thread_create failed {}", ret);
        return false;
    }
    
	return true;
}

void MediasoupEngine::Destroy()  {
    if (nullptr == m_workThread) {
        MS_lOGGERI("need Init first");
        return;
    }

    // notify quit
    uv_async_send(&m_async);

    MS_lOGGERI("wait work thread quit");
    uv_thread_join(&m_workThread);
    m_workThread = nullptr;
    MS_lOGGERI("work thread quit");
}

IWorker* MediasoupEngine::CreateWorker()  {
    return new Worker();
}

void MediasoupEngine::WorkerFun() {
    MS_lOGGERI("WorkerFun begine");

    uv_loop_t* loop = uv_default_loop();
    if (nullptr == loop) {
        MS_lOGGERE("create loop failed");
        return;
    }
    // save this
    loop->data = static_cast<void*>(this);

    uv_async_init(loop, &m_async, StaticAsync);

    MS_lOGGERI("uv_run");
    uv_run(loop, UV_RUN_DEFAULT);
    
    uv_loop_close(loop);
    MS_lOGGERI("WorkerFun end");
}

void MediasoupEngine::Async(uv_async_t *handle) {
    MS_lOGGERI("async stop");
    uv_stop(handle->loop);
}

}