#include <gtk/gtk.h>
#include "producto.h"
#include "database.h"
#include <string.h>

// Estructura para pasar datos a la app
typedef struct {
    sqlite3 *db;
    GListStore *productos_store; //Almacén de objetos ProductoObj
} AppData;

//PROTOTIPOS DE FUNCIONES

//activar ventana principal
static void activate(GtkApplication *app, gpointer user_data);

//devuelve string con la hora
char *obtener_hora_actual();

//actualiza el label de la hora
static gboolean actualizar_reloj(gpointer user_data);

//funcion para aplicar estilo de css al programa
static void estilos();

//funcion para abrir ventana de añadir producto
void on_btn_abrir_buscador_clicked(GtkButton *btn, gpointer ventana_princial);

static void on_search_entry_changed(GtkEditable *editable, gpointer user_data);

int main(int argc, char **argv){
	GtkApplication *app;
	int status;

	// Inicializar la base de datos antes de arrancar GTK
        sqlite3 *db = inicializar_db("punto_de_venta.db");
        if (!db) return 1;

	//empaquetar la base de datos en la estructura
	AppData *data = g_malloc(sizeof(AppData));
        data->db = db;

	// El store se inicializará cuando se abra el buscador o en activate
        data->productos_store = g_list_store_new(PRODUCTO_TYPE_OBJ);

	app = gtk_application_new("besto.team.struct", G_APPLICATION_DEFAULT_FLAGS);
	
	//activate crea varias cosas de la ventana y se le pasa parametro data con informacion de la base de datos etc.
	g_signal_connect(app, "activate", G_CALLBACK(activate), data);

	//se utiliza al final. deja a la ventana en bucle y espera al usuario
	//no escribir código despues de estas líneas. 
	status = g_application_run(G_APPLICATION(app), argc, argv);
	cerrar_db(db);
	g_free(data);
	g_object_unref(app);
	return status;

}

// --- CALLBACK DE BÚSQUEDA (INTEGRACIÓN SQLITE) ---
static void on_search_entry_changed(GtkEditable *editable, gpointer user_data) {
    AppData *data = (AppData *)user_data;
    const char *texto = gtk_editable_get_text(editable);

    // 1. Limpiar lista actual
    g_list_store_remove_all(data->productos_store);

    if (strlen(texto) == 0) return;

    sqlite3_stmt *stmt;
    // Consulta SQL usando LIKE para búsqueda parcial
    const char *sql = "SELECT id, nombre, precio, existencia FROM productos WHERE nombre LIKE ?;";

    if (sqlite3_prepare_v2(data->db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        // Formateamos el texto para SQLite: %texto%
        char *busqueda = g_strdup_printf("%%%s%%", texto);
        sqlite3_bind_text(stmt, 1, busqueda, -1, g_free);

        while (sqlite3_step(stmt) == SQLITE_ROW) {
            int id = sqlite3_column_int(stmt, 0);
            const char *nombre = (const char *)sqlite3_column_text(stmt, 1);
            double precio = sqlite3_column_double(stmt, 2);
            int existencia = sqlite3_column_int(stmt, 3);

            // Creamos el objeto con los datos reales de la DB
            ProductoObj *p = producto_obj_new(id, nombre, precio, existencia);
            g_list_store_append(data->productos_store, p);
            g_object_unref(p); // El store mantiene la referencia
        }
        sqlite3_finalize(stmt);
    } else {
        g_printerr("Error en la consulta: %s\n", sqlite3_errmsg(data->db));
    }
}


static void activate(GtkApplication *app, gpointer user_data){
	//Recuperar la estructura que trae la db
	AppData *data = (AppData *)user_data;
	
	//cargar estilos css con funcion personalizada
	estilos();

	GtkBuilder *builder; //builder que ayuda a obtener widgets del xml
	GtkWidget *window; //ventana principal
	GtkWidget *reloj; //label de fecha y hora
	GtkWidget *btn_añadir; //boton de añadir productos ventana ventas
		
	//crea el builder y cargar el archivo xml(.ui)
	builder = gtk_builder_new_from_file("pos_ALPS.ui");

	//obtener el objeto de ventana principal
	window = GTK_WIDGET(gtk_builder_get_object(builder,"main_window"));

	//obtener el label de la fecha y hora
	reloj = GTK_WIDGET(gtk_builder_get_object(builder, "fecha_label"));

	//temporizador actualiza cada segundo para la hora
	g_timeout_add_seconds(1, actualizar_reloj, reloj);

	//vincular ventana a la app actual
	gtk_window_set_application(GTK_WINDOW(window), app);

	//obtener el boton de agregar 
	btn_añadir = GTK_WIDGET(gtk_builder_get_object(builder, "btn_add_product"));	
		
	//al pulsar el boton añadir abrir la ventana del buscador
	g_signal_connect(btn_añadir, "clicked", G_CALLBACK(on_btn_abrir_buscador_clicked), data);

	//mostrar la ventana
	gtk_window_present(GTK_WINDOW(window));

	//liberar el builder una vez que la ventana ya está en memoria
	g_object_unref(builder);



}

char *obtener_hora_actual(){
	GDateTime *now = g_date_time_new_now_local();
	char *texto_hora = g_date_time_format(now, "%H:%M:%S");
	g_date_time_unref(now);
	return texto_hora;
}


static gboolean actualizar_reloj(gpointer user_data){
	GtkLabel *label = GTK_LABEL(user_data);

	GDateTime *now = g_date_time_new_now_local();
	//formatos para fecha y hora
	char *formato = g_date_time_format(now, "%d/%m/%Y  %H:%M:%S");

	gtk_label_set_text(label, formato);

	g_free(formato);
	g_date_time_unref(now);

	//para que el timer siga corriendo
	return G_SOURCE_CONTINUE; 
}


static void estilos(){
	//crear proveedor de css
	GtkCssProvider *provider = gtk_css_provider_new();

	//cargar el archivo
	gtk_css_provider_load_from_path(provider, "styles.css");

	//obtener la pantalla actual
	GdkDisplay *display = gdk_display_get_default();

	//Añadir el proovedor al contexto de la pantalla
	gtk_style_context_add_provider_for_display(
			display,
			GTK_STYLE_PROVIDER(provider),
			GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
			);

	g_object_unref(provider);
}

void on_btn_abrir_buscador_clicked(GtkButton *btn, gpointer user_data) {
    
    // Recuperamos data que contiene la db y el store
    AppData *data = (AppData *)user_data;	

    GtkBuilder *builder;
    GtkWidget *search_window;
    GtkWidget *btn_cancelar_2;
    GtkWidget *entry_busqueda;
    GtkColumnView *cv;

    builder = gtk_builder_new_from_file("search_product.ui");
    search_window = GTK_WIDGET(gtk_builder_get_object(builder, "search_window"));

    //obtiene el widget para cancelar el añadir
    btn_cancelar_2 = GTK_WIDGET(gtk_builder_get_object(builder, "btn_cancelar_2"));

    //se obtiene el widget de el buscador y el columnview de añadir productos
    entry_busqueda = GTK_WIDGET(gtk_builder_get_object(builder, "search_entry"));

    cv = GTK_COLUMN_VIEW(gtk_builder_get_object(builder, "column_view_productos"));

    //obtener la ventana de ventas
    GtkWindow *ventana_padre = gtk_application_get_active_window(GTK_APPLICATION(g_application_get_default()));
    
    // Establecemos que la ventana de añadir productos depende de la principal
    if (ventana_padre != NULL) {
        gtk_window_set_transient_for(GTK_WINDOW(search_window), ventana_padre);
    }
    gtk_window_set_modal(GTK_WINDOW(search_window), TRUE);
    // ----------------------------------------


    // Configurar Factories (Llamada a producto.c)
    producto_configurar_columnas(builder);

    // Conectar el Modelo al ColumnView
    if (cv) {
        GtkNoSelection *selection = gtk_no_selection_new(G_LIST_MODEL(data->productos_store));
        gtk_column_view_set_model(cv, GTK_SELECTION_MODEL(selection));
        g_object_unref(selection);
    }

    //Conectar señal de búsqueda
    g_signal_connect(entry_busqueda, "changed", G_CALLBACK(on_search_entry_changed), data);

    //se utiliza para cerrar la ventana cuando se da al boton cancelar
    g_signal_connect_swapped(btn_cancelar_2, "clicked", G_CALLBACK(gtk_window_destroy), search_window);

    gtk_window_present(GTK_WINDOW(search_window));

    g_object_unref(builder);
}



