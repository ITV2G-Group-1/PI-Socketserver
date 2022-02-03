#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <json-c/json.h>
#include <mysql/mysql.h>

#define json_get_ex(datapoint, var, result) (json_object_object_get_ex(datapoint, var, result))
#define json_get_id(array, i) (json_object_array_get_idx(array, i))
#define json_length(array) (json_object_array_length(array))
#define json_stringify(var) (json_object_get_string(var))
#define json_intify(var) (json_object_get_int(var))
#define json_doublify(var) (json_object_get_double(var))

#define PORT 17021

int create_socket();
int connect_to_database();
int create_database();
int sql_err();
int insert_gps_data(char type[12], double *gps_long, double *gps_lat, char time[32]);
int insert_temp_data(char type[12], double *temp, char time[18]);
int insert_light_data(char type[12], double *light_intensity, char time[32]);
int json_reader(char *json_string);

static const char time_format[] = "%Y-%m-%d %X";
static const char *server = "localhost";
static const char *user = "main";
static const char *passwd = "";
static const char *database = "testdb";

MYSQL *con;

// {"uuid":"489377d75d1d15247320","data":[{"type":"temperature","value":22.61000061,"timestamp":1643818772},{"type":"light","value":15,"timestamp":1643818773},{"type":"temperature","value":22.61000061,"timestamp":1643818775},{"type":"light","value":15,"timestamp":1643818776},{"type":"temperature","value":22.60000038,"timestamp":1643818778},{"type":"light","value":15,"timestamp":1643818779},{"type":"temperature","value":22.59000015,"timestamp":1643818781}]}}]}

char *json_string = "{\"uuid\":\"489377d75d1d15247320\",\"data\":[{\"type\":\"temperature\",\"value\":22.61000061,\"timestamp\":1643818772},{\"type\":\"light\",\"value\":15,\"timestamp\":1643818773},{\"type\":\"temperature\",\"value\":22.61000061,\"timestamp\":1643818775},{\"type\":\"light\",\"value\":15,\"timestamp\":1643818776},{\"type\":\"temperature\",\"value\":22.60000038,\"timestamp\":1643818778},{\"type\":\"light\",\"value\":15,\"timestamp\":1643818779},{\"type\":\"temperature\",\"value\":22.59000015,\"timestamp\":1643818781}]}";

int main(int argc, char const *argv[]) {
	con = mysql_init(NULL);
}

int sql_err() {
	fprintf(stderr, "%s\n", mysql_error(con));
	if (con != NULL) {
		mysql_close(con);
	}
	return(1);
}

int insert_gps_data(char type[12], double *gps_long, double *gps_lat, char time[32]) {

}

int insert_temp_data(char type[12], double *temp, char time[32]) {

}

int insert_light_data(char type[12], double *light_intensity, char time[32]) {

}

int json_reader(char *json_string) {
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

    char c_type[12];
    int i_light;
    double d_temp;
    double d_long;
    double d_lat;

    time_t t;
	struct tm lt;
	char time[32];

    parsed_json = json_tokener_parse(json_string);

    json_get_ex(parsed_json, "uuid", &uuid);
    if (!uuid) {
        fprintf(stderr, "Error occured while trying to get \"uuid\" from: \n%s\n", json_string);
        return 1;
    }
    fprintf(stdout, "uuid: %s\n", json_object_get_string(uuid));

	json_get_ex(parsed_json, "data", &data);
    if (!data) {
        fprintf(stderr, "Error occured while trying to get \"data\" from: \n%s\n", json_string);
        return 1;
    }

    n_data = json_length(data);
    fprintf(stdout, "Found %d datapoints\n", n_data);
    for (i = 0; i < n_data; i++) {
        datapoint = json_get_id(data, i);

        json_get_ex(datapoint, "type", &type);
        if (!type) {
            fprintf(stderr, "Error occured while trying to get \"type\" at datapoint %d from: \n%s\n", i, json_string);
            break;
        }

        json_get_ex(datapoint, "timestamp", &timestamp);
        if (!timestamp) {
            fprintf(stderr, "Error occured while trying to get \"timestamp\" at datapoint %d from: \n%s\n", i, json_string);
            break;
        }

        t = json_doublify(timestamp);
		lt = *localtime(&t);
		strftime(time, sizeof(time), time_format, &lt);

        strcpy(c_type, json_stringify(type));
        if (strcmp(c_type, "temperature") == 0) {
            json_get_ex(datapoint, "value", &value);
            d_temp = json_doublify(value);
            if (!d_temp) {
                fprintf(stderr, "Error occured while trying to get \"temperature\" at datapoint %d from: \n%s\n", i, json_string);
                break;
            }
                    
			fprintf(stdout, "type: %s \t", c_type);
			fprintf(stdout, "time: %s \t", time);
			fprintf(stdout, "value: %.8f\n", d_temp);
        } else if (strcmp(c_type, "light") == 0) {
            json_get_ex(datapoint, "value", &value);
            i_light = json_intify(value);
            if (!i_light) {
                fprintf(stderr, "Error occured while trying to get \"light intensity\" at datapoint %d from: \n%s\n", i, json_string);
                break;
            }
            fprintf(stdout, "type: %s \t\t", c_type);
			fprintf(stdout, "time: %s\t", time);
			fprintf(stdout, "value: %d\n", i_light);
        } else if (strcmp(c_type, "gps") == 0) {
            json_get_ex(datapoint, "long", &gps_long);
            d_long = json_doublify(gps_long);
            if (!d_long) {
                fprintf(stderr, "Error occured while trying to get \"longitude\" at datapoint %d from: \n%s\n", i, json_string);
                break;
            }

            json_get_ex(datapoint, "lat", &gps_lat);
            d_lat = json_doublify(gps_lat);
            if (!d_lat) {
                fprintf(stderr, "Error occured while trying to get \"latitude\" at datapoint %d from: \n%s\n", i, json_string);
                break;
            }
            fprintf(stdout, "type: %s \t\t", c_type);
			fprintf(stdout, "time: %s \t", time);
			fprintf(stdout, "long: %.9f \t", d_long);
			fprintf(stdout, "lat: %.8f\n", d_lat);
        } else {
            fprintf(stderr, "Error occured while trying to find the \"type\" at datapoint %d from: \n%s\n", i, json_string);
            break;
        }
    }
    return 0;
}
