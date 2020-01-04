# esp-idf-mysql
Operate MySQL from ESP32 via PHP.   
Use [this](https://github.com/mevdschee/php-crud-api) as middleware.

![0001](https://user-images.githubusercontent.com/6020549/71568475-e7fce780-2b0a-11ea-8e91-52a6d268d0cd.jpg)

# Host Side

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

## Install PHP
```

$ sudo apt install php

$ sudo apt-get install php-mysql

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

---

# ESP32 Side

## Build Firmware
You have to set this config value with menuconfig.   
CONFIG_ESP_WIFI_SSID   
CONFIG_ESP_WIFI_PASSWORD   
CONFIG_ESP_MAXIMUM_RETRY   
CONFIG_ESP_WEB_SERVER_IP   
CONFIG_ESP_WEB_SERVER_PORT   
CONFIG_ESP_PHP_PATH   

```
git clone https://github.com/nopnop2002/esp-idf-mysql
cd esp-idf-mysql/
make menuconfig
make flash
```

![menuconfig-11](https://user-images.githubusercontent.com/6020549/71758309-4f3ae300-2ee1-11ea-86f6-89869ae35f63.jpg)

![menuconfig-12](https://user-images.githubusercontent.com/6020549/71758645-033e6d00-2ee6-11ea-81b5-a3b9d954e9fe.jpg)

## List
```
I (35386) JSON: path=
I (35386) JSON: url=http://192.168.10.43:8080/api.php/records/posts
I (36666) JSON: HTTP GET Status = 200, content_length = 134
I (37666) JSON: buffer=
{"records":[{"id":1,"user_id":1,"category_id":1,"content":"blog started"},{"id":2,"user_id":1,"category_id":2,"content":"It works!"}]}
I (37666) JSON: Deserialize.....
I (37666) JSON: [records]
I (37676) JSON: [id]
I (37676) JSON: int=1 double=1.000000
I (37676) JSON: [user_id]
I (37686) JSON: int=1 double=1.000000
I (37686) JSON: [category_id]
I (37686) JSON: int=1 double=1.000000
I (37696) JSON: [content]
I (37696) JSON: blog started
I (37706) JSON: [id]
I (37706) JSON: int=2 double=2.000000
I (37706) JSON: [user_id]
I (37716) JSON: int=1 double=1.000000
I (37716) JSON: [category_id]
I (37716) JSON: int=2 double=2.000000
I (37726) JSON: [content]
I (37726) JSON: It works!
```

## Read
```
I (70386) JSON: path=1
I (70386) JSON: url=http://192.168.10.43:8080/api.php/records/posts/1
I (70416) JSON: HTTP GET Status = 200, content_length = 61
I (71416) JSON: buffer=
{"id":1,"user_id":1,"category_id":1,"content":"blog started"}
I (71416) JSON: Deserialize.....
I (71416) JSON: [id]
I (71416) JSON: int=1 double=1.000000
I (71416) JSON: [user_id]
I (71426) JSON: int=1 double=1.000000
I (71426) JSON: [category_id]
I (71436) JSON: int=1 double=1.000000
I (71436) JSON: [content]
I (71436) JSON: blog started
```

## Create
```
I (95486) JSON: url=http://192.168.10.43:8080/api.php/records/posts
I (95656) JSON: HTTP GET Status = 200, content_length = 1
I (96656) JSON: buffer=
3
I (96656) JSON: new_id=3
```

## Update
```
I (115586) JSON: path=3
I (115586) JSON: url=http://192.168.10.43:8080/api.php/records/posts/3
I (115616) JSON: HTTP GET Status = 200, content_length = 60
I (116616) JSON: buffer=
{"id":3,"user_id":1,"category_id":3,"content":"Hello World"}
I (116616) JSON: Deserialize.....
I (116616) JSON: [id]
I (116616) JSON: int=3 double=3.000000
I (116616) JSON: [user_id]
I (116626) JSON: int=1 double=1.000000
I (116626) JSON: [category_id]
I (116636) JSON: int=3 double=3.000000
I (116636) JSON: [content]
I (116636) JSON: Hello World
I (116646) JSON: path=3
I (116656) JSON: url=http://192.168.10.43:8080/api.php/records/posts/3
I (116806) JSON: HTTP GET Status = 200, content_length = 1
I (117806) JSON: buffer=
1
I (117806) JSON: path=3
I (117806) JSON: url=http://192.168.10.43:8080/api.php/records/posts/3
I (117836) JSON: HTTP GET Status = 200, content_length = 60
I (118836) JSON: buffer=
{"id":3,"user_id":1,"category_id":3,"content":"Hello Japan"}
I (118836) JSON: Deserialize.....
I (118836) JSON: [id]
I (118836) JSON: int=3 double=3.000000
I (118836) JSON: [user_id]
I (118846) JSON: int=1 double=1.000000
I (118846) JSON: [category_id]
I (118856) JSON: int=3 double=3.000000
I (118856) JSON: [content]
I (118856) JSON: Hello Japan
```

## Delete
```
I (149386) JSON: path=3
I (149386) JSON: url=http://192.168.10.43:8080/api.php/records/posts/3
I (149436) JSON: HTTP GET Status = 200, content_length = 60
I (150436) JSON: buffer=
{"id":3,"user_id":1,"category_id":3,"content":"Hello Japan"}
I (150436) JSON: Deserialize.....
I (150436) JSON: [id]
I (150436) JSON: int=3 double=3.000000
I (150436) JSON: [user_id]
I (150446) JSON: int=1 double=1.000000
I (150446) JSON: [category_id]
I (150456) JSON: int=3 double=3.000000
I (150456) JSON: [content]
I (150456) JSON: Hello Japan
I (150466) JSON: path=3
I (150476) JSON: url=http://192.168.10.43:8080/api.php/records/posts/3
I (150646) JSON: HTTP GET Status = 200, content_length = 1
I (151646) JSON: buffer=
1
I (151646) JSON: path=3
I (151646) JSON: url=http://192.168.10.43:8080/api.php/records/posts/3
I (151686) JSON: HTTP GET Status = 404, content_length = 46
I (152686) JSON: buffer=
{"code":1003,"message":"Record '3' not found"}
I (152686) JSON: Deserialize.....
I (152686) JSON: [code]
I (152686) JSON: int=1003 double=1003.000000
I (152686) JSON: [message]
I (152696) JSON: Record '3' not found
```
