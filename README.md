# PI-Socketserver

This program is meant for linux based platforms as some libraries/header dont exist on other platforms

Required libraries:

MariaDB, json-c, libmysqlclient-dev


MariaDB install:
```
$ sudo apt install mariadb-server
$ sudo mysql_secure_installation
$ sudo mariadb
MariaDB [(none)]> GRANT ALL ON *.* TO 'admin'@'localhost' IDENTIFIED BY 'password' WITH GRANT OPTION;
MariaDB [(none)]> FLUSH PRIVILEGES;
MariaDB [(none)]> exit
```

In secret.h fill in 'admin', 'localhost', 'password' of the previous step to ensure a authorised connection to the database
```char *server "<localhost>", char *user "<admin>", char *passwd "<password>"```

Further change ```char *database "<mydb>"``` to the preferred database name, the program is able to create it itself


json-c install:
```
$ sudo apt install libjson-c-dev
```

libmysqlclient-dev install:
```
$ sudo apt install libmysqlclient-dev
```

Compile using ```gcc pi_socketserver.c -lmysqlclient -ljson-c -o <desired output file>```
