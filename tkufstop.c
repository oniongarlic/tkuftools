#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>
#include <math.h>

#include <signal.h>

#include "http.h"
#include "json.h"

#define API_URL_BASE "http://data.foli.fi/siri/stops"

typedef struct {
 const char *lineref;
 const char *originref;
 const char *destinationref;
 const char *destinationdisplay;
 int vehicleatstop;
 int incongestion;
 time_t destinationaimedarrivaltime;
 time_t aimedarrivaltime;
 time_t expectedarrivaltime;
 time_t aimeddeparturetime;
 time_t expecteddeparturetime;
} StopData;

static int max_departures=0;
static int loop_done=0;

enum OpModes {
  MODE_TOP=0,
  MODE_ONESHOT,
} opmode=MODE_ONESHOT;


/* XXX: Not perfect in any way... but for now
 *
 * Valid are (from GTFS data):
 * 1-9004
 * L2, L3, L4, Ltulo
 * PT1, PT2, PT4, PT5, PT6
 * T1 - T10, T22, T24, T32-T42
 */
int validate_stop(const char *stop)
{
long int ts;

if (strlen(stop)==0)
    return 0;

if (strlen(stop)>5)
    return 0;

if (stop[0]=='L') {
    if (strcmp(stop, "Ltulo")==0)
        return 1;
    ts=strtol(stop+1, NULL, 10);
    if (ts==2 || ts==3 || ts==4)
        return 1;
    return 0;
}

if (stop[0]=='P' && stop[1]=='T') {
    ts=strtol(stop+2, NULL, 10);
    if (ts==1 || ts==2 || ts==4 || ts==5 || ts==6)
        return 1;
    return 0;
}

if (stop[0]=='T') {
    ts=strtol(stop+1, NULL, 10);
    if (ts>=1 && ts<=10)
        return 1;
    if (ts==22 && ts==24)
        return 1;
    if (ts>=32 && ts<=42)
        return 1;
    return 0;
}

ts=strtol(stop, NULL, 10);
if (ts>=1 && ts<=9004)
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
StopData s;

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

s.destinationaimedarrivaltime=json_get_int(o, "destinationaimedarrivaltime", -1);

s.vehicleatstop=json_get_bool(o, "vehicleatstop", 0);
s.incongestion=json_get_bool(o, "incongestion", 0);

printf("%-5s", s.lineref);
if (s.vehicleatstop==1)
	printf("* ");
else
	printf("  ");
if (s.incongestion==1)
	printf("! ");
else
	printf("  ");

print_time(&s.expecteddeparturetime);

printf(" - ");

print_time(&s.destinationaimedarrivaltime);

printf(" %s", s.destinationdisplay);

printf("\n");
}

void print_stops(json_object *stops)
{
int x,l;

if (!stops) {
	fprintf(stderr, "Invalid JSON object data\n");
}
// json_dump(stops);
l=json_object_array_length(stops);

// Limit departures output to first max_departures
if (max_departures>0 && l>max_departures)
	l=max_departures;

for (x=0;x<l;x++) {
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
//      123451212 11:22:33 D
printf("Line @ C Time       At Destination\n");
}

int foli_parse_response(json_object *obj, const char *stop)
{
const char *status;
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

foli_parse_response(obj, stop);

free(s);

return 0;
}

int foli_stops_update(char *stops[], int n)
{
for (int i=0;i<n;i++) {
	if (foli_stop_update(stops[i])!=0)
        	return 1;
	printf("\n");
}
return 0;
}

void action_term(int signum)
{
loop_done=1;
}

int main_loop_stop(char *stops[], int n)
{
struct sigaction action;

memset(&action, 0, sizeof(action));
action.sa_handler = action_term;
sigaction(SIGTERM, &action, NULL);

while (loop_done==0) {
	printf("\e[1;1H\e[2J");

	if (foli_stops_update(stops, n)!=0)
		return 1;

	sleep(5);
}
return 0;
}

#define MAX_STOPS (10)

int main (int argc, char **argv)
{
char *stop;
char *stops[MAX_STOPS];
int rows=0,ret=0,opt,runmode=MODE_ONESHOT,i=0;

if (argc<2) {
    fprintf(stderr, "Usage: tkfstop stoprefs [-r rows] [-o] [-t]\n");
    return 1;
}

stop=argv[1];

char *p=strtok (stop, ",");
while (p!=NULL) {
    if (validate_stop(p)!=1) {
        fprintf(stderr, "Invalid stop ref [%s] given\n", p);
        return 1;
    }
    stops[i]=p;
    if (i>MAX_STOPS-1)
        break;
    i++;
    p=strtok (NULL, ",");
}

/* Skip stops */
optind=2;

while ((opt = getopt(argc, argv, "otr:")) != -1) {
    switch (opt) {
    case 't':
        opmode=MODE_TOP;
    break;
    case 'o':
        opmode=MODE_ONESHOT;
    break;
    case 'r':
	rows=atoi(optarg);
	if (rows<0)
        	rows=0;
	max_departures=rows;
    break;
    }
}

http_init();

switch (opmode) {
	case MODE_TOP:
	ret=main_loop_stop(stops, i);
	break;
	case MODE_ONESHOT:
	default:
	if (foli_stops_update(stops, i)!=0) {
		fprintf(stderr, "Invalid stop or no data\n");
		ret=2;
	}
}

http_deinit();
return ret;
}

