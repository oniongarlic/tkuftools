#include "json.h"
#include "racks.h"

int rack_fill_from_json(json_object *o, Rack *rack)
{
rack->stop_code=json_get_string(o, "stop_code");
if (!rack->stop_code)
	return -1;
rack->name=json_get_string(o, "name");
rack->last_seen=json_get_int(o, "last_seen", 0);
rack->bikes_avail=json_get_int(o, "bikes_avail", -1);
rack->slots_total=json_get_int(o, "slots_total", -1);
rack->slots_avail=json_get_int(o, "slots_avail", -1);
rack->lat=json_get_double(o, "lat", 0);
rack->lon=json_get_double(o, "lon", 0);

return 0;
}

int racks_fill_from_json(json_object *o, Racks *ri)
{
int i=0;
json_object_object_foreach(o, key, val) {
	(void)key;
	if (rack_fill_from_json(val, &ri->data[i])==0)
		i++;
	/* Check limits */
	if (i>RACKS_MAX-1) {
		return i-1;
	}
}

return i;
}
