#include "mqtt.h"

#include <string.h>
#include <stdio.h>

int mqtt_publish_info_topic_int(struct mosquitto *mqtt, const char *prefix, const char *topic, int value)
{
int r;
char ftopic[80];
char data[256];

snprintf(ftopic, sizeof(ftopic), "%s/%s", prefix, topic);
snprintf(data, sizeof(data), "%d", value);

r=mosquitto_publish(mqtt, NULL, ftopic, strlen(data), data, 0, false);
if (r!=MOSQ_ERR_SUCCESS)
	fprintf(stderr, "MQTT Publish for info [%s] failed with %s\n", topic, mosquitto_strerror(r));

return r;
}


