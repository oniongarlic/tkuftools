#include <curl/curl.h>

#include "json.h"

void http_init(void);
void http_deinit(void);

json_object * http_get_json(const char *url);
