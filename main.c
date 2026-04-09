#include <gtk/gtk.h>
#include "producto.h"
#include "database.h"
#include <string.h>

// Estructura para pasar datos a la app
typedef struct {
    sqlite3 *db;
    GListStore *productos_store; //Almacén de objetos ProductoObj
    GListStore *venta_actual_store; //Almacen de la tabla de ventas principal
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

static void on_seleccion_producto_changed(GtkSelectionModel *model, guint position, guint n_items, gpointer user_data);

//confirma accion del boton de añadir para enviar producto a pestaña de ventas
void on_btn_confirmar_add_clicked(GtkButton *btn, gpointer user_data);

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

	data->venta_actual_store = g_list_store_new(PRODUCTO_TYPE_OBJ);

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
 
    //recuperando el builder pegado abajo
    GtkBuilder *builder = g_object_get_data(G_OBJECT(editable), "m_builder");

    // 1. Limpiar store
    g_list_store_remove_all(data->productos_store);

    if(builder){
    //Limpiamos el entry donde el producto va a ser mostrado para que el usuario vea que agregó
    GtkEditable *entry_sel = GTK_EDITABLE(gtk_builder_get_object(builder, "entry_producto_seleccionado"));
    if (entry_sel) {
        gtk_editable_set_text(entry_sel, ""); 
    }
    }

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
        GtkColumnView *cv_ventas; //column view donde estan las ventas

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
	//obtener el column view donde se verá el carrito de productos
       cv_ventas = GTK_COLUMN_VIEW(gtk_builder_get_object(builder, "productos_venta"));
 
    	if (cv_ventas) {
    GtkNoSelection *selection = gtk_no_selection_new(G_LIST_MODEL(data->venta_actual_store));
    gtk_column_view_set_model(cv_ventas, GTK_SELECTION_MODEL(selection));
    g_object_unref(selection);

    configurar_columnas_venta(builder, data->venta_actual_store); 
}

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

    //limpiar el almacen de datos para que no aparezca lo anterior
        g_list_store_remove_all(data->productos_store);

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
    gtk_editable_set_text(GTK_EDITABLE(entry_busqueda), "");
    cv = GTK_COLUMN_VIEW(gtk_builder_get_object(builder, "column_view_productos"));

    // Limpiar visualmente el Entry de producto seleccionado al abrir
    GtkEditable *entry_sel = GTK_EDITABLE(gtk_builder_get_object(builder, "entry_producto_seleccionado"));
    if (entry_sel) gtk_editable_set_text(entry_sel, "");

    //obtener la ventana de ventas
    GtkWindow *ventana_padre = gtk_application_get_active_window(GTK_APPLICATION(g_application_get_default()));
    
    // Establecemos que la ventana de añadir productos depende de la principal
    if (ventana_padre != NULL) {
        gtk_window_set_transient_for(GTK_WINDOW(search_window), ventana_padre);
    }
    gtk_window_set_modal(GTK_WINDOW(search_window), TRUE);

    // Configurar Factories (Llamada a producto.c)
    producto_configurar_columnas(builder);

    // Conectar el Modelo al ColumnView
    if (cv) {
        GtkSingleSelection *selection = gtk_single_selection_new(G_LIST_MODEL(data->productos_store));
	
	//asegurar que no haya nada seleccionado por defecto al abrir
	gtk_single_selection_set_autoselect(selection, FALSE);
        gtk_single_selection_set_can_unselect(selection, TRUE);
        gtk_single_selection_set_selected(selection, GTK_INVALID_LIST_POSITION);
	gtk_column_view_set_model(cv, GTK_SELECTION_MODEL(selection));
        g_signal_connect(selection, "selection-changed", G_CALLBACK(on_seleccion_producto_changed), builder);
       	g_object_unref(selection);
    }

    //Conectar señal de búsqueda
    // Pasamos el BUILDER en lugar de DATA para que la búsqueda pueda limpiar el otro entry
    g_signal_connect(entry_busqueda, "changed", G_CALLBACK(on_search_entry_changed), data);
    // Necesitamos que on_search_entry_changed tenga acceso al builder. 
    g_object_set_data_full(G_OBJECT(entry_busqueda), "m_builder", builder, g_object_unref);
    //se utiliza para cerrar la ventana cuando se da al boton cancelar
    g_signal_connect_swapped(btn_cancelar_2, "clicked", G_CALLBACK(gtk_window_destroy), search_window);

    // Obtener el botón de añadir del buscador (el de confirmar)
    GtkWidget *btn_add_confirm = GTK_WIDGET(gtk_builder_get_object(builder, "btn_add_confirm")); 
    if (btn_add_confirm) {
    // Le pasamos el builder para que encuentre el SpinButton y el Entry
    g_object_set_data(G_OBJECT(btn_add_confirm), "m_builder", builder);
    g_signal_connect(btn_add_confirm, "clicked", G_CALLBACK(on_btn_confirmar_add_clicked), data);
    }


    gtk_window_present(GTK_WINDOW(search_window));
}

static void on_seleccion_producto_changed(GtkSelectionModel *model, guint position, guint n_items, gpointer user_data){
    GtkBuilder *builder = (GtkBuilder *)user_data;
  
   //obtener seleccion real actual 
   GtkBitset *selection = gtk_selection_model_get_selection(model);
    
    if (!gtk_bitset_is_empty(selection)) {
        // Obtenemos el primer (y único) índice seleccionado
        guint selected_pos = gtk_bitset_get_nth(selection, 0);

    //Obtener el objeto seleccionado
    ProductoObj *seleccionado = g_list_model_get_item(G_LIST_MODEL(model), selected_pos);
    
    if (seleccionado != NULL) {
        //Obtener el widget donde mostraremos el nombre
        GtkEditable *entry_nombre = GTK_EDITABLE(gtk_builder_get_object(builder, "entry_producto_seleccionado"));
        
        
        if(entry_nombre){
        // Poner el nombre del objeto en el Entry
        gtk_editable_set_text(entry_nombre, producto_obj_get_nombre(seleccionado));
	//guardamos el objeto, se adjunta al widget para utilizarlo después
	g_object_set_data_full(G_OBJECT(entry_nombre), "producto_actual", seleccionado, g_object_unref);
	}

    }
}
     gtk_bitset_unref(selection);
}

void on_btn_confirmar_add_clicked(GtkButton *btn, gpointer user_data) {
    AppData *data = (AppData *)user_data;
    GtkBuilder *builder = g_object_get_data(G_OBJECT(btn), "m_builder");

    GtkEditable *entry_nombre = GTK_EDITABLE(gtk_builder_get_object(builder, "entry_producto_seleccionado"));
    GtkSpinButton *spin_cant = GTK_SPIN_BUTTON(gtk_builder_get_object(builder, "spin_cantidad"));
    
    //Recuperar el objeto que guardamos en on_seleccion_producto_changed
    ProductoObj *producto_sel = g_object_get_data(G_OBJECT(entry_nombre), "producto_actual");
    int cantidad = gtk_spin_button_get_value_as_int(spin_cant);

    if (producto_sel && cantidad > 0) {
        //setear cantidad 
	producto_obj_set_cantidad(producto_sel, cantidad);
	    //Añadir al store de la venta principal
        g_list_store_append(data->venta_actual_store, producto_sel);

        // Cerrar la ventana del buscador
        GtkWidget *window = GTK_WIDGET(gtk_builder_get_object(builder, "search_window"));
        gtk_window_destroy(GTK_WINDOW(window));
    }
}
