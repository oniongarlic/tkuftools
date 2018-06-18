#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <math.h>

#include "http.h"
#include "json.h"

#define MAX_BIKES 300
#define API_URL "http://data.foli.fi/citybike"

struct Racks {
 int racks_total;
 int bikes_total_avail;
 time_t generated;
 time_t lastupdate;
};

struct Rack {
 const char *id;
 const char *stop_code;
 const char *operator;
 const char *name;
 time_t last_seen;
 double lon;
 double lat;
 int bikes_avail;
 int slots_total;
 int slots_avail;
};

static struct Racks ri;
static uint rentals=0;

void print_rack(json_object *o)
{
struct Rack rack;

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
}

void print_racks(json_object *racks)
{
printf("ID  Avail Slots       Name                     Flag\n");
json_object_object_foreach(racks, key, val) {
	print_rack(val);
}
}

int load(struct Racks *ri)
{
float t=((float)ri->bikes_total_avail/MAX_BIKES)*100.0f;

return round(100.0-t);
}

void print_header(struct Racks *ri)
{
struct tm *tmp;
tmp = localtime(&ri->lastupdate);
char outstr[40];

strftime(outstr, sizeof(outstr), "%F %T", tmp);

printf("\e[1;1H\e[2J");
printf("TkuFtop - %s, available %d, load %d%%, rentals %d\n\n", outstr, ri->bikes_total_avail, load(ri), rentals);
}

int follari_parse_response(json_object *obj)
{
int bikes;

if (!obj) {
	fprintf(stderr, "Invalid JSON object data\n");
	return -1;
}

if (!json_object_is_type(obj, json_type_object)) {
	fprintf(stderr, "JSON is not an object\n");
	return -1;
}

ri.racks_total=json_get_int(obj, "rack_total", 0);
bikes=json_get_int(obj, "bikes_total_avail", 0);

if (ri.bikes_total_avail>0 && ri.bikes_total_avail<bikes)
    rentals+=bikes-ri.bikes_total_avail;

ri.bikes_total_avail=bikes;

ri.generated=json_get_int(obj, "generated", 0);
ri.lastupdate=json_get_int(obj, "lastupdate", 0);

print_header(&ri);

json_object *racks;
if (json_object_object_get_ex(obj, "racks", &racks)) {
	print_racks(racks);
}

json_object_put(obj);

return 0;
}

int follari_update()
{
json_object *obj=http_get_json(API_URL);
if (!obj)
    return -1;

follari_parse_response(obj);

return 0;
}

int main (int argc, char **argv)
{
http_init();

ri.bikes_total_avail=-1;

while (follari_update()==0) {
	sleep(5);
}

http_deinit();

return 0;
}
