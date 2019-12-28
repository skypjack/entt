#ifndef ENTT_LIB_EMITTER_PLUGIN_PROXY_H
#define ENTT_LIB_EMITTER_PLUGIN_PROXY_H

#include "types.h"

struct proxy: emitter_proxy {
    proxy(test_emitter &);
    void publish(message) override;
    void publish(event) override;

private:
    test_emitter *emitter;
};

#endif
