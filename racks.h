#ifndef _RACKS_H
#define _RACKS_H

#define RACKS_MAX (50)

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
 Rack data[RACKS_MAX];
} Racks;

void rack_fill_from_json(json_object *o, Rack *rack);
int racks_fill_from_json(json_object *o, Racks *ri);

#endif
