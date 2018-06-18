#include <curl/curl.h>

#include "json.h"

void http_init();
void http_deinit();

json_object * http_get_json(char *url);
