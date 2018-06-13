#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <math.h>

#include <curl/curl.h>

#include "json.h"

#define MAX_BIKES 300
#define API_URL "http://data.foli.fi/citybike"

CURL *curl;

struct MemoryStruct {
  char *memory;
  size_t size;
};

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

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
  size_t realsize = size * nmemb;
  struct MemoryStruct *mem = (struct MemoryStruct *)userp;

  mem->memory = realloc(mem->memory, mem->size + realsize + 1);
  if(mem->memory == NULL) {
    /* out of memory! */ 
    printf("not enough memory (realloc returned NULL)\n");
    return 0;
  }

  memcpy(&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;

  return realsize;
}

void prepare(char *url, struct MemoryStruct *chunk)
{
curl = curl_easy_init();
curl_easy_setopt(curl, CURLOPT_URL, url);
curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)chunk);
}



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
printf("TkuFtop - %s, available %d, load %d %%\n\n", outstr, ri->bikes_total_avail, load(ri));
}

int get(char *url, struct MemoryStruct *chunk)
{
CURLcode res;

chunk->memory = malloc(8192);
chunk->size = 0;

prepare(url, chunk);
res=curl_easy_perform(curl);
if(res!=CURLE_OK) {
	fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
	curl_easy_cleanup(curl);
	return -1;
}
curl_easy_cleanup(curl);

//printf("Downloaded: %d\n", chunk.size);
//fprintf(stderr, "\n\n%s\n\n", chunk.memory);

return 0;
}

int follari_parse_response(struct MemoryStruct *chunk)
{
struct Racks ri;

json_object *obj = json_tokener_parse(chunk->memory);
if (!obj) {
	fprintf(stderr, "Invalid JSON\n");
	return -1;
}

if (!json_object_is_type(obj, json_type_object)) {
	fprintf(stderr, "JSON is not an object\n");
	return -1;
}

ri.racks_total=json_get_int(obj, "rack_total", 0);
ri.bikes_total_avail=json_get_int(obj, "bikes_total_avail", 0);
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
struct MemoryStruct chunk;

if (get(API_URL, &chunk)!=0)
    return -1;

follari_parse_response(&chunk);

free(chunk.memory);

return 0;
}

int main (int argc, char **argv)
{
curl_global_init(CURL_GLOBAL_DEFAULT);

while (follari_update()==0) {
	sleep(5);
}

curl_global_cleanup();
return 0;
}
