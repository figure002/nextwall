#include <stdlib.h>
#include <stdio.h>
#include <sqlite3.h>

#define MAXLINE 1000
#define NEXTWALL_DB_VERSION 0.2

//char *sql_errmsg = 0;
static int rc;

/* function prototypes */
int nextwall_make_db(sqlite3 *db);
int handle_sqlite_response(int rc);

/* example sqlite callback function
static int callback(void *notused, int argc, char **argv, char **colnames) {
   int i;
   for(i=0; i<argc; i++){
      printf("%s = %s\n", colnames[i], argv[i] ? argv[i] : "NULL");
   }
   printf("\n");
   return 0;
}
*/

/* Print the SQLite error message if there was an error. */
int handle_sqlite_response(int rc) {
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", "Unknown");
        //sqlite3_free(sql_errmsg);
        exit(1);
    }
    //sqlite3_free(sql_errmsg);
    return 0;
}

/* Create an empty database with the necessary tables. */
int nextwall_make_db(sqlite3 *db) {
    char *sql;
    char sqlstr[MAXLINE];

    sql = "CREATE TABLE wallpapers (" \
        "id INTEGER PRIMARY KEY," \
        "path TEXT," \
        "kurtosis FLOAT," \
        "defined_brightness INTEGER," \
        "rating INTEGER);";
    rc = sqlite3_exec(db, sql, NULL, NULL, NULL);
    handle_sqlite_response(rc);

    sql = "CREATE TABLE info (" \
        "id INTEGER PRIMARY KEY," \
        "name VARCHAR," \
        "value VARCHAR);";
    rc = sqlite3_exec(db, sql, NULL, NULL, NULL);
    handle_sqlite_response(rc);

    sprintf(sqlstr, "INSERT INTO info VALUES (null, 'version', %f);", NEXTWALL_DB_VERSION);
    rc = sqlite3_exec(db, sqlstr, NULL, NULL, NULL);
    handle_sqlite_response(rc);

    return 0;
}


