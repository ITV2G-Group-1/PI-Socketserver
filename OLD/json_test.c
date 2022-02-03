#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <json-c/json.h>
#include <time.h>

// gcc cJsonTest.c -std=c99 -ljson-c

// time ex: "2019-03-10 02:55:05"

// {"uuid":"489377d75d1d15247320","data":[{"type":"temperature","value":22.61000061,"timestamp":1643818772},{"type":"light","value":15,"timestamp":1643818773},{"type":"temperature","value":22.61000061,"timestamp":1643818775},{"type":"light","value":15,"timestamp":1643818776},{"type":"temperature","value":22.60000038,"timestamp":1643818778},{"type":"light","value":15,"timestamp":1643818779},{"type":"temperature","value":22.59000015,"timestamp":1643818781}]}}]}
// char *json_string = "{\"uuid\":\"489377d75d1d15247320\",\"data\":[{\"type\":\"temperature\",\"value\":22.61000061,\"timestamp\":1643818772},{\"type\":\"light\",\"value\":15,\"timestamp\":1643818773},{\"type\":\"temperature\",\"value\":22.61000061,\"timestamp\":1643818775},{\"type\":\"light\",\"value\":15,\"timestamp\":1643818776},{\"type\":\"temperature\",\"value\":22.60000038,\"timestamp\":1643818778},{\"type\":\"light\",\"value\":15,\"timestamp\":1643818779},{\"type\":\"temperature\",\"value\":22.59000015,\"timestamp\":1643818781}]}";

int json_reader(char *json_string) {
	// fprintf(stdout, "%s\n", json_string);
	struct json_object *parsed_json;
	struct json_object *uuid;
	struct json_object *data;
	struct json_object *datapoint;
	struct json_object *type;
	struct json_object *value;
	struct json_object *gps_long;
	struct json_object *gps_lat;
	struct json_object *timestamp;
	
	size_t n_data;
	size_t i;
	
	char c_type[20];
	
	const char time_format[] = "%Y-%m-%d %X";
	time_t t;
	struct tm lt;
	char time[32];
	
	parsed_json = json_tokener_parse(json_string);
	
	json_object_object_get_ex(parsed_json, "uuid", &uuid);
	json_object_object_get_ex(parsed_json, "data", &data);
	
	fprintf(stdout, "uuid: %s\n", json_object_get_string(uuid));
	
	n_data = json_object_array_length(data);
	fprintf(stdout, "Found %d datapoints\n", n_data);
	
	for (i = 0; i < n_data; i++) {
		datapoint = json_object_array_get_idx(data, i);
		
		json_object_object_get_ex(datapoint, "type", &type);
		json_object_object_get_ex(datapoint, "timestamp", &timestamp);
		
		t = json_object_get_double(timestamp);
		lt = *localtime(&t);
		strftime(time, sizeof(time), time_format, &lt);
		
		strcpy(c_type, json_object_get_string(type));
		if (strcmp(c_type, "temperature") == 0) {
			json_object_object_get_ex(datapoint, "value", &value);
			
			fprintf(stdout, "type: %s \t", c_type);
			fprintf(stdout, "value: %.8f \t", json_object_get_double(value));
			fprintf(stdout, "time: %s\n", time);
		} else if (strcmp(c_type,"light") == 0) {
			json_object_object_get_ex(datapoint, "value", &value);
			
			fprintf(stdout, "type: %s \t\t", c_type);
			fprintf(stdout, "value: %d \t\t", json_object_get_int(value));
			fprintf(stdout, "time: %s\n", time);
		} else if (strcmp(c_type, "gps")) {
			json_object_object_get_ex(datapoint, "long", &gps_long);
			json_object_object_get_ex(datapoint, "lat", &gps_lat);
			
			fprintf(stdout, "type: %s \t\t", c_type);
			fprintf(stdout, "long: %.9f \t", json_object_get_double(gps_long));
			fprintf(stdout, "lat: %.8f \t", json_object_get_double(gps_lat));
			fprintf(stdout, "time: %s\n", time);
		} else {
			fprintf(stderr, "Error occured trying to find type\n");
			return 1;
		}
	}
	return 0;
}
