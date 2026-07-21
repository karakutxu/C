#ifndef TEST_COMPONENT_COMMON_H
#define TEST_COMPONENT_COMMON_H

#include "ifnet.h"
#include "proto_main.h"
#include "proto_statics_ng.h"

typedef struct {
    struct ifnet *ifp;
    proto_context_t ctx;
    struct proto_statics statics;
    struct pf_stats work_stats;
    struct pf_stats full_stats;
} test_fixture_t;

extern test_fixture_t g_fixture;

void component_setup(void);
void component_teardown(void);

#endif /* TEST_COMPONENT_COMMON_H */