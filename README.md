# esp-idf-mysql
MySQL access example for esp-idf.   
You can access MySQL on the server over the network.   
Use [this](https://github.com/mevdschee/php-crud-api) as PHP script of WEB server.

![0001](https://user-images.githubusercontent.com/6020549/71758785-7694ae80-2ee7-11ea-99cc-cf65c44f48bc.jpg)

# Server Side

## Install MySQL Server
Many installation methods are open to the Internet.   
I referred [here](https://docs.rackspace.com/support/how-to/install-mysql-server-on-the-ubuntu-operating-system/).   

## Install php-crud-api
```
$ cd $HOME

$ git clone https://github.com/mevdschee/php-crud-api
```

## Create DB and User
```
$ cd php-crud-api/

$ sudo mysql -u root -p
mysql> source tests/fixtures/create_mysql.sql;

mysql> SHOW DATABASES;
+--------------------+
| Database           |
+--------------------+
| information_schema |
| mysql              |
| performance_schema |
| php-crud-api       |
| sys                |
+--------------------+
6 rows in set (0.00 sec)


mysql> select Host, User from mysql.user;
+-----------+------------------+
| Host      | User             |
+-----------+------------------+
| localhost | debian-sys-maint |
| localhost | mysql.session    |
| localhost | mysql.sys        |
| localhost | php-crud-api     |
| localhost | root             |
+-----------+------------------+
7 rows in set (0.00 sec)

mysql> exit
```


## Create Tables
```
$ mysql -u php-crud-api -h localhost --password=php-crud-api

mysql> use php-crud-api;
Database changed

mysql> select database();
+--------------+
| database()   |
+--------------+
| php-crud-api |
+--------------+
1 row in set (0.00 sec)


mysql> source tests/fixtures/blog_mysql.sql;

mysql> SHOW TABLES;
+------------------------+
| Tables_in_php-crud-api |
+------------------------+
| barcodes               |
| categories             |
| comments               |
| countries              |
| events                 |
| invisibles             |
| kunsthandvark          |
| nopk                   |
| post_tags              |
| posts                  |
| products               |
| tag_usage              |
| tags                   |
| users                  |
+------------------------+
14 rows in set (0.00 sec)

mysql> select * from posts;
+----+---------+-------------+--------------+
| id | user_id | category_id | content      |
+----+---------+-------------+--------------+
|  1 |       1 |           1 | blog started |
|  2 |       1 |           2 | It works!    |
+----+---------+-------------+--------------+
2 rows in set (0.00 sec)

mysql> exit
```

## Install PHP & PDO driver for MySQL
```

$ sudo apt install php

$ sudo apt install php-mysql

$ php --version
PHP 7.2.24-0ubuntu0.18.04.1 (cli) (built: Oct 28 2019 12:07:07) ( NTS )
Copyright (c) 1997-2018 The PHP Group
Zend Engine v3.2.0, Copyright (c) 1998-2018 Zend Technologies
    with Zend OPcache v7.2.24-0ubuntu0.18.04.1, Copyright (c) 1999-2018, by Zend Technologies
```


## Start Built-in WEB Server
```
$ php -S 0.0.0.0:8080 -t $HOME/php-crud-api
PHP 7.2.24-0ubuntu0.18.04.1 Development Server started at Mon Dec 30 09:21:32 2019
Listening on http://0.0.0.0:8080
Document root is /home/nop/php-crud-api
Press Ctrl-C to quit.
```


## Test php-crud-api
```
$ curl http://localhost:8080/api.php/records/posts/ | python -mjson.tool
  % Total    % Received % Xferd  Average Speed   Time    Time     Time  Current
                                 Dload  Upload   Total   Spent    Left  Speed
100   134  100   134    0     0   8933      0 --:--:-- --:--:-- --:--:--  8933
{
    "records": [
        {
            "category_id": 1,
            "content": "blog started",
            "id": 1,
            "user_id": 1
        },
        {
            "category_id": 2,
            "content": "It works!",
            "id": 2,
            "user_id": 1
        }
    ]
}
```

## Test php-crud-api from remote
```
$ curl http://ServerIP:8080/api.php/records/posts/ | python -mjson.tool
  % Total    % Received % Xferd  Average Speed   Time    Time     Time  Current
                                 Dload  Upload   Total   Spent    Left  Speed
100   134  100   134    0     0   8933      0 --:--:-- --:--:-- --:--:--  8933
{
    "records": [
        {
            "category_id": 1,
            "content": "blog started",
            "id": 1,
            "user_id": 1
        },
        {
            "category_id": 2,
            "content": "It works!",
            "id": 2,
            "user_id": 1
        }
    ]
}
```


---

# ESP32 Side


## Software requirements
esp-idf ver4.1 or later.   

## Install
You have to set this config value with menuconfig.   
- CONFIG_ESP_WIFI_SSID   
SSID of your wifi.
- CONFIG_ESP_WIFI_PASSWORD   
PASSWORD of your wifi.
- CONFIG_ESP_MAXIMUM_RETRY   
Maximum number of retries when connecting to wifi.
- CONFIG_ESP_WEB_SERVER_IP   
IP or DNS of your WEB Server.
- CONFIG_ESP_WEB_SERVER_PORT   
Port number of your WEB Server.
- CONFIG_ESP_PHP_PATH   
Path of PHP Script,
- CONFIG_JSON_PARSE   
Enable JSON parse.

```
git clone https://github.com/nopnop2002/esp-idf-mysql
cd esp-idf-mysql/
idf.py menuconfig
idf.py flash monitor
```

![menuconfig-1](https://user-images.githubusercontent.com/6020549/97793281-68114380-1c2d-11eb-9787-c8df218693ed.jpg)

![menuconfig-2](https://user-images.githubusercontent.com/6020549/97793282-69427080-1c2d-11eb-929f-2dce855073c3.jpg)

## Read all data
```
I (3592) MYSQL:
{"records":[{"id":1,"user_id":1,"category_id":1,"content":"blog started"},{"id":2,"user_id":1,"category_id":2,"content":"It works!"}]}
I (3592) MYSQL: Deserialize.....
I (3602) MYSQL: [id] int=1 double=1.000000
I (3602) MYSQL: [user_id] int=1 double=1.000000
I (3612) MYSQL: [category_id] int=1 double=1.000000
I (3612) MYSQL: [content] blog started
I (3622) MYSQL: [id] int=2 double=2.000000
I (3622) MYSQL: [user_id] int=1 double=1.000000
I (3632) MYSQL: [category_id] int=2 double=2.000000
I (3632) MYSQL: [content] It works
```

## Read by ID
```
I (20602) MYSQL:
{"id":2,"user_id":1,"category_id":2,"content":"It works!"}
I (20602) MYSQL: Deserialize.....
I (20602) MYSQL: [id] int=2 double=2.000000
I (20602) MYSQL: [user_id] int=1 double=1.000000
I (20612) MYSQL: [category_id] int=2 double=2.000000
I (20612) MYSQL: [content] It works!
```

## Create new record
```
I (32932) MYSQL:
{"id":16,"user_id":1,"category_id":3,"content":"Hello World"}
I (32932) MYSQL: Deserialize.....
I (32932) MYSQL: [id] int=16 double=16.000000
I (32942) MYSQL: [user_id] int=1 double=1.000000
I (32942) MYSQL: [category_id] int=3 double=3.000000
I (32952) MYSQL: [content] Hello World
```

## Update record
```
I (42872) MYSQL:
{"id":16,"user_id":1,"category_id":3,"content":"Hello Japan"}
I (42872) MYSQL: Deserialize.....
I (42872) MYSQL: [id] int=16 double=16.000000
I (42872) MYSQL: [user_id] int=1 double=1.000000
I (42882) MYSQL: [category_id] int=3 double=3.000000
I (42882) MYSQL: [content] Hello Japan
```

## Delete record
```
I (54642) MYSQL:
{"code":1003,"message":"Record '16' not found"}
I (54642) MYSQL: Deserialize.....
I (54642) MYSQL: [code] int=1003 double=1003.000000
I (54652) MYSQL: [message] Record '16' not found
```
