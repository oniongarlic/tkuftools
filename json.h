#ifndef _JSON_H
#define _JSON_H

#include <json-c/json.h>

const char *json_get_string(json_object *o, const char *key);
int json_get_int(json_object *o, const char *key, int defval);
double json_get_double(json_object *o, const char *key, double defval);
int json_get_bool(json_object *o, const char *key, int defval);
array_list* json_get_array(json_object *o, const char *key);

void json_dump(json_object *o);

#endif
