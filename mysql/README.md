# esp-idf-sql-client
You can access MySQL on the server over the network.   
Use [this](https://github.com/mevdschee/php-crud-api) as PHP script of WEB server.

![0001](https://user-images.githubusercontent.com/6020549/71758785-7694ae80-2ee7-11ea-99cc-cf65c44f48bc.jpg)

# Install MySQL Server
Many installation methods are open to the Internet.   
I referred [here](https://docs.rackspace.com/support/how-to/install-mysql-server-on-the-ubuntu-operating-system/).   
__No remote access permission is required.__   

# Install php-crud-api
```
$ cd $HOME

$ git clone https://github.com/mevdschee/php-crud-api
```

# Create DB and User
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


# Create Tables
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

# Install PHP & PDO driver for MySQL
```

$ sudo apt install php

$ sudo apt install php-mysql

$ php --version
PHP 7.2.24-0ubuntu0.18.04.1 (cli) (built: Oct 28 2019 12:07:07) ( NTS )
Copyright (c) 1997-2018 The PHP Group
Zend Engine v3.2.0, Copyright (c) 1998-2018 Zend Technologies
    with Zend OPcache v7.2.24-0ubuntu0.18.04.1, Copyright (c) 1999-2018, by Zend Technologies
```


# Start Built-in WEB Server
```
$ php -S 0.0.0.0:8080 -t $HOME/php-crud-api
PHP 7.2.24-0ubuntu0.18.04.1 Development Server started at Mon Dec 30 09:21:32 2019
Listening on http://0.0.0.0:8080
Document root is /home/nop/php-crud-api
Press Ctrl-C to quit.
```


# Test php-crud-api
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
