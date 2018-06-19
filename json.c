#include <stdio.h>

#include "json.h"

const char *json_get_string(json_object *o, const char *key)
{
json_object *tmp;

if (json_object_object_get_ex(o, key, &tmp))
    if (json_object_is_type(o, json_type_string))
	return json_object_get_string(tmp);

return NULL;
}

int json_get_int(json_object *o, const char *key, int defval)
{
json_object *tmp;

if (json_object_object_get_ex(o, key, &tmp))
    if (json_object_is_type(o, json_type_int))
    	return json_object_get_int(tmp);

return defval;
}

array_list* json_get_array(json_object *o, const char *key)
{
json_object *tmp;

if (json_object_object_get_ex(o, key, &tmp))
    if (json_object_is_type(o, json_type_array))
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
int type=json_object_get_type(o);
printf("\nJSON Object: [%d], ", type);
json_dump_type(type);

switch (type) {
    case json_type_string:
        printf("String: %s\n", json_object_get_string(o));
    break;
    case json_type_int:
        printf("Int: %d\n", json_object_get_int(o));
    break;
    case json_type_double:
        printf("Double: %f\n", json_object_get_double(o));
    break;
    case json_type_boolean:
        printf("Bool: %d\n", json_object_get_boolean(o));
    break;
    case json_type_object: 
    {
        printf("Object: fields: %d\n", json_object_object_length(o));
        json_object_object_foreach(o, key, val) {
        	type = json_object_get_type(val);
	        printf("key: \"%s\", contents:\n", key);
            json_dump(val);
        }
        printf("--\n");
    }
    break;
    case json_type_array:
    {
        int l=json_object_array_length(o);
        printf("Array: items: %d\n", l);
        for (int x=0;x<l;x++  ) {
            json_object *a=json_object_array_get_idx(o, x);
            json_dump(a);
        }
        printf("--\n");
    }
    break;
    default:;
}    
}
