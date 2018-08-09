
=Table definitions for PostgreSQL + PostGIS

Enable PostGIS in your database:

 CREATE EXTENSION postgis;

Rack location table

 CREATE TABLE racks (
  id INT PRIMARY KEY,
  name VARCHAR,
  location GEOGRAPHY(Point,4326)
 );

Rack status table

 CREATE TABLE rack_status (
  id INT NOT NULL,
  dt TIMESTAMP NOT NULL,
  bikes INT NOT NULL,
  UNIQUE (id,dt)
 ); 
