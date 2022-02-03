#include <stdio.h>
#include <stdlib.h>
#include <mysql/mysql.h>

//gcc createDB.c -std=c99 -lmysqlclient

char *server = "localhost";
char *user = "main";
char *passwd = "";
char *database = "testdb";

int sql_err(MYSQL *con) {
	fprintf(stderr, "%s\n", mysql_error(con));
	if (con != NULL) {
		mysql_close(con);
	}
	return(1);
}

int main(int argc, char **argv) {
	MYSQL *con;
	con = mysql_init(NULL);
	
	// check if connection has been initialized
	if (!con) print_err(con);
	
	// check if connection can be made with database
	if (!mysql_real_connect(con, server, user, passwd,
			database, 0, NULL, 0)) {
		print_err(con);
	}
	
	// check if database exists, if not creates it,
	// returns 0 if query is completed succesfully
	if (mysql_query(con, "CREATE DATABASE IF NOT EXISTS testdb")) {
		print_err(con);
	}	
	// creating tables
	if (mysql_query(con, "CREATE TABLE IF NOT EXISTS ESPs(\
			id INT NOT NULL AUTO_INCREMENT,\
			uuid CHAR(20) NOT NULL,\
			PRIMARY KEY (id),\
			UNIQUE INDEX id_UNIQUE (id ASC))\
			ENGINE = InnoDB;")) {
		print_err(con);
	}		
	
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
			ENGINE = InnoDB;")) {
		print_err(con);
	}
	
	
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
			ENGINE = InnoDB;")) {
		print_err(con);
	}
	
	
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
			ENGINE = InnoDB;")) {
		print_err(con);
	}
	
	
	printf("completed\n");
	mysql_close(con);	
	exit(0);
}
