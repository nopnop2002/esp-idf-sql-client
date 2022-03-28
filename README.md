# esp-idf-sql-client
SQL database access example for esp-idf.   
You can access MySQL/PostgreSQL on the server over the network.   
Use [this](https://github.com/mevdschee/php-crud-api) as PHP script of WEB server.

![0001](https://user-images.githubusercontent.com/6020549/71758785-7694ae80-2ee7-11ea-99cc-cf65c44f48bc.jpg)

# Using MySQL
https://github.com/nopnop2002/esp-idf-sql-client/mysql/

# Using PostgreSQL
https://github.com/nopnop2002/esp-idf-sql-client/pgsql/

# Software requirements
esp-idf ver4.1 or later.   

# Installation   
```
git clone https://github.com/nopnop2002/esp-idf-sql-client
cd esp-idf-sql-client/
idf.py menuconfig
idf.py flash monitor
```


# Configuration   
You have to set this config value with menuconfig.   
- CONFIG_ESP_WIFI_SSID   
SSID of your wifi.
- CONFIG_ESP_WIFI_PASSWORD   
PASSWORD of your wifi.
- CONFIG_ESP_MAXIMUM_RETRY   
Maximum number of retries when connecting to wifi.
- CONFIG_ESP_WEB_SERVER_IP   
IP or mDNS of your WEB Server.
- CONFIG_ESP_WEB_SERVER_PORT   
Port number of your WEB Server.
- CONFIG_ESP_PHP_PATH   
Path of PHP Script,


![menuconfig-1](https://user-images.githubusercontent.com/6020549/97793281-68114380-1c2d-11eb-9787-c8df218693ed.jpg)
![menuconfig-2](https://user-images.githubusercontent.com/6020549/97793282-69427080-1c2d-11eb-929f-2dce855073c3.jpg)

# Read all data
```
I (7369) HTTP: -----------------------------------------
I (7379) HTTP: 1        1       1       blog started
I (7379) HTTP: 2        1       2       It works!
I (7379) HTTP: -----------------------------------------
```

# Read by ID
```
I (18249) HTTP: -----------------------------------------
I (18249) HTTP: 2       1       2       It works!
I (18249) HTTP: -----------------------------------------
```

# Create new record
```
I (26519) HTTP: -----------------------------------------
I (26519) HTTP: 3       1       3       Hello World
I (26529) HTTP: -----------------------------------------
```

# Update a new record
```
I (37739) HTTP: -----------------------------------------
I (37749) HTTP: 3       1       3       Hello Japan
I (37749) HTTP: -----------------------------------------
```

# Delete a new record
```
I (47959) HTTP: -----------------------------------------
I (47959) HTTP: 1       1       1       blog started
I (47969) HTTP: 2       1       2       It works!
I (47969) HTTP: -----------------------------------------
```
