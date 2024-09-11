#pragma once
#include "http.h"

void callback_redirect(struct response_t *res, const char *location);

void callback400(struct response_t *res, struct request_t *req);
void callback404(struct response_t *res, struct request_t *req);
void callback405(struct response_t *res, struct request_t *req);

