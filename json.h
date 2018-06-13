#include <json-c/json.h>

const char *json_get_string(json_object *o, const char *key);
int json_get_int(json_object *o, const char *key, int defval);

void json_dump(json_object *o);
