#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <json-c/json.h>
#include <mysql/mysql.h>

#define is_hex_char(char) ((char >= '0' && char <= '9') || (char >= 'a' && char <= 'f') || (char >= 'A' && char <= 'F'))
#define ensure_db_con() (con ? 0 : connect_to_database())
#define json_get_ex(datapoint, var, result) (json_object_object_get_ex(datapoint, var, result))
#define json_get_id(array, i) (json_object_array_get_idx(array, i))
#define json_length(array) (json_object_array_length(array))
#define json_stringify(var) (json_object_get_string(var))
#define json_object_stringify(obj) (json_object_to_json_string(obj))
#define json_intify(var) (json_object_get_int(var))
#define json_doublify(var) (json_object_get_double(var))

#define PORT 17021
#define BUFFERSIZE 16384

static int create_socket();
static int json_reader(char *json_string);
static int sql_err();
static int connect_to_database();
static int get_uuid_id(char uuid[20]);
static int insert_temp_data(int id, double temp, char time[32]);
static int insert_light_data(int id, int light_intensity, char time[32]);
static int insert_gps_data(int id, double gps_long, double gps_lat, char time[32]);
static int create_database();

static const char datetime_format[] = "%Y-%m-%d %X";
static const char time_format[] = "%X";
static const char *server = "localhost";
static const char *user = "groep1user";
static const char *passwd = "userKanto!1";
static const char *database = "mydb";

static char query[512];
static char *buffer;

static MYSQL *con;
static MYSQL_RES *res;
static MYSQL_ROW row;

static int server_fd, new_socket;
static struct sockaddr_in address;
static int addrlen = sizeof(address);

int main(int argc, char const *argv[]) {
    connect_to_database(); // Automatically exits if error occurs
    create_database();
    if (create_socket() != 0) return 1;
    
    buffer = malloc(BUFFERSIZE);
    
    // Listen for client connection
    if (listen(server_fd, 5) < 0) {
        fprintf(stderr, "[SOCK ERROR] Failure occured while listening for client connection");
        return 1;
    }
    
    while (1) {
        memset(buffer, 0, BUFFERSIZE);
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, 
                        (socklen_t*)&addrlen))<0) {
            fprintf(stderr, "[SOCK ERROR] Failure occured while accepting a clients connection");
            return 1;
        }
    
        read(new_socket, buffer, BUFFERSIZE);
        json_reader(buffer);
        memset(buffer, 0, BUFFERSIZE);
    }
    return 0;
}

static int create_socket() {
    int opt = 1;

    // Create socket, AF_INET = net, AF_LOCAL = local
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        fprintf(stderr, "[SOCK ERROR] Creation of socket failed\n");
        return 1;
    }
       
    // Set socket options (prevents "already in use" errors)
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR,
                                                  &opt, sizeof(opt))) {
        fprintf(stderr, "[SOCK ERROR] Setting socket options failed\n");
        return 1;
    }
    
    // Set values to address struct
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Binds socket to address and port   
    if (bind(server_fd, (struct sockaddr *)&address, 
                                 sizeof(address))<0) {
        fprintf(stderr, "[SOCK ERROR] Binding of socket failed\n");
        return 1;
    }
    return 0;
}

static int json_reader(char *json_string) {
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
		fprintf(stderr, "[JSON ERROR] Message contained invalid json: \n%s\n\n", json_string);
        return 1;
	}

    json_get_ex(parsed_json, "uuid", &uuid);
    if (!uuid) {
        fprintf(stderr, "[JSON ERROR] Couldn't acquire \"uuid\" from: \n%s\n\n", json_string);
        return 1;
    }

    strcpy(c_uuid, json_stringify(uuid));
    id = get_uuid_id(c_uuid);
    if (!id) {
        fprintf(stderr, "[SQL ERROR] Couldn't acquire id from uuid: %s\n", c_uuid);
        return 1;
    }

	json_get_ex(parsed_json, "data", &data);
    if (!data) {
        fprintf(stderr, "[JSON ERROR] Couldn't acquire \"data\": \n%s\n\n", json_string);
        return 1;
    }
	
    n_data = json_length(data);
    fprintf(stdout, "[%s] id: %d, uuid: %s send %d datapoints\n", curr_time , id, c_uuid, n_data);
    for (i = 0; i < n_data; i++) {
        datapoint = json_get_id(data, i);

        json_get_ex(datapoint, "type", &type);
        if (!type) {
            fprintf(stderr, "[JSON ERROR] Couldn't acquire \"type\" from: \n%s\n", json_object_stringify(datapoint));
            continue;
        }

        json_get_ex(datapoint, "timestamp", &timestamp);
        if (!timestamp) {
            fprintf(stderr, "[JSON ERROR] Couldn't acquire \"timestamp\" from: \n%s\n", json_object_stringify(datapoint));
            continue;
        }

        t = json_doublify(timestamp);
		lt = *localtime(&t);
		strftime(ESP_time, sizeof(ESP_time), datetime_format, &lt);

        strcpy(c_type, json_stringify(type));
        if (strcmp(c_type, "temperature") == 0) {
            json_get_ex(datapoint, "value", &value);
            d_temp = json_doublify(value);
            if (!d_temp || d_temp >= 100) {
                fprintf(stderr, "[JSON ERROR] Couldn't acquire \"temperature\" from: \n%s\n", json_object_stringify(datapoint));
                continue;
            }

            insert_temp_data(id, d_temp, ESP_time);

        } else if (strcmp(c_type, "light") == 0) {
            json_get_ex(datapoint, "value", &value);
            i_light = json_intify(value);
            if (!i_light) {
                fprintf(stderr, "[JSON ERROR] Couldn't acquire \"light intensity\" from: \n%s\n", json_object_stringify(datapoint));
                continue;
            }
            
            insert_light_data(id, i_light, ESP_time);

        } else if (strcmp(c_type, "gps") == 0) {
            json_get_ex(datapoint, "lng", &gps_long);
            d_long = json_doublify(gps_long);
            if (!d_long) {
                fprintf(stderr, "[JSON ERROR] Couldn't acquire \"longitude\" from: \n%s\n", json_object_stringify(datapoint));
                continue;
            }

            json_get_ex(datapoint, "lat", &gps_lat);
            d_lat = json_doublify(gps_lat);
            if (!d_lat) {
                fprintf(stderr, "[JSON ERROR] Couldn't acquire \"latitude\" from: \n%s\n", json_object_stringify(datapoint));
                continue;
            }

            insert_gps_data(id, d_long, d_lat, ESP_time);

        } else {
            fprintf(stderr, "[JSON ERROR] Couldn't acquire \"type\" from: \n%s\n\n", json_object_stringify(datapoint));
            continue;
        }
    }
    return 0;
}

static int sql_err() {
	fprintf(stderr, "[SQL ERROR] %s\n", mysql_error(con));
	if (con) mysql_close(con);
	exit(1);
}

static int connect_to_database() {
    con = mysql_init(NULL);
    if (!con) sql_err();

    if (!mysql_real_connect(con, server, user, passwd,
                NULL, 0, NULL, 0)) sql_err();

    sprintf(query, "USE %s", database);
    if (mysql_query(con, query)) {
        fprintf(stderr, "[SQL ERROR] %s\nCreating database '%s'\n", mysql_error(con), database);
        create_database(); // Automatically exits if error occurs
    }
	return 0;
}

static int get_uuid_id(char uuid[20]) {
	int id;

    for (int i = 0; i < 20; i++) {
        if (!is_hex_char(uuid[i])) return 0;
    }

    ensure_db_con(); // Automatically exits if error occurs

	sprintf(query, "SELECT(SELECT id FROM ESPs WHERE uuid='%s')", uuid);
	mysql_query(con, query);

	res = mysql_store_result(con); 
    while((row = mysql_fetch_row(res)) !=0) {
		id = row[0] ? atof(row[0]) : 0;
    }
    mysql_free_result(res);

	if (id != 0) return id;
    else {
        fprintf(stderr, "[SQL ERROR] uuid: %s not found, inserting it into ESPs table\n", uuid);

        sprintf(query, "INSERT INTO ESPs (uuid) VALUES ('%s')", uuid);
        mysql_query(con, query);

        mysql_query(con, "SELECT LAST_INSERT_ID()");
        res = mysql_store_result(con);

        while((row = mysql_fetch_row(res)) !=0) {
		    id = row[0] ? atof(row[0]) : 0;
        }

        if (id == 0) sql_err();

        mysql_free_result(res);
        return id;
    }
    return 0;
}

static int insert_temp_data(int id, double temp, char time[32]) {
    ensure_db_con(); // Automatically exits if error occurs

    sprintf(query, "INSERT INTO TempData (Temp_ESP_id, Temp_DateTimeFromESP, Temp_Temp) VALUES (%d, '%s', %.8f)", id, time, temp);
    if (mysql_query(con, query)) sql_err();
    return 0;
}

static int insert_light_data(int id, int light_intensity, char time[32]) {
    ensure_db_con(); // Automatically exits if error occurs

    sprintf(query, "INSERT INTO LightIntensityData (Light_ESP_id, Light_DateTimeFromESP, Light_Intensity) VALUES ('%d', '%s', '%d')", id, time, light_intensity);
    if (mysql_query(con, query)) sql_err();
    return 0;
}

static int insert_gps_data(int id, double gps_long, double gps_lat, char time[32]) {
    ensure_db_con(); // Automatically exits if error occurs

    sprintf(query, "INSERT INTO GPSData (GPS_ESP_id, GPS_DateTimeFromESP, GPS_long, GPS_lat) VALUES ('%d', '%s', '%.9f', '%.8f')", id, time, gps_long, gps_lat);
    if (mysql_query(con, query)) sql_err();
    return 0;
}

static int create_database() {
    sprintf(query, "CREATE DATABASE IF NOT EXISTS %s", database);
	if (mysql_query(con, query)) sql_err();

    sprintf(query, "USE %s", database);
    if (mysql_query(con, query)) sql_err();

    sprintf(query, "CREATE TABLE IF NOT EXISTS ESPs(\
			id INT NOT NULL AUTO_INCREMENT,\
			uuid CHAR(20) NOT NULL,\
			PRIMARY KEY (id),\
			UNIQUE INDEX id_UNIQUE (id ASC))\
			ENGINE = InnoDB;");
	if (mysql_query(con, query)) sql_err();		
	
    sprintf(query, "CREATE TABLE IF NOT EXISTS TempData(\
			Temp_ESP_id INT NOT NULL,\
			Temp_DateTimeFromESP DATETIME(2) NOT NULL,\
			Temp_TimestampAdded TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,\
			Temp_Temp DECIMAL(3,1) NOT NULL,\
			PRIMARY KEY (Temp_ESP_id, Temp_DateTimeFromESP),\
			CONSTRAINT fk_TempData_ESPs\
				FOREIGN KEY (Temp_ESP_id)\
				REFERENCES %s.ESPs (id)\
				ON DELETE NO ACTION\
				ON UPDATE NO ACTION)\
			ENGINE = InnoDB;", database);
    if (mysql_query(con, query)) sql_err();
	
    sprintf(query, "CREATE TABLE IF NOT EXISTS LightIntensityData(\
			Light_ESP_id INT NOT NULL,\
			Light_DateTimeFromESP DATETIME(2) NOT NULL,\
			Light_TimestampAdded TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,\
			Light_Intensity SMALLINT NOT NULL,\
			PRIMARY KEY (Light_ESP_id, Light_DateTimeFromESP),\
			CONSTRAINT fk_LightIntensityData_ESPs\
				FOREIGN KEY (Light_ESP_id)\
				REFERENCES %s.ESPs (id)\
				ON DELETE NO ACTION\
				ON UPDATE NO ACTION)\
			ENGINE = InnoDB;", database);
	if (mysql_query(con, query)) sql_err();

    sprintf(query, "CREATE TABLE IF NOT EXISTS GPSData(\
			GPS_ESP_id INT NOT NULL,\
			GPS_DateTimeFromESP DATETIME(2) NOT NULL,\
			GPS_TimestampAdded TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,\
			GPS_long DECIMAL(9,6) NOT NULL,\
			GPS_lat DECIMAL(8,6) NOT NULL,\
			PRIMARY KEY (GPS_ESP_id, GPS_DateTimeFromESP),\
			CONSTRAINT fk_GPSData_ESPs\
				FOREIGN KEY (GPS_ESP_id)\
				REFERENCES %s.ESPs (id)\
				ON DELETE NO ACTION\
				ON UPDATE NO ACTION)\
			ENGINE = InnoDB;", database);
	if (mysql_query(con,  query)) sql_err();

	return(0);
}
