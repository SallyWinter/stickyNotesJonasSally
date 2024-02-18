//#include <gtk/gtk.h>
#include <sqlite3.h>
#include <stdbool.h>
#include <stdio.h>

// Database variables
sqlite3 *db;
char *last_database_error_message = 0;
int resultCode;
char *sql;
const char* data = "Callback function called";

#define GROUP_BUFFER_SIZE 25
#define GROUP_NAME_LENGTH 20
char buffer_group_names[GROUP_BUFFER_SIZE][GROUP_NAME_LENGTH];
long buffer_group_ids[GROUP_BUFFER_SIZE] = {0};
int buffer_group_count = 0;

#define NOTE_BUFFER_SIZE 100
#define NOTE_NAME_LENGTH 50
#define NOTE_TEXT_LENGTH 1000
char buffer_note_names[NOTE_BUFFER_SIZE][NOTE_NAME_LENGTH];
char buffer_note_text[NOTE_BUFFER_SIZE][NOTE_TEXT_LENGTH];
long buffer_note_ids[NOTE_BUFFER_SIZE] = {0};
int buffer_note_count = 0;

//
int repo_get_count(char const* sqlCommand)
{
    int output = 0;
    sqlite3_stmt *stmt;
    resultCode = sqlite3_prepare_v2(db, sqlCommand, -1, &stmt, NULL);

    if (resultCode != SQLITE_OK) {
        // error handling -> statement not prepared
        sqlite3_finalize(stmt);
        return -1;
    }
    resultCode = sqlite3_step(stmt);
    if (resultCode != SQLITE_ROW) {
        // error handling -> no rows returned, or an error occurred
        sqlite3_finalize(stmt);
        return -2;
    }

    output = sqlite3_column_int(stmt, 0);

    sqlite3_finalize(stmt);

    return output;
}

// Kaldes via SQL SELECT ID, Name FROM Groups
int _db_load_group_callback(void *data, int argc, char **argv, char **azColName)
{
    if(buffer_group_count > GROUP_BUFFER_SIZE) {
        return 0;
    }

    // Fyld bufferen op og opdater tælleren
    char *ptr;
    if(argc >= 2) {
        buffer_group_ids[buffer_group_count] = strtol(argv[0], &ptr, 10);
        strcpy(buffer_group_names[buffer_group_count], argv[1]);
        buffer_group_count++;
    }
    return 0;
}

//
int repo_load_groups_into_buffer() {
    buffer_group_count = 0;
    sql = "SELECT ID, Name FROM Groups ORDER BY ID ASC LIMIT 25";
    resultCode = sqlite3_exec(db, sql, _db_load_group_callback, (void *) data, &last_database_error_message);

    if (resultCode != SQLITE_OK) {
        sqlite3_free(last_database_error_message);
    } else {
        last_database_error_message = "OK";
    }
    return buffer_group_count * 1; // returner antallet af gruper: denne variabel er blevet opdateret i callback
}

//
int repo_get_group_count()
{
    int output = repo_get_count("SELECT count(*) GroupCount FROM Groups");
    return output;
}

int repo_get_notes_count(int group_id)
{
    char* sql = sqlite3_mprintf("SELECT count(*) NotesCount FROM Notes WHERE GroupID = %d", group_id);
    int output = repo_get_count(sql);
    free(sql);
    return output;
}


int db_load_notes_callback(void *not_used, int argc, char **argv, char **azColumnName)
{
   if(buffer_note_count > NOTE_BUFFER_SIZE) {
        return 0;
    }

    // Fyld bufferen op og opdater tælleren
    char *ptr;
    if(argc >= 2) {
        int id = strtol(argv[0], &ptr, 10);
        if(id > 0) {
            buffer_note_ids[buffer_note_count] = id;
            if(argv[1] != NULL)
            {
                strcpy(buffer_note_names[buffer_note_count], argv[1]);
            }
            if(argv[2] != NULL)
            {
                strcpy(buffer_note_text[buffer_note_count], argv[2]);
            }
            buffer_note_count++;
        }
    }
    return 0;
}


int repo_load_notes_into_buffer(int group_id)
{
    //SELECT ID, Name, Text FROM Notes WHERE GroupID=1;

    /*
     * Ideen er at indlæse bufferne, der kan tilgås fra main.c, så man kalder en "load"-funktion
     * og derefter tager man fat i bufferen, hvis load funtionen returnerer et tal større end 0.
     * Loadfunktionerne starter med at nulstille antallet af fundne poster (buffer_count), kalder så
     * databasen, der så kalder callbacks og til sidst vil bufferen være fyldt op.
     * callback funktionen skal lægge 1 til hver gang den kører for den aktuelle buffers tæller - der er en tæller til
     * både grupper og noter.
     */
    buffer_note_count = 0;

    for(int i = 0; i < NOTE_BUFFER_SIZE; i ++)
    {
        buffer_note_ids[i] = 0;
        strcpy(buffer_note_names[i], "");
        strcpy(buffer_note_text[i],  "");
    }

    char* sql = sqlite3_mprintf("SELECT ID, Name, Text FROM Notes WHERE GroupID = %d ORDER BY ID ASC LIMIT 100;", group_id);
    resultCode = sqlite3_exec(db, sql, db_load_notes_callback, (void *) data, &last_database_error_message);

    if (resultCode != SQLITE_OK) {
        sqlite3_free(last_database_error_message);
    } else {
        last_database_error_message = "OK";
    }
    sqlite3_free(sql);

    return buffer_note_count * 1; // returner antallet af noter: denne variabel er blevet opdateret i callback
}

// opretter gruppe
int64_t repo_create_group(const char* group_name)
{
    //INSERT INTO Groups( Name)
    //VALUES ( 'gruppenavnetf');
    char* query = sqlite3_mprintf("INSERT INTO Groups(Name) VALUES ('%q');", group_name);

    char* errMsg = 0;

    resultCode = sqlite3_exec(db, query, NULL, NULL, &errMsg);

    if (resultCode != SQLITE_OK) {

        //g_debug("Error in SQL statement:", last_database_error_message);

        sqlite3_free(errMsg);
        sqlite3_free(query);

        sqlite3_close(db);

        return -1;
    }

    sqlite3_free(query);

    int64_t last_row_id = 0;
    last_row_id = sqlite3_last_insert_rowid(db);

    return last_row_id;
}
//opretter note
int64_t repo_create_note(char* note_name, int group_id)
{
    //INSERT INTO Notes (Name, GroupID)
    //VALUES ('ting og sager', 2);
    char* query = sqlite3_mprintf("INSERT INTO Notes(Name, GroupID) VALUES ('%q', %d);", note_name, group_id);

    resultCode = sqlite3_exec(db, query, NULL, NULL, &last_database_error_message);

    if (resultCode != SQLITE_OK) {
        sqlite3_free(query);
        sqlite3_close(db);
        return -1;
    }
    sqlite3_free(query);

    int64_t last_row_id = 0;
    last_row_id = sqlite3_last_insert_rowid(db);

    return last_row_id;
}

int set_note_color()
{
    return true;
}

//sletter gruppe
int repo_delete_group(int group_id)
{
    //DELETE FROM Groups WHERE ID=5;
    char* query = sqlite3_mprintf("DELETE FROM Groups WHERE ID=%d;", group_id);

    char* errMsg = 0;

    resultCode = sqlite3_exec(db, query, NULL, NULL, &errMsg);

    if (resultCode != SQLITE_OK) {

        //g_debug("Error in SQL statement:", last_database_error_message);

        sqlite3_free(errMsg);
        sqlite3_free(query);

        return -1;
    }

    sqlite3_free(query);
    return true;
}

//sletter note
int repo_delete_note(int note_id)
{
    //DELETE FROM Notes WHERE ID=5;
    char* query = sqlite3_mprintf("DELETE FROM Notes WHERE ID=%d;", note_id);

    char* errMsg = 0;

    resultCode = sqlite3_exec(db, query, NULL, NULL, &errMsg);

    if (resultCode != SQLITE_OK) {

        //g_debug("Error in SQL statement:", last_database_error_message);

        sqlite3_free(errMsg);
        sqlite3_free(query);

        return -1;
    }

    sqlite3_free(query);
    return true;
}
//bliver ikke brugt
int repo_lock_group(char* password)
{
    //UPDATE Groups
    //SET Password = 'superhemmelig'
    //WHERE ID = 1;
    return true;
}

int repo_update_note(int note_id, char *name, char* text)
{
    //UPDATE Notes
    //SET Name = 'Sallys note'
    //SET Text = 'Alfred Schmidt er morderen på slottet!'
    //WHERE ID = 1;
    char* query = sqlite3_mprintf("UPDATE Notes SET Name = '%q', Text = '%q' WHERE ID=%d;", name, text, note_id);

    char* errMsg = 0;

    resultCode = sqlite3_exec(db, query, NULL, NULL, &errMsg);

    if (resultCode != SQLITE_OK) {

        sqlite3_free(errMsg);
        sqlite3_free(query);

        return -1;
    }

    sqlite3_free(query);

    return true;
}



char *repo_open_database_connection(char *database_file_name) {

    resultCode = sqlite3_open(database_file_name, &db);

    if(resultCode)
    {
        strcpy(last_database_error_message, sqlite3_errmsg(db));

        char *err_msg;
        strcat(err_msg, "Can't open database:");
        return err_msg;
    }
    else
    {
        // Tjek om databasen er tom. Hvis der ikke er nogle tabeller, så opret dem med en SQL sammen med en enke note.
        int notes_table_count = repo_get_count(
                "SELECT count(*) FROM sqlite_master WHERE type='table' and (name LIKE 'Groups' OR name LIKE 'Notes')");

        if(notes_table_count != 2) {
            char *create_all_tables = "BEGIN TRANSACTION;\n"
                                      "CREATE TABLE IF NOT EXISTS \"Notes\" (\n"
                                      "\"ID\"\tINTEGER NOT NULL UNIQUE,\n"
                                      "\"Text\"\tTEXT,\n"
                                      "\"Color\"\tTEXT,\n"
                                      "\"GroupID\"\tINTEGER,\n"
                                      "\"Name\"\tTEXT,\n"
                                      "FOREIGN KEY(\"GroupID\") REFERENCES \"Groups\"(\"ID\"),\n"
                                      "PRIMARY KEY(\"ID\" AUTOINCREMENT)\n"
                                      ");\n"
                                      "CREATE TABLE IF NOT EXISTS \"Groups\" (\n"
                                      "\"ID\"\tINTEGER NOT NULL UNIQUE,\n"
                                      "\"Name\"\tTEXT,\n"
                                      "\"Password\"\tTEXT,\n"
                                      "PRIMARY KEY(\"ID\" AUTOINCREMENT)\n"
                                      ");\n"
                                      "INSERT INTO Groups(Name) VALUES('Installationsnoter');\n"
                                      "INSERT INTO Notes(Name, Text, GroupID)\n"
                                      "SELECT\n"
                                      " 'Min første note' AS \"Name\",\n"
                                      " 'Denne note er oprettet automatisk under installationen.\\nGod fornøjelse!' AS \"Text\",\n"
                                      "  (SELECT MAX(ID) FROM Groups) AS GroupID;\n"
                                      "COMMIT;";
            resultCode = sqlite3_exec(db, create_all_tables, NULL, (void *) data, &last_database_error_message);

            if (resultCode != SQLITE_OK) {
                sqlite3_free(last_database_error_message);
            } else {
                last_database_error_message = "OK";
            }
        }


        return "Opened database successfully";
    }
}

int repo_close_database_connection() {
    return sqlite3_close(db);
}

