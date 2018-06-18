#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <math.h>

#include "http.h"
#include "json.h"

#define API_URL_BASE "http://data.foli.fi/siri/stops"

void print_stop(json_object *obj)
{
/*
rack.stop_code=json_get_string(o, "stop_code");
rack.name=json_get_string(o, "name");
rack.bikes_avail=json_get_int(o, "bikes_avail", -1);
rack.slots_total=json_get_int(o, "slots_total", -1);
rack.slots_avail=json_get_int(o, "slots_avail", -1);

printf("%-3s [%3d] [%3d /%3d] - %-30s", rack.stop_code, rack.bikes_avail, rack.slots_total, rack.slots_avail, rack.name);
if (rack.bikes_avail==0)
	printf("!");
else if (rack.bikes_avail<3)
	printf("*");

printf("\n");
*/

if (!obj) {
	fprintf(stderr, "Invalid JSON object data\n");
	return -1;
}

if (!json_object_is_type(obj, json_type_object)) {
    fprintf(stderr, "JSON is not an object: %d\n", json_object_get_type(obj));
	return -1;
}

json_dump(obj);

}

void print_stops(json_object *stops)
{
if (!stops) {
	fprintf(stderr, "Invalid JSON object data\n");
	return -1;
}
json_dump(stops);
printf("ID  Avail Slots       Name                     Flag\n");
json_object_object_foreach(stops, key, val) {
	print_stop(val);
}
}

void print_header(time_t *t)
{
struct tm *tmp;
tmp = localtime(t);
char outstr[40];

strftime(outstr, sizeof(outstr), "%F %T", tmp);

printf("\e[1;1H\e[2J");
printf("TkuFStop - %s\n", outstr);
}

int foli_parse_response(json_object *obj)
{
time_t t;

if (!obj) {
	fprintf(stderr, "Invalid JSON object data\n");
	return -1;
}

if (!json_object_is_type(obj, json_type_object)) {
	fprintf(stderr, "JSON is not an object: %d\n", json_object_get_type(obj));
	return -1;
}

t=json_get_int(obj, "servertime", 0);

print_header(&t);

json_dump(obj);

json_object *stop;
if (json_object_object_get_ex(obj, "result", &stop)) {
    json_dump(stop);
	print_stops(stop);
}

json_object_put(obj);

return 0;
}

int foli_stop_update(char *stop)
{
char *s;
int l;

l=strlen(API_URL_BASE)+strlen(stop)+2;
s=malloc(l);
snprintf(s, l, "%s/%s", API_URL_BASE, stop);

json_object *obj=http_get_json(s);
if (!obj)
    return -1;

foli_parse_response(obj);

free(s);

return 0;
}

int main (int argc, char **argv)
{
http_init();

foli_stop_update("T9");

http_deinit();
return 0;
}
