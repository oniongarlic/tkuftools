#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <math.h>

#include <termios.h>
#include <sys/select.h>

#include <signal.h>

#include <mosquitto.h>

#include "http.h"
#include "json.h"
#include "racks.h"

#define MAX_BIKES 300
#define API_URL "http://data.foli.fi/citybike"

enum SortOrder {
  SORT_NONE=0,
  SORT_BIKES,
  SORT_STOP_CODE,
  SORT_NAME
} sort_order=SORT_NONE;

enum OpModes {
  MODE_TOP=0,
  MODE_CSV,
  MODE_SQL,
  MODE_SQL_GEO,
  MODE_MQTT
} opmode=MODE_TOP;

enum RunModes {
  MODE_CONTINUOUS=0,
  MODE_ONESHOT
} runmode=MODE_CONTINUOUS;

static Racks ri;

static int loop_done=0;

static struct mosquitto *mqtt = NULL;
const char *mqtt_host=NULL;
const char *mqtt_clientid=NULL;
const char *mqtt_topic_prefix=NULL;

static int cmp_rack_stop_code(const void * a, const void * b)
{
const Rack *aa=(const Rack *)a;
const Rack *bb=(const Rack *)b;

return strverscmp(aa->stop_code, bb->stop_code);
}

static int cmp_rack_name(const void * a, const void * b)
{
const Rack *aa=(const Rack *)a;
const Rack *bb=(const Rack *)b;

return strverscmp(aa->name, bb->name);
}

static int cmp_rack_bikes(const void * a, const void * b)
{
const Rack *aa=(const Rack *)a;
const Rack *bb=(const Rack *)b;

if (aa->bikes_avail>bb->bikes_avail)
	return 1;
else if (aa->bikes_avail>bb->bikes_avail)
	return -1;

return 0;
}

static int load(Racks *ri)
{
float t=((float)ri->bikes_total_avail/MAX_BIKES)*100.0f;

return round(100.0-t);
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

void print_rack_sql_geo(Rack *r)
{
printf("INSERT INTO racks (id,name,location) VALUES (%s, '%s', ST_GeographyFromText('SRID=4326;POINT(%f %f)'));\n",
	r->stop_code, r->name, r->lon, r->lat);
}

void print_racks_sql_geo(Racks *ri)
{
int x;

for(x=0;x<ri->racks_total;x++)
	print_rack_sql_geo(&ri->data[x]);

}

void print_rack_sql(Rack *r)
{
struct tm *tmp=localtime(&r->last_seen);
char outstr[40];

strftime(outstr, sizeof(outstr), "%F %T", tmp);

printf("INSERT INTO rack_status (id,dt,bikes) VALUES (%s,'%s',%d);\n", r->stop_code, outstr, r->bikes_avail);
}

void print_racks_sql(Racks *ri)
{
int x;

for(x=0;x<ri->racks_total;x++) {
	print_rack_sql(&ri->data[x]);
}
}

void print_racks(Racks *ri)
{
int x;

printf("ID  Avail Slots       Name                     Flag\n");

for(x=0;x<ri->racks_total;x++)
	print_rack(&ri->data[x]);

}

int mqtt_publish_info_topic_int(const char *topic, int value)
{
int r;
char ftopic[80];
char data[256];

snprintf(ftopic, sizeof(ftopic), "%s/%s", mqtt_topic_prefix, topic);
snprintf(data, sizeof(data), "%d", value);

r=mosquitto_publish(mqtt, NULL, ftopic, strlen(data), data, 0, false);
if (r!=MOSQ_ERR_SUCCESS)
	fprintf(stderr, "MQTT Publish for info [%s] failed with %s\n", topic, mosquitto_strerror(r));

return r;
}

void mqtt_publish_info(Racks *ri)
{
mqtt_publish_info_topic_int("total", ri->bikes_total_avail);
mqtt_publish_info_topic_int("load", load(ri));
mqtt_publish_info_topic_int("rentals", ri->rentals);
mqtt_publish_info_topic_int("returns", ri->returns);
}

int mqtt_publish_rack(Rack *rack)
{
return mqtt_publish_info_topic_int(rack->stop_code, rack->bikes_avail);
}

void mqtt_publish_racks(Racks *ri)
{
uint x;

mqtt_publish_info(ri);

for(x=0;x<ri->racks_total;x++) {
	mqtt_publish_rack(&ri->data[x]);
	mosquitto_loop(mqtt, 100, 1);
}
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
        if (!json_object_is_type(racks, json_type_object)) {
                fprintf(stderr, "\"racks\" is not an object\n");
                return -1;
        }
	rt=racks_fill_from_json(racks, &ri);
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

switch (opmode) {
	case MODE_CSV:
		print_racks_csv(&ri);
	break;
	case MODE_SQL:
		print_racks_sql(&ri);
	break;
	case MODE_SQL_GEO:
		print_racks_sql_geo(&ri);
	break;
	case MODE_MQTT:
		mqtt_publish_racks(&ri);
	break;
	default:
		print_header(&ri);
		print_racks(&ri);
	break;
}

json_object_put(obj);

return 0;
}

int follari_update(void)
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

void main_loop(void)
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


void main_loop_simple(void)
{
struct sigaction action;

memset(&action, 0, sizeof(action));
action.sa_handler = action_term;
sigaction(SIGTERM, &action, NULL);

while (loop_done==0) {
	if (follari_update()!=0)
		break;
	sleep(5);
}

}

void mqtt_log_callback(struct mosquitto *m, void *userdata, int level, const char *str)
{
fprintf(stderr, "[MQTT-%d] %s\n", level, str);
}

void main_loop_mqtt(void)
{
int port = 1883;
int keepalive = 120;
bool clean_session = true;

printf("MQTT Mode: Host: '%s' ID: '%s' Tprefix: '%s'\n", mqtt_host, mqtt_clientid, mqtt_topic_prefix);

mqtt=mosquitto_new(mqtt_clientid, clean_session, NULL);

mosquitto_log_callback_set(mqtt, mqtt_log_callback);

if (mosquitto_connect(mqtt, mqtt_host, port, keepalive)) {
	fprintf(stderr, "Unable to connect.\n");
	goto mqtt_out;
}

while (1) {
	if (follari_update()!=0)
		break;

	mosquitto_loop(mqtt, 1000, 1);
	sleep(9);
}

mqtt_out:;

mosquitto_destroy(mqtt);
}

int main (int argc, char **argv)
{
int opt;

while ((opt = getopt(argc, argv, "qgmcos:t:h:i:")) != -1) {
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
	runmode=MODE_ONESHOT;
    break;
    case 'c':
	opmode=MODE_CSV;
    break;
    case 'q':
	opmode=MODE_SQL;
    break;
    case 'g':
	opmode=MODE_SQL_GEO;
	runmode=MODE_ONESHOT;
    break;
    case 'm':
	opmode=MODE_MQTT;
	mqtt_host="localhost";
	mqtt_topic_prefix="citybike/turku";
	mqtt_clientid="";
    break;
    case 'h':
	mqtt_host=optarg;
    break;
    case 't':
	mqtt_topic_prefix=optarg;
    break;
    case 'i':
	mqtt_clientid=optarg;
    break;
    default:
        fprintf(stderr, "Usage: %s\n\n", argv[0]);
        fprintf(stderr, " -s order 	Sort by order (stop,bikes,name)\n");
        fprintf(stderr, " -o 		Oneshot mode, display current status and exit\n");
        fprintf(stderr, " -c 		Output rack info as CSV\n");
        fprintf(stderr, " -q 		Output rack info as SQL INSERT commands\n");
        fprintf(stderr, " -g 		Output rack Geo locations as SQL INSERT commands\n");
        fprintf(stderr, " -m 		MQTT mode\n");
        fprintf(stderr, "  -h 		MQTT host\n");
        fprintf(stderr, "  -t 		MQTT topic prefix\n");
        fprintf(stderr, "  -i 		MQTT client id\n");
        exit(1);
    }
}

mosquitto_lib_init();
http_init();

ri.bikes_total_avail=-1;
ri.rentals=0;
ri.returns=0;

if (runmode==MODE_CONTINUOUS) {
	switch (opmode) {
	case MODE_SQL:
	case MODE_CSV:
		main_loop_simple();
	break;
	case MODE_TOP:
		main_loop();
	break;
	case MODE_MQTT:
		main_loop_mqtt();
	break;
	default:
	        fprintf(stderr, "Mode not supported\n");
	break;
	}
} else if (runmode==MODE_ONESHOT) {
	switch (opmode) {
	case MODE_SQL:
	case MODE_SQL_GEO:
	case MODE_CSV:
	case MODE_TOP:
		follari_update();
	break;
	default:
	        fprintf(stderr, "Mode not supported\n");
	break;
	}
}


http_deinit();
mosquitto_lib_cleanup();

return 0;
}
