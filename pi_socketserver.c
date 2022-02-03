#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <json-c/json.h>
#include <mysql/mysql.h>

#define PORT 17021

#define json_get_ex(datapoint, var, result) (json_object_object_get_ex(datapoint, var, result))
#define json_get_id(array, i) (json_object_array_get_idx(array, i))
#define json_length(array) (json_object_array_length(array))
#define json_stringify(var) (json_object_get_string(var))
#define json_intify(var) (json_object_get_int(var))
#define json_doublify(var) (json_object_get_double(var))

int create_socket();
int json_reader(char *json_string);
int sql_err();
int connect_to_database();
int get_uuid_id(char uuid[20]);
int insert_temp_data(char uuid[20], double *temp, char time[32]);
int insert_light_data(char uuid[20], double *light_intensity, char time[32]);
int insert_gps_data(char uuid[20], double *gps_long, double *gps_lat, char time[32]);
int create_database();

static const char time_format[] = "%Y-%m-%d %X";
static const char *server = "localhost";
static const char *user = "main";
static const char *passwd = "";
static const char *database = "testdb";

static char query[100];

static MYSQL *con;
static MYSQL_RES *res;
static MYSQL_ROW row;

// {"uuid":"489377d75d1d15247320","data":[{"type":"temperature","value":22.61000061,"timestamp":1643818772},{"type":"light","value":15,"timestamp":1643818773},{"type":"temperature","value":22.61000061,"timestamp":1643818775},{"type":"light","value":15,"timestamp":1643818776},{"type":"temperature","value":22.60000038,"timestamp":1643818778},{"type":"light","value":15,"timestamp":1643818779},{"type":"temperature","value":22.59000015,"timestamp":1643818781}]}}]}

char *json_string = "{\"uuid\":\"489377d75d1d15247320\",\"data\":[{\"type\":\"temperature\",\"value\":22.61000061,\"timestamp\":1643818772},{\"type\":\"light\",\"value\":15,\"timestamp\":1643818773},{\"type\":\"temperature\",\"value\":22.61000061,\"timestamp\":1643818775},{\"type\":\"light\",\"value\":15,\"timestamp\":1643818776},{\"type\":\"temperature\",\"value\":22.60000038,\"timestamp\":1643818778},{\"type\":\"light\",\"value\":15,\"timestamp\":1643818779},{\"type\":\"temperature\",\"value\":22.59000015,\"timestamp\":1643818781}]}";

int main(int argc, char const *argv[]) {
    connect_to_database();
    json_reader(json_string);
    return 0;
}

int create_socket() {
	return 0;
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

    char c_uuid[20];
    char c_type[12];
    int id;
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
    strcpy(c_uuid, json_stringify(uuid));
    fprintf(stdout, "uuid: %s\n", c_uuid);

    id = get_uuid_id(c_uuid);
    fprintf(stdout, "id: %d\n", id); 

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

            //insert_temp_data(uuid, d_temp, time);
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
            
            //insert_light_data(uuid, i_light, time);
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

            //insert_gps_data(uuid, gps_long, gps_lat, time);
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

int sql_err() {
	fprintf(stderr, "%s\n", mysql_error(con));
	if (con) mysql_close(con);
	return(1);
}

int connect_to_database() {
    con = mysql_init(NULL);
    if (!con) sql_err();

    if (!mysql_real_connect(con, server, user, passwd,
                NULL, 0, NULL, 0)) sql_err();

    sprintf(query, "USE %s", database);
    if (mysql_query(con, query)) {
        fprintf(stderr, "%s\ncreating database '%s'\n", mysql_error(con), database);
        if (!create_database() == 0) return 1;
    }
	return 0;
}

int get_uuid_id(char uuid[20]) {
	int id;

	sprintf(query, "SELECT id FROM ESPs WHERE uuid='%s'", uuid);
	mysql_query(con, query);

	res = mysql_store_result(con);
	
	if (!res) sql_err();
    
    while((row = mysql_fetch_row(res)) !=0) {
		id = row[0] ? atof(row[0]) : 0.0f;

		mysql_free_result(res);
		return id;
    }
    return 0;
}

int insert_temp_data(char uuid[20], double *temp, char time[32]) {
    if (mysql_query(con, "")) return sql_err();
    return 0;
}

int insert_light_data(char uuid[20], double *light_intensity, char time[32]) {
    if (mysql_query(con, "")) return sql_err();
    return 0;
}

int insert_gps_data(char uuid[20], double *gps_long, double *gps_lat, char time[32]) {
    if (mysql_query(con, "")) return sql_err();
    return 0;
}

int create_database() {
    if (!con) return sql_err();

    // check if database exists, if not creates it,
	if (mysql_query(con, "CREATE DATABASE IF NOT EXISTS testdb")) {
		return sql_err();
	}
	// creating tables (if they dont exist)
	if (mysql_query(con, "CREATE TABLE IF NOT EXISTS ESPs(\
			id INT NOT NULL AUTO_INCREMENT,\
			uuid CHAR(20) NOT NULL,\
			PRIMARY KEY (id),\
			UNIQUE INDEX id_UNIQUE (id ASC))\
			ENGINE = InnoDB;")) return sql_err();		
	
	if (mysql_query(con, "CREATE TABLE IF NOT EXISTS GPSData(\
			GPS_ESP_id INT NOT NULL,\
			GPS_DateTimeFromESP DATETIME(2) NOT NULL,\
			GPS_TimestampAdded TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,\
			GPS_long DECIMAL(9,6) NOT NULL,\
			GPS_lat DECIMAL(8,6) NOT NULL,\
			PRIMARY KEY (ESP_id, DateTimeFromESP),\
			CONSTRAINT fk_GPSData_ESPs\
				FOREIGN KEY (ESP_id)\
				REFERENCES testdb.ESPs (id)\
				ON DELETE NO ACTION\
				ON UPDATE NO ACTION)\
			ENGINE = InnoDB;")) return sql_err();
	
	if (mysql_query(con, "CREATE TABLE IF NOT EXISTS TempData(\
			Temp_ESP_id INT NOT NULL,\
			Temp_DateTimeFromESP DATETIME(2) NOT NULL,\
			Temp_TimestampAdded TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,\
			Temp_Temp DECIMAL(3,1) NOT NULL,\
			PRIMARY KEY (ESP_id, DateTimeFromESP),\
			CONSTRAINT fk_TempData_ESPs\
				FOREIGN KEY (ESP_id)\
				REFERENCES testdb.ESPs (id)\
				ON DELETE NO ACTION\
				ON UPDATE NO ACTION)\
			ENGINE = InnoDB;")) return sql_err();
	
	if (mysql_query(con, "CREATE TABLE IF NOT EXISTS LightIntensityData(\
			Light_ESP_id INT NOT NULL,\
			Light_DateTimeFromESP DATETIME(2) NOT NULL,\
			Light_TimestampAdded TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,\
			Light_Intensity DECIMAL(11,4) NOT NULL,\
			PRIMARY KEY (ESP_id, DateTimeFromESP),\
			CONSTRAINT fk_LightIntensityData_ESPs\
				FOREIGN KEY (ESP_id)\
				REFERENCES testdb.ESPs (id)\
				ON DELETE NO ACTION\
				ON UPDATE NO ACTION)\
			ENGINE = InnoDB;")) return sql_err();

	return(0);
}
