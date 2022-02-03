/*
TO ADD
socket rewrite
uuid exists check
comments
*/

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
#define json_object_stringify(obj) (json_object_to_json_string(obj))
#define json_intify(var) (json_object_get_int(var))
#define json_doublify(var) (json_object_get_double(var))

int create_socket();
int json_reader(char *json_string);
int sql_err();
int connect_to_database();
int get_uuid_id(char uuid[20]);
int insert_temp_data(int id, double temp, char time[32]);
int insert_light_data(int id, int light_intensity, char time[32]);
int insert_gps_data(int id, double gps_long, double gps_lat, char time[32]);
int create_database();

static const char datetime_format[] = "%Y-%m-%d %X";
static const char time_format[] = "%X";
static const char *server = "localhost";
static const char *user = "main";
static const char *passwd = "";
static const char *database = "testdb";

static char query[256];

static MYSQL *con;
static MYSQL_RES *res;
static MYSQL_ROW row;

int server_fd, new_socket;
struct sockaddr_in address;
int opt = 1;
int addrlen = sizeof(address);

int main(int argc, char const *argv[]) {
    connect_to_database();
    create_socket();
       
    if (bind(server_fd, (struct sockaddr *)&address, 
                                 sizeof(address))<0) {
        fprintf(stderr, "bind failed");
        return 1;
    }

    if (listen(server_fd, 3) < 0) {
        fprintf(stderr, "listen");
        return 1;
    }
    
    while (1) {
        char buffer[1024] = {0};
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, 
                        (socklen_t*)&addrlen))<0) {
            fprintf(stderr, "accept");
            return 1;
        }
    
        read(new_socket, buffer, 1024);
        json_reader(buffer);
    }
    return 0;
}

int create_socket() {
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        fprintf(stderr, "socket failed");
        return 1;
    }
       
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
                                                  &opt, sizeof(opt))) {
        fprintf(stderr, "setsockopt");
        return 1;
    }
    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons( PORT );
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
	char ESP_time[32];
    char curr_time[32];
	
    t = time(NULL);
	lt = *localtime(&t);
	strftime(curr_time, sizeof(curr_time), time_format, &lt);

    parsed_json = json_tokener_parse(json_string);
    if (!parsed_json) {
		fprintf(stderr, "Message contain invalid json: \n%s\n\n", json_string);
	}

    json_get_ex(parsed_json, "uuid", &uuid);
    if (!uuid) {
        fprintf(stderr, "Json contains no or an invalid \"uuid\": \n%s\n\n", json_string);
        return 1;
    }
    
    strcpy(c_uuid, json_stringify(uuid));
    id = get_uuid_id(c_uuid);

	json_get_ex(parsed_json, "data", &data);
    if (!data) {
        fprintf(stderr, "Json contains no or invalid \"data\": \n%s\n\n", json_string);
        return 1;
    }
	
    n_data = json_length(data);
    fprintf(stdout, "[%s] uuid: %s send %d datapoints\n", curr_time ,c_uuid, n_data);
    for (i = 0; i < n_data; i++) {
        datapoint = json_get_id(data, i);

        json_get_ex(datapoint, "type", &type);
        if (!type) {
            fprintf(stderr, "Error occured while trying to get \"type\": \n%s\n", json_object_stringify(datapoint));
            break;
        }

        json_get_ex(datapoint, "timestamp", &timestamp);
        if (!timestamp) {
            fprintf(stderr, "Error occured while trying to get \"timestamp\": \n%s\n", json_object_stringify(datapoint));
            break;
        }

        t = json_doublify(timestamp);
		lt = *localtime(&t);
		strftime(ESP_time, sizeof(ESP_time), datetime_format, &lt);

        strcpy(c_type, json_stringify(type));
        if (strcmp(c_type, "temperature") == 0) {
            json_get_ex(datapoint, "value", &value);
            d_temp = json_doublify(value);
            if (!d_temp) {
                fprintf(stderr, "Error occured while trying to get \"temperature\": \n%s\n", json_object_stringify(datapoint));
                continue;
            }

            insert_temp_data(id, d_temp, ESP_time);

        } else if (strcmp(c_type, "light") == 0) {
            json_get_ex(datapoint, "value", &value);
            i_light = json_intify(value);
            if (!i_light) {
                fprintf(stderr, "Error occured while trying to get \"light intensity\": \n%s\n", json_object_stringify(datapoint));
                continue;
            }
            
            insert_light_data(id, i_light, ESP_time);

        } else if (strcmp(c_type, "gps") == 0) {
            json_get_ex(datapoint, "long", &gps_long);
            d_long = json_doublify(gps_long);
            if (!d_long) {
                fprintf(stderr, "Error occured while trying to get \"longitude\" at datapoint %d from: \n%s\n\n", i, json_object_stringify(datapoint));
                continue;
            }

            json_get_ex(datapoint, "lat", &gps_lat);
            d_lat = json_doublify(gps_lat);
            if (!d_lat) {
                fprintf(stderr, "Error occured while trying to get \"latitude\" at datapoint %d from: \n%s\n\n", i, json_object_stringify(datapoint));
                continue;
            }

            insert_gps_data(id, d_long, d_lat, ESP_time);

        } else {
            fprintf(stderr, "Error occured while trying to find the \"type\" at datapoint %d from: \n%s\n\n", i, json_object_stringify(datapoint));
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

int insert_temp_data(int id, double temp, char time[32]) {
    sprintf(query, "INSERT INTO TempData (Temp_ESP_id, Temp_DateTimeFromESP, Temp_Temp) VALUES (%d, '%s', %.8f)", id, time, temp);
    if (mysql_query(con, query)) return sql_err();
    return 0;
}

int insert_light_data(int id, int light_intensity, char time[32]) {
    sprintf(query, "INSERT INTO LightIntensityData (Light_ESP_id, Light_DateTimeFromESP, Light_Intensity) VALUES ('%d', '%s', '%d')", id, time, light_intensity);
    if (mysql_query(con, query)) return sql_err();
    return 0;
}

int insert_gps_data(int id, double gps_long, double gps_lat, char time[32]) {
    sprintf(query, "INSERT INTO GPSData (GPS_ESP_id, GPS_DateTimeFromESP, GPS_long, GPS_lat) VALUES ('%d', '%s', '%.9f', '%.8f')", id, time, gps_long, gps_lat);
    if (mysql_query(con, query)) return sql_err();
    return 0;
}

int create_database() {
    if (!con) return sql_err();

    sprintf(query, "CREATE DATABASE IF NOT EXISTS %s", database);
	if (mysql_query(con, query)) return sql_err();

	if (mysql_query(con, "CREATE TABLE IF NOT EXISTS ESPs(\
			id INT NOT NULL AUTO_INCREMENT,\
			uuid CHAR(20) NOT NULL,\
			PRIMARY KEY (id),\
			UNIQUE INDEX id_UNIQUE (id ASC))\
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

	return(0);
}
