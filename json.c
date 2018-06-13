#include <stdio.h>

#include "json.h"

const char *json_get_string(json_object *o, const char *key)
{
json_object *tmp;

if (json_object_object_get_ex(o, key, &tmp))
	return json_object_get_string(tmp);

return NULL;
}

int json_get_int(json_object *o, const char *key, int defval)
{
json_object *tmp;

if (json_object_object_get_ex(o, key, &tmp))
	return json_object_get_int(tmp);

return defval;
}

void json_dump(json_object *o)
{
json_object_object_foreach(o, key, val) {
	int val_type = json_object_get_type(val);
	printf("key: \"%s\", type of val: %d\n", key, val_type);
}
}
