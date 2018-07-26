#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <math.h>

#include <termios.h>
#include <sys/select.h>

#include <signal.h>

#include "http.h"
#include "json.h"

#define MAX_BIKES 300
#define MAX_RACKS 50
#define API_URL "http://data.foli.fi/citybike"

typedef struct {
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
} Rack;

typedef struct {
 int racks_total;
 int bikes_total_avail;
 time_t generated;
 time_t lastupdate;
 uint rentals;
 uint returns;
 Rack data[MAX_RACKS];
} Racks;

enum SortOrder {
  SORT_NONE=0,
  SORT_BIKES,
  SORT_STOP_CODE,
  SORT_NAME
} sort_order=SORT_NONE;

enum OpModes {
  MODE_TOP=0,
  MODE_ONESHOT,
  MODE_CSV
} opmode=MODE_TOP;

static Racks ri;

static int loop_done=0;

static int cmp_rack_stop_code(const void * a, const void * b)
{
Rack *aa=(Rack *)a;
Rack *bb=(Rack *)b;

return strverscmp(aa->stop_code, bb->stop_code);
}

static int cmp_rack_name(const void * a, const void * b)
{
Rack *aa=(Rack *)a;
Rack *bb=(Rack *)b;

return strverscmp(aa->name, bb->name);
}

static int cmp_rack_bikes(const void * a, const void * b)
{
Rack *aa=(Rack *)a;
Rack *bb=(Rack *)b;

if (aa->bikes_avail>bb->bikes_avail)
	return 1;
else if (aa->bikes_avail>bb->bikes_avail)
	return -1;

return 0;
}

void fill_rack(json_object *o, Rack *rack)
{
rack->stop_code=json_get_string(o, "stop_code");
rack->name=json_get_string(o, "name");
rack->bikes_avail=json_get_int(o, "bikes_avail", -1);
rack->slots_total=json_get_int(o, "slots_total", -1);
rack->slots_avail=json_get_int(o, "slots_avail", -1);
rack->lat=json_get_double(o, "lat", 0);
rack->lon=json_get_double(o, "lon", 0);
}

void print_rack(Rack *r)
{
printf("%-3s [%3d] [%3d /%3d] - %-30s", r->stop_code, r->bikes_avail, r->slots_total, r->slots_avail, r->name);
if (r->bikes_avail==0)
	printf("!");
else if (r->bikes_avail<3)
	printf("*");

printf("\n");
}

void print_rack_csv(Rack *r)
{
printf("%s,%d,%d,%d,%f,%f,%s\n", r->stop_code, r->bikes_avail, r->slots_total, r->slots_avail, r->lat, r->lon, r->name);
}

void print_racks_csv(Racks *ri)
{
int x;

printf("ID,Available,SlotsTotal,SlotsAvailable,Lat,Lon,Name\n");
for(x=0;x<ri->racks_total;x++)
	print_rack_csv(&ri->data[x]);
}

int fill_racks(json_object *o, Racks *ri)
{
int i=0;
json_object_object_foreach(o, key, val) {
	(void)key;
	fill_rack(val, &ri->data[i]);
	i++;
	/* Check limits */
	if (i>MAX_RACKS-1) {
		return i-1;
	}
}

return i;
}


void print_racks(Racks *ri)
{
int x;

printf("ID  Avail Slots       Name                     Flag\n");

for(x=0;x<ri->racks_total;x++)
	print_rack(&ri->data[x]);

}

int load(Racks *ri)
{
float t=((float)ri->bikes_total_avail/MAX_BIKES)*100.0f;

return round(100.0-t);
}

void print_header(Racks *ri)
{
struct tm *tmp;
tmp = localtime(&ri->lastupdate);
char outstr[40];

strftime(outstr, sizeof(outstr), "%F %T", tmp);

printf("\e[1;1H\e[2J");
printf("TkuFtop - %s, available %d, load %d%%\nrentals %d, returns %d\n\n", outstr, ri->bikes_total_avail, load(ri), ri->rentals, ri->returns);
}

int follari_parse_response(json_object *obj)
{
int bikes;
int rt=0;

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
    ri.rentals+=bikes-ri.bikes_total_avail;
else if (ri.bikes_total_avail>0 && ri.bikes_total_avail>bikes)
    ri.returns+=ri.bikes_total_avail-bikes;

ri.bikes_total_avail=bikes;

ri.generated=json_get_int(obj, "generated", 0);
ri.lastupdate=json_get_int(obj, "lastupdate", 0);

json_object *racks;
if (json_object_object_get_ex(obj, "racks", &racks)) {
	rt=fill_racks(racks, &ri);
}

ri.racks_total=rt;

switch (sort_order) {
	case SORT_STOP_CODE:
		qsort(ri.data, ri.racks_total, sizeof(Rack), cmp_rack_stop_code);
	break;
	case SORT_BIKES:
		qsort(ri.data, ri.racks_total, sizeof(Rack), cmp_rack_name);
		qsort(ri.data, ri.racks_total, sizeof(Rack), cmp_rack_bikes);
	break;
	case SORT_NAME:
		qsort(ri.data, ri.racks_total, sizeof(Rack), cmp_rack_name);
	break;
	default:;
}

if (opmode==MODE_CSV) {
	print_racks_csv(&ri);
} else {
	print_header(&ri);
	print_racks(&ri);
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

void action_term(int signum)
{
loop_done=1;
}

void main_loop()
{
static struct termios oldt, newt;
struct timeval tv;
fd_set rfds;
struct sigaction action;

memset(&action, 0, sizeof(action));
action.sa_handler = action_term;
sigaction(SIGTERM, &action, NULL);

tcgetattr( STDIN_FILENO, &oldt);

newt=oldt;
newt.c_lflag &= ~(ICANON | ECHO);

tcsetattr( STDIN_FILENO, TCSANOW, &newt);

while (loop_done==0) {
	if (follari_update()!=0)
		break;

	FD_ZERO(&rfds);
	FD_SET(STDIN_FILENO, &rfds);

	tv.tv_sec = 5;
	tv.tv_usec = 0;

	int r=select(1, &rfds, NULL, NULL, &tv);
	if (r==-1) {
		perror("select");
		break;
	}
	if (FD_ISSET(0, &rfds)) {
		char c=getchar();
		switch (c) {
		case 'q':
			loop_done=1;
 		break;
		case 's':
			sort_order=SORT_STOP_CODE;
		break;
		case 'b':
			sort_order=SORT_BIKES;
		break;
		case 'n':
			sort_order=SORT_NAME;
		break;
		default:;
			fprintf(stderr, "Unknown command: %c!\n", c);sleep(1);
		break;
		}
	}
}

tcsetattr( STDIN_FILENO, TCSANOW, &oldt);
}

int main (int argc, char **argv)
{
int opt;

while ((opt = getopt(argc, argv, "cos:")) != -1) {
    switch (opt) {
    case 's':
	if (strcmp(optarg, "stop")==0)
		sort_order=SORT_STOP_CODE;
	else if (strcmp(optarg, "bikes")==0)
		sort_order=SORT_BIKES;
	else if (strcmp(optarg, "name")==0)
		sort_order=SORT_NAME;
	else {
	        fprintf(stderr, "Valid sort options are: stop,bikes,name\n");
		exit(1);
	}
    break;
    case 'o':
	opmode=MODE_ONESHOT;
    break;
    case 'c':
	opmode=MODE_CSV;
    break;
    default:
        fprintf(stderr, "Usage: %s [-o] [-s order] [-c] %o\n", argv[0], opt);
        exit(1);
    }
}


http_init();

ri.bikes_total_avail=-1;
ri.rentals=0;
ri.returns=0;

switch (opmode) {
	case MODE_TOP:
		main_loop();
	break;
	case MODE_CSV:
	case MODE_ONESHOT:
		follari_update();
	break;
}

http_deinit();

return 0;
}
