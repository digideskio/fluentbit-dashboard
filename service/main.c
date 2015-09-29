/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include "webservice.h"

DUDA_REGISTER("Fluent Bit", "Dashboard");

void cb_stats(duda_request_t *dr)
{
    response->http_status(dr, 200);
    response->printf(dr, "this is a test");
    response->end(dr, NULL);
}

int duda_main()
{
    map->static_add("/stats", "cb_stats");
    return 0;
}
