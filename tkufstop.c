#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>
#include <math.h>

#include "http.h"
#include "json.h"

#define API_URL_BASE "http://data.foli.fi/siri/stops"

struct StopData {
 const char *lineref;
 const char *originref;
 const char *destinationref;
 const char *destinationdisplay;
 int vehicleatstop;
 time_t aimedarrivaltime;
 time_t expectedarrivaltime;
 time_t aimeddeparturetime;
 time_t expecteddeparturetime;
};

/* XXX: Not perfect in any way... but for now
 */
int validate_stop(const char *stop)
{
long int ts;

if (strlen(stop)==0)
    return 0;

if (strlen(stop)>4)
    return 0;

if (stop[0]=='T' || stop[0]=='P') {
    ts=strtol(stop+1, NULL, 10);
    if (ts>0 && ts<99) /* XXX */
        return 1;
}

ts=strtol(stop, NULL, 10); 
if (ts>0 && ts<9999) /* XXX */
    return 1;

return 0;
}

void print_datetime(time_t *t)
{
struct tm *tmp;
tmp = localtime(t);
char outstr[40];

strftime(outstr, sizeof(outstr), "%F %T", tmp);
printf("%s", outstr);
}

void print_time(time_t *t)
{
struct tm *tmp;
tmp = localtime(t);
char outstr[40];

strftime(outstr, sizeof(outstr), "%T", tmp);
printf("%s", outstr);
}

void print_stop(json_object *o)
{
struct StopData s;

if (!o) {
	fprintf(stderr, "Invalid JSON object data\n");
	return;
}

if (!json_object_is_type(o, json_type_object)) {
    fprintf(stderr, "JSON is not an object: %d\n", json_object_get_type(o));
	return; 
}

s.lineref=json_get_string(o, "lineref");
s.originref=json_get_string(o, "originref");
s.destinationref=json_get_string(o, "destinationref");
s.destinationdisplay=json_get_string(o, "destinationdisplay");

s.aimedarrivaltime=json_get_int(o, "aimedarrivaltime", -1);
s.expectedarrivaltime=json_get_int(o, "expectedarrivaltime", -1);

s.aimeddeparturetime=json_get_int(o, "aimeddeparturetime", -1);
s.expecteddeparturetime=json_get_int(o, "expecteddeparturetime", -1);

s.vehicleatstop=json_get_bool(o, "vehicleatstop", 0);

printf("%-5s", s.lineref);
if (s.vehicleatstop==1)
	printf("* ");
else
	printf("  ");

print_time(&s.expecteddeparturetime);

printf(" %s", s.destinationdisplay);

printf("\n");
}

void print_stops(json_object *stops)
{
if (!stops) {
	fprintf(stderr, "Invalid JSON object data\n");
}
// json_dump(stops);
int l=json_object_array_length(stops);
for (int x=0;x<l;x++  ) {
json_object *a=json_object_array_get_idx(stops, x);
	print_stop(a);
}
}

void print_header(time_t *t, const char *stop)
{
struct tm *tmp;
tmp = localtime(t);
char outstr[40];

strftime(outstr, sizeof(outstr), "%F %T", tmp);

//printf("\e[1;1H\e[2J");
printf("TkuFStop - %s %s\n", stop, outstr);
}

int foli_parse_response(json_object *obj, const char *stop)
{
char *status;
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
status=json_get_string(obj, "status");

if (strcmp(status,"OK")!=0) {
	fprintf(stderr, "Server status not OK\n");
	return -1;   
}

print_header(&t, stop);

json_object *s;
if (json_object_object_get_ex(obj, "result", &s)) {
	print_stops(s);
}

json_object_put(obj);

return 0;
}

int foli_stop_update(const char *stop)
{
char *s;
int l;

l=strlen(API_URL_BASE)+strlen(stop)+2;
s=malloc(l);
snprintf(s, l, "%s/%s", API_URL_BASE, stop);

json_object *obj=http_get_json(s);
if (!obj)
    return -1;

foli_parse_response(obj, stop);

free(s);

return 0;
}

int main (int argc, char **argv)
{
const char *stop;

if (argc!=2) {
    fprintf(stderr, "Usage: tkfstop stopref\n");
    return 1;
}

stop=argv[1];
if (validate_stop(stop)!=1) {
    fprintf(stderr, "Invalid stop ref\n");
    return 1;
}

http_init();

if (foli_stop_update(stop)!=0) {
    fprintf(stderr, "Invalid stop or no data\n");
    return 2;
}

http_deinit();
return 0;
}

