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

array_list* json_get_array(json_object *o, const char *key)
{
json_object *tmp;

if (json_object_object_get_ex(o, key, &tmp))
	return json_object_get_array(tmp);

return NULL;
}

void json_dump_type(int type)
{
    switch (type) {
    case json_type_null: printf("json_type_null\n");
    break;
    case json_type_boolean: printf("json_type_boolean\n");
    break;
    case json_type_double: printf("json_type_double\n");
    break;
    case json_type_int: printf("json_type_int\n");
    break;
    case json_type_object: printf("json_type_object\n");
    break;
    case json_type_array: printf("json_type_array\n");
    break;
    case json_type_string: printf("json_type_string\n");
    break;
    }
}

void json_dump(json_object *o)
{
int type = json_object_get_type(o);
printf("type of object: [%d] ", type);
json_dump_type(type);

if (type==json_type_object) {
    json_object_object_foreach(o, key, val) {
    	type = json_object_get_type(val);
	    printf("key: \"%s\", type of val: [%d] ", key, type);
        json_dump_type(type);
    }
} else if (type==json_type_array) {
    int l=json_object_array_length(o);
    printf("items: %d ", l);
}
}
