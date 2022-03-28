# esp-idf-sql-client
PostgreSQL access example for esp-idf.   
You can access PostgreSQL on the server over the network.   
Use [this](https://github.com/mevdschee/php-crud-api) as PHP script of WEB server.

![0001](https://user-images.githubusercontent.com/6020549/71758785-7694ae80-2ee7-11ea-99cc-cf65c44f48bc.jpg)

# Install PostgreSQL Server
Many installation methods are open to the Internet.   
__No remote access permission is required.__   

# Install php-crud-api
```
$ cd $HOME

$ git clone https://github.com/mevdschee/php-crud-api
```

# Create DB and User
```
$ sudo -u postgres psql
psql (12.9 (Ubuntu 12.9-0ubuntu0.20.04.1))
Type "help" for help.

postgres=# CREATE USER "php-crud-api" WITH ENCRYPTED PASSWORD 'php-crud-api';
CREATE ROLE
php-crud-api=# select usename from pg_user;
   usename
--------------
 postgres
 php-crud-api
(2 rows)



postgres=# CREATE DATABASE "php-crud-api" OWNER "php-crud-api";
CREATE DATABASE
postgres=# GRANT ALL PRIVILEGES ON DATABASE "php-crud-api" to "php-crud-api";
GRANT
postgres=# \l
                                           List of databases
     Name     |    Owner     | Encoding |   Collate   |    Ctype    |         Access privileges
--------------+--------------+----------+-------------+-------------+-----------------------------------
 php-crud-api | php-crud-api | UTF8     | ja_JP.UTF-8 | ja_JP.UTF-8 | =Tc/"php-crud-api"               +
              |              |          |             |             | "php-crud-api"=CTc/"php-crud-api"
 postgres     | postgres     | UTF8     | ja_JP.UTF-8 | ja_JP.UTF-8 |
 template0    | postgres     | UTF8     | ja_JP.UTF-8 | ja_JP.UTF-8 | =c/postgres                      +
              |              |          |             |             | postgres=CTc/postgres
 template1    | postgres     | UTF8     | ja_JP.UTF-8 | ja_JP.UTF-8 | =c/postgres                      +
              |              |          |             |             | postgres=CTc/postgres
(4 rows)


postgres=# \c "php-crud-api";
You are now connected to database "php-crud-api" as user "postgres".
#php-crud-api=# CREATE EXTENSION

php-crud-api=# quit;
```




# Create Tables
```
$ cd php-crud-api
$ PGPASSWORD=php-crud-api psql -U php-crud-api -h localhost

php-crud-api=> \i tests/fixtures/blog_pgsql.sql

php-crud-api=> \dt
               List of relations
 Schema |     Name      | Type  |    Owner
--------+---------------+-------+--------------
 public | barcodes      | table | php-crud-api
 public | categories    | table | php-crud-api
 public | comments      | table | php-crud-api
 public | events        | table | php-crud-api
 public | invisibles    | table | php-crud-api
 public | kunsthandvark | table | php-crud-api
 public | nopk          | table | php-crud-api
 public | post_tags     | table | php-crud-api
 public | posts         | table | php-crud-api
 public | products      | table | php-crud-api
 public | tags          | table | php-crud-api
(11 rows)


php-crud-api=> select * from posts;
 id | user_id | category_id |   content
----+---------+-------------+--------------
  1 |       1 |           1 | blog started
  2 |       1 |           2 | It works!
(2 rows)

php-crud-api=> quit;
```

# Install PHP & PDO driver for MySQL
```
$ sudo apt install php

$ sudo apt install php-pgsql

$ php --version
PHP 7.4.3 (cli) (built: Mar  2 2022 15:36:52) ( NTS )
Copyright (c) The PHP Group
Zend Engine v3.4.0, Copyright (c) Zend Technologies
    with Zend OPcache v7.4.3, Copyright (c), by Zend Technologies
```

# Change api.php for postgres
```
$ cd php-crud-api

$ vi api.php

    $config = new Config([
        'driver' => 'pgsql',
        // 'address' => 'localhost',
        // 'port' => '3306',
        'username' => 'php-crud-api',
        'password' => 'php-crud-api',
        'database' => 'php-crud-api',
        // 'debug' => false
    ]);
```


# Start Built-in WEB Server
```
$ php -S 0.0.0.0:8080 -t $HOME/php-crud-api
[Mon Mar 28 15:47:39 2022] PHP 7.4.3 Development Server (http://0.0.0.0:8080) started
```

# Test php-crud-api
```
$ curl http://localhost:8080/api.php/records/posts/ | python -mjson.tool
  % Total    % Received % Xferd  Average Speed   Time    Time     Time  Current
                                 Dload  Upload   Total   Spent    Left  Speed
100   134  100   134    0     0    498      0 --:--:-- --:--:-- --:--:--   496
{
    "records": [
        {
            "id": 1,
            "user_id": 1,
            "category_id": 1,
            "content": "blog started"
        },
        {
            "id": 2,
            "user_id": 1,
            "category_id": 2,
            "content": "It works!"
        }
    ]
}
```

