#include <gtk/gtk.h>
#include <sqlite3.h>
#include "NotesRepo.c"
#include <stdio.h>

#define MAX_GROUPS_COUNT 25
#define MAX_NOTES_COUNT 100

GtkApplication *app;
GtkWidget *window;
GtkWidget *grid;
GtkWidget *button;
GtkWidget *create_note_button;
GtkWidget *delete_note_group_button;
GtkWidget *edit_Field;
GtkWidget *group_buttons[MAX_GROUPS_COUNT];
GtkWidget *notes_widgets[MAX_NOTES_COUNT];
GtkWidget *label;
GtkWidget *edit_note_dialog;
GtkWidget *edit_note_dialog_name_input;
GtkWidget *edit_note_dialog_text_input;

static void refresh_note_widgets_callback (GtkWidget *widget, gpointer data);
static void refresh_group_buttons();
static void delete_note_group();
static void delete_current_note_group();
static void create_note(int group_id, char *name);
static void refresh_note_widgets(int group_id);

int current_group_id = 0;
int current_note_index = 0;

static void print_stuff(char* stuff)
{
    gtk_label_set_text((GtkLabel *) label, stuff);
}

static void print_hello (GtkWidget *widget, gpointer data)
{
    //gtk_label_set_text((GtkLabel *) label,"Hello World!");
    print_stuff("hello world");
}

static void delete_note_callback (GtkWidget *widget, gpointer data) {
    int note_id = GPOINTER_TO_INT(data);
    repo_delete_note(note_id);

//    g_signal_emit_by_name(GINT_TO_POINTER(1), "clicked");
//    gtk_widget_activate_action();
//    g_signal_connect(group_buttons[i], "clicked", G_CALLBACK(refresh_note_widgets_callback), GINT_TO_POINTER(buffer_group_ids[i]));

}

static void refresh_group_buttons_callback(GtkWidget *widget, gpointer data) {
    refresh_group_buttons();
}

static void edit_note_dialog_save_callback(GtkWidget *widget, gpointer data) {
    int note_id = GPOINTER_TO_INT(data);
    if(note_id == 0) return;

    // Få fat i text bufferen for at få fat i teksten i et GtkTextView
    GtkTextIter start;
    GtkTextIter end;
    GtkTextBuffer *text_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(edit_note_dialog_text_input));
    gtk_text_buffer_set_modified (text_buffer, FALSE);
    gtk_text_buffer_get_start_iter(text_buffer, &start);
    gtk_text_buffer_get_end_iter(text_buffer, &end);

    char *note_text_to_update = gtk_text_buffer_get_text(text_buffer, &start, &end, FALSE);
    char *note_name_to_update = (char *) gtk_editable_get_text(GTK_EDITABLE(edit_note_dialog_name_input));

    repo_update_note(note_id, note_name_to_update, note_text_to_update);
    free(note_text_to_update);
    gtk_window_destroy(GTK_WINDOW(edit_note_dialog));
}

static void edit_note_callback (GtkWidget *widget, gpointer data) {
    int note_id = GPOINTER_TO_INT(data);
    int current_note_index = 0;

    // find noten i bufferen
    for(int i= 0; i < buffer_note_count; i++) {
        if(buffer_note_ids[i] == note_id) {
            current_note_index = i;
            break;
        }
    }
    GtkWidget *edit_node_content_box;
    // create new window
    edit_note_dialog = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(edit_note_dialog), "Rediger note");
    gtk_window_set_default_size(GTK_WINDOW(edit_note_dialog), 320, 200);

    edit_node_content_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_window_set_child(GTK_WINDOW(edit_note_dialog), edit_node_content_box);

    edit_note_dialog_name_input = gtk_entry_new();
    gtk_editable_set_text(GTK_EDITABLE(edit_note_dialog_name_input), buffer_note_names[current_note_index]);

    gtk_box_append(GTK_BOX(edit_node_content_box), edit_note_dialog_name_input);

    edit_note_dialog_text_input = gtk_text_view_new();
    GtkTextTagTable * ttt = gtk_text_tag_table_new();
    GtkTextBuffer *text_buffer = gtk_text_buffer_new(ttt);
    gtk_text_buffer_set_text(text_buffer, buffer_note_text[current_note_index], -1);
    gtk_text_view_set_buffer(GTK_TEXT_VIEW(edit_note_dialog_text_input), text_buffer);
    gtk_widget_set_size_request(edit_note_dialog_text_input, 0, 50);
    gtk_box_append(GTK_BOX(edit_node_content_box), edit_note_dialog_text_input);

    GtkWidget *input_note_save_button = gtk_button_new_with_label("Gem");
    g_signal_connect(input_note_save_button, "clicked", G_CALLBACK(edit_note_dialog_save_callback), GINT_TO_POINTER(note_id));
    g_signal_connect(input_note_save_button, "clicked", G_CALLBACK(refresh_note_widgets_callback), GINT_TO_POINTER(-1));
    gtk_box_append(GTK_BOX(edit_node_content_box), input_note_save_button);

    gtk_widget_set_visible(edit_note_dialog, true);
}

GtkWidget *create_note_widget(int note_id, const char *note_name, const char *note_text) {
    GtkWidget *note = gtk_frame_new(note_name);

    GtkWidget *note_header_label = gtk_label_new(note_name);
    gtk_frame_set_label_widget(GTK_FRAME(note), note_header_label);

    GtkWidget *note_content_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_frame_set_child(GTK_FRAME(note), note_content_box);

    GtkWidget *note_text_label = gtk_label_new(note_text);
    gtk_box_append(GTK_BOX(note_content_box), note_text_label);

    GtkWidget *note_action_bar = gtk_action_bar_new();
    gtk_box_append(GTK_BOX(note_content_box), note_action_bar);

    GtkWidget *note_button_delete = gtk_button_new_with_label("Slet");
    g_signal_connect(note_button_delete, "clicked", G_CALLBACK(delete_note_callback), GINT_TO_POINTER(note_id));
    g_signal_connect(note_button_delete, "clicked", G_CALLBACK(refresh_note_widgets_callback), GINT_TO_POINTER(-1));

    gtk_action_bar_pack_end(GTK_ACTION_BAR(note_action_bar), note_button_delete);

    GtkWidget *note_button_edit = gtk_button_new_with_label("Redigér");
    g_signal_connect(note_button_edit, "clicked", G_CALLBACK(edit_note_callback), GINT_TO_POINTER(note_id));
    gtk_action_bar_pack_start(GTK_ACTION_BAR(note_action_bar), note_button_edit);

    return note;
}

static void refresh_note_widgets_callback (GtkWidget *widget, gpointer data)
{
    if(data == NULL) return;

    int group_id = GPOINTER_TO_INT(data);

    if(group_id == -1 && current_group_id > 0) {
        group_id = current_group_id;
    }

    if(group_id == 0) return;

    refresh_note_widgets(group_id);
}

static void refresh_note_widgets(int group_id)
{
    if(group_id == 0) {
        return;
    }

    current_group_id = group_id;

    // Slet alle noter i grid'et.
    for(int i=0; i < buffer_note_count; i++) {
        gtk_grid_remove(GTK_GRID(grid), GTK_WIDGET(notes_widgets[i]));
    }

    int groups_loaded = repo_load_notes_into_buffer(group_id);

    char *msg = malloc(200);
    sprintf(msg, "antal noter i gruppen (id:%d) %d", group_id, groups_loaded);
    print_stuff(msg);

    // Opret en widget til hver note og placer i griddet. Gem en reference i array til husholdningen
    for(int i=0; i < buffer_note_count; i++) {
        GtkWidget *new_note = create_note_widget(buffer_note_ids[i], buffer_note_names[i], buffer_note_text[i]);
        int col_offset = 1;
        int row_offset = 4;
        int col =(i % 4) * 2;
        int row = (i / 4) * 2;
        gtk_grid_attach(GTK_GRID(grid), new_note, col_offset + col, row_offset + row, 2, 2);
        notes_widgets[i] = new_note;
    }
}

static void create_note_group_callback(GtkWidget *widget, gpointer data)
{
    // Indlæs tekst fra indtastningsfeltet
    const char *new_group_name = gtk_editable_get_text(GTK_EDITABLE(edit_Field));

    // Hvis ikke teksten er tom, så opret en ny gruppe i databasen
    if(strlen(new_group_name) > 0)
    {
        int64_t new_group_id = repo_create_group(new_group_name);

        char *msg = malloc(200);
        sprintf(msg, "Gruppe oprettet med navnet '%s' og ID=%lld", new_group_name, new_group_id);
        print_stuff(msg);

        // Fjern tekst fra indtastningsfeltet
        gtk_editable_set_text(GTK_EDITABLE(edit_Field), "");

        free(msg);
    }
    else
    {
        print_stuff("Du skal da skrive et navn til gruppen");
        gtk_root_set_focus(GTK_ROOT(window), edit_Field);
    }

}

static void btn_test_click(GtkWidget *widget, gpointer data)
{
    int c = repo_get_group_count();

    char *msg = malloc(200);
    if(msg) {
        sprintf(msg, "Fandt %d grupper i databasen", c);
        print_stuff(msg);

    }
    free(msg);
}

static void create_note(int group_id, char *name)
{
    repo_create_note(name, group_id);

}

static void create_note_callback(GtkWidget *widget, gpointer data)
{
    if(current_group_id == 0) {
        print_stuff("Du skal vælge en gruppe først. Klik på en gruppe-knap.");
        return;
    }

    create_note(current_group_id, "Ny note");
}

static void refresh_group_buttons()
{
    for (int i = 0; i < buffer_group_count; i++) {
        gtk_grid_remove(GTK_GRID(grid), GTK_WIDGET(group_buttons[i]));
    }

    repo_load_groups_into_buffer();

    for (int i = 0; i < buffer_group_count; i++) {

        // create button for every group
        group_buttons[i] = gtk_button_new_with_label(buffer_group_names[i]);
        g_signal_connect(group_buttons[i], "clicked", G_CALLBACK(refresh_note_widgets_callback), GINT_TO_POINTER(buffer_group_ids[i]));
        gtk_grid_attach(GTK_GRID(grid), group_buttons[i], 0, i+4, 1, 1);
    }
}


static void delete_note_group_callback(GtkWidget *widget, gpointer data)
{
    //delete_note_group();
    delete_current_note_group();
};

static void delete_current_note_group() {
    // Tjek om current_group_id er sat
    if(current_group_id == 0) {
        print_stuff("Der er ikke valgt en notegruppe. Klik på en gruppe først for at slette den.");
        return;
    }

    // Slet hvis ingen noter findes i denne gruppe
    int notes_count = repo_get_notes_count(current_group_id);
    if(notes_count > 0)
    {
        print_stuff("Der findes noter i gruppen. De skal slettes først.");
        return;
    }

    // Gruppen er tom. Den kan slettes nu.
    repo_delete_group(current_group_id);
    current_group_id = 0;
}


static void delete_note_group()
{
    // Indlæs tekst fra indtastningsfeltet
    const char *to_be_deleted_group_id_str = gtk_editable_get_text(GTK_EDITABLE(edit_Field));

    // Hvis ikke teksten er tom, så opret en ny gruppe i databasen
    if(strlen(to_be_deleted_group_id_str) > 0)
    {
        int to_be_deleted_group_id = atoi(to_be_deleted_group_id_str);

        if(to_be_deleted_group_id == 0)
        {
            print_stuff("Angiv gyldigt id til gruppen der skal slettes");
            gtk_root_set_focus(GTK_ROOT(window), edit_Field);
            return;
        }

        repo_delete_group(to_be_deleted_group_id);

        // Fjern tekst fra indtastningsfeltet
        gtk_editable_set_text(GTK_EDITABLE(edit_Field), "");
        return;
    }
    else
    {
        print_stuff("Angiv id til gruppen der skal slettes");
        gtk_root_set_focus(GTK_ROOT(window), edit_Field);
        return;
    }
}

static void callback_error_log(char * msg)
{
    print_stuff(msg);
}

//static void print_text()
//{
//    print_stuff("test");
//    gtk_label_set_text((GtkLabel *) label, gtk_editable_get_text(GTK_EDITABLE(edit_Field)));
//    //int group_count = repo_get_group_count();
////    note *notes = get_notes(1);
//
////    char str[500] = "";
////    for (int i = 0; i < 5; i++) {
////        char buff[50] = "";
////        sprintf(buff, "%d) %s\n", notes[i].note_id, notes[i].note_name);
////        strcat(str, buff);
////    }
////    print_stuff(str);
//}

static void quit_application(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
    GApplication *app = G_APPLICATION (user_data);
    repo_close_database_connection();
    gtk_window_destroy(GTK_WINDOW(window));
}

static void activate(GtkApplication *app, gpointer user_data)
{
    /* create a new window, and set it's title */
    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Notes");
    gtk_window_set_default_size(GTK_WINDOW(window), 300, 150);

    /* Here we construct the container that is going pack our buttons */
    grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID (grid), 6);
    gtk_grid_set_column_spacing(GTK_GRID (grid), 6);
    gtk_widget_set_margin_top(grid, 10);
    gtk_widget_set_margin_start(grid, 10);
    gtk_widget_set_margin_bottom(grid, 10);

    /* Pack the container in the window */
    gtk_window_set_child(GTK_WINDOW (window), grid);

    /* Place the first button in the grid cell (0, 0), and make it fill
    * just 1 cell horizontally and vertically (ie no spanning)*/
//    button = gtk_button_new_with_label("Button 1");
//    g_signal_connect(button, "clicked", G_CALLBACK(print_text), NULL);
//    gtk_grid_attach(GTK_GRID(grid), button, 0, 0, 1, 1);

    button = gtk_button_new_with_label("Genindlæs");
    g_signal_connect(button, "clicked", G_CALLBACK(refresh_group_buttons_callback), NULL);
    gtk_grid_attach(GTK_GRID(grid), button, 0, 0, 1, 1);

    /* Place the second button in the grid cell (1, 0), and make it fill
     * just 1 cell horizontally and vertically (ie no spanning)*/
//    button = gtk_button_new_with_label("Button 2");
//    g_signal_connect(button, "clicked", G_CALLBACK (print_hello), NULL);
//    gtk_grid_attach(GTK_GRID(grid), button, 1, 0, 1, 1);

    /* Place the Quit button in the grid cell (0, 1), and make it
     * span 2 columns.*/
    button = gtk_button_new_with_label("Quit");
    g_signal_connect_swapped(button, "clicked", G_CALLBACK (quit_application), NULL);
    gtk_grid_attach(GTK_GRID(grid), button, 2, 0, 1, 1);

//    button = gtk_button_new_with_label("Test");
//    g_signal_connect_swapped(button, "clicked", G_CALLBACK (btn_test_click), NULL);
//    gtk_grid_attach(GTK_GRID(grid), button, 1, 2, 1, 1);

    button = gtk_button_new_with_label("Opret gruppe");
    g_signal_connect_swapped(button, "clicked", G_CALLBACK(create_note_group_callback), NULL);
    g_signal_connect_swapped(button, "clicked", G_CALLBACK(refresh_group_buttons_callback), NULL);
    gtk_grid_attach(GTK_GRID(grid), button, 2, 2, 1, 1);

    create_note_button = gtk_button_new_with_label("Ny note");
    g_signal_connect(create_note_button, "clicked", G_CALLBACK(create_note_callback), NULL);
    g_signal_connect(create_note_button, "clicked", G_CALLBACK(refresh_note_widgets_callback), GINT_TO_POINTER(-1));
    gtk_grid_attach(GTK_GRID(grid), create_note_button, 4, 2, 1, 1);

    delete_note_group_button = gtk_button_new_with_label("Slet gruppe");
    g_signal_connect(delete_note_group_button, "clicked", G_CALLBACK(delete_note_group_callback), NULL);
    g_signal_connect(delete_note_group_button, "clicked", G_CALLBACK(refresh_group_buttons_callback), NULL);
    gtk_grid_attach(GTK_GRID(grid), delete_note_group_button, 3, 2, 1, 1);

    /* Entry field */
    edit_Field = gtk_entry_new();
    gtk_grid_attach(GTK_GRID(grid), edit_Field, 0, 2, 1, 1);

    /* label */
//    label = gtk_label_new("Hej Verden!");
//    gtk_grid_attach(GTK_GRID(grid), label, 0, 3 , 1, 1);

    gtk_window_present(GTK_WINDOW (window));

    char *result_text = repo_open_database_connection("notes.db");
    print_stuff(result_text);

    refresh_group_buttons();

}


int main (int argc, char **argv)
{
    int status;

    app = gtk_application_new("unord.htg.stickynotes", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;
}