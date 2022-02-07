# PI-Socketserver

Required libraries:

MariaDB, json-c


MariaDB install:
```
$ sudo apt install mariadb-server
$ sudo mysql_secure_installation
$ sudo mariadb
MariaDB [(none)]> GRANT ALL ON *.* TO 'admin'@'localhost' IDENTIFIED BY 'password' WITH GRANT OPTION;
MariaDB [(none)]> FLUSH PRIVILEGES;
MariaDB [(none)]> exit
```
Fill 'admin', 'localhost' and 'password' in the following variables: ```char *server, char *user, char *passwd```

Set ```char *database``` to prefered database name ex: "mydb"


json-c install:
```
$ sudo apt install libjson-c-dev
```

Compile using ```gcc pi_socketserver.c -std=c99 -lmysqlclient -ljson-c -o <desired output file>```
