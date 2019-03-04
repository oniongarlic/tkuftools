#include <string.h>

#include "http.h"

static CURL *curl;

struct MemoryStruct {
  char *memory;
  size_t size;
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

static void http_prepare(const char *url, struct MemoryStruct *chunk)
{
curl = curl_easy_init();
curl_easy_setopt(curl, CURLOPT_URL, url);
curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
curl_easy_setopt(curl, CURLOPT_USERAGENT, "tkuftools/0.1 (https://www.tal.org/projects/tkuftools)");
curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)chunk);
}

static int http_get(const char *url, struct MemoryStruct *chunk)
{
long http_code=0;
CURLcode res;

chunk->memory = malloc(8192);
chunk->size = 0;

http_prepare(url, chunk);
res=curl_easy_perform(curl);

curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

if(res!=CURLE_OK) {
	fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
	curl_easy_cleanup(curl);
	return -1;
}
curl_easy_cleanup(curl);

//printf("Downloaded: %d\n", chunk.size);
//fprintf(stderr, "\n\n%s\n\n", chunk.memory);

if (http_code!=200)
    return http_code;

return 0;
}

json_object *http_get_json(const char *url)
{
struct MemoryStruct chunk;

if (http_get(url, &chunk)!=0)
    return NULL;

json_object *obj = json_tokener_parse(chunk.memory);
if (!obj) {
	fprintf(stderr, "Invalid JSON\n");
	return NULL;
}

free(chunk.memory);

return obj;
}

void http_init()
{
curl_global_init(CURL_GLOBAL_DEFAULT);
}

void http_deinit()
{
curl_global_cleanup();
}

