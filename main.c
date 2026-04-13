#include <gtk/gtk.h>
#include "producto.h"
#include "database.h"
#include <string.h>

//Estructura para la venta
typedef struct {
    double subtotal;
    double iva;
    double total;
} VentaActual;

VentaActual venta = {0.0, 0.0, 0.0};

// Estructura para pasar datos a la app
typedef struct {
    sqlite3 *db;
    GListStore *productos_store; //Almacén de objetos ProductoObj
    GListStore *venta_actual_store; //Almacen de la tabla de ventas principal
} AppData;

// Estructura auxiliar para pasar datos al callback de respuesta
// para las funciones del area de inventario de agregar y eliminar...
typedef struct {
    AppData *data;
    GtkWidget *ent_nom;
    GtkWidget *ent_pre;
    GtkWidget *ent_stk;
    GtkWidget *win_dialog;
    GtkBuilder *builder;
} DialogData;

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

//funcion para usar el boton de cancelar venta y pedir confirmacion con un widget de cuadro de dialogo
void on_btn_cancelar_venta_clicked(GtkButton *btn, gpointer user_data);

//despues de que se pulsa boton de cancelar venta y se da que si a la confirmacion del boton de cuadro de dialogo actúa esta funcion
static void on_cancelar_confirmado(GObject *source, GAsyncResult *res, gpointer user_data);

//funcon para utilizar usar los valores recibidos de precios y actualizar los widgets correspondientes en el resumen de la venta
void actualizar_resumen(GtkBuilder *builder);

//funcion que genera un archivo de texto con el ticket de venta
void generar_ticket_archivo(AppData *data, GtkWidget *parent_window);

//funcion para generar ticket al pulsar el boton de generar ticket
void on_btn_generar_ticket_clicked(GtkButton *btn, gpointer user_data);

//funcion para cargar el inventario de la base de datos a las columnas en la pagina de inventarios 
void cargar_inventario(GtkColumnView *cv, sqlite3 *db);

// CRUD DE INVENTARIOS
// funcion para agregar producto cuando se pulsa el boton correspondiente  en la pagina de inventario
void on_btn_inventario_agregar_clicked(GtkButton *btn, gpointer user_data);
// funcion para eliminar producto cuando se pulsa el boton correspondiente en la pagina de inventario
void on_btn_inventario_eliminar_clicked(GtkButton *btn, gpointer user_data);
//apoyo para generar el producto
static void on_guardar_nuevo_producto(GtkButton *btn, gpointer user_data);
//funcion que elimina el producto si la funcion de eliminar el producto que generó el cuadro de dialogo se le pulsa que SI
static void on_confirmar_eliminar_finish(GObject *source_object, GAsyncResult *res, gpointer user_data);


//
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

void on_btn_cancelar_venta_clicked(GtkButton *btn, gpointer user_data) {
    AppData *data = (AppData *)user_data;
    
    // Crear el diálogo de alerta
    GtkAlertDialog *dialog = gtk_alert_dialog_new("¿Estás seguro de cancelar la venta?");
    gtk_alert_dialog_set_detail(dialog, "Esta acción borrará todos los productos del carrito.");
    
    // Definir los botones: el primero (índice 0) es el positivo
    const char *botones[] = {"Sí, borrar todo", "No, mantener", NULL};
    gtk_alert_dialog_set_buttons(dialog, botones);
    
    // Mostrar el diálogo de forma asíncrona
    gtk_alert_dialog_choose(dialog, GTK_WINDOW(gtk_widget_get_root(GTK_WIDGET(btn))), NULL, on_cancelar_confirmado, data);
    
    g_object_unref(dialog);
}    

static void on_cancelar_confirmado(GObject *source, GAsyncResult *res, gpointer user_data) {
    GtkAlertDialog *dialog = GTK_ALERT_DIALOG(source);
    AppData *data = (AppData *)user_data;
    GError *error = NULL;
    int respuesta = gtk_alert_dialog_choose_finish(dialog, res, &error);

    // Si la respuesta es 0 (sabiendo que el botón "Sí" es el índice 0)
    if (respuesta == 0) {
	    //esta funcion de gtk borra el contenido del list_store lo que hará que no haya nada que mostrar que bien ! :] atte: SS
        g_list_store_remove_all(data->venta_actual_store);

	//lo siguiente es para actualizar el resumen de venta cuando se elimina un producto
	//reiniciar valores lógicos
	venta.subtotal = 0.0;
	venta.iva = 0.0;
	venta.total = 0.0;
	
	//recuperar el builder principal
	//obtener la ventana principal que esta activa
	GtkWindow *main_win = gtk_application_get_active_window(GTK_APPLICATION(g_application_get_default()));
	//extraer el builder de la funcion activate
	GtkBuilder *builder_principal = g_object_get_data(G_OBJECT(main_win), "m_builder");
	//llamar a actualizar, con el builder principal correcto
	if (GTK_IS_BUILDER(builder_principal)) {
            actualizar_resumen(builder_principal);
        }
    }
    
    if (error) g_error_free(error);
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
	GtkWidget *btn_cancelar; //boton que cancela la venta, borra carrito
	GtkWidget *btn_generar_ticket; //boton para generar el ticket de venta
	GtkWidget *pag_ventas; //pagina de ventas
	GtkWidget *pag_inventario; //pagina de inventario
	GtkNotebook *notebook; //contenedor de las pestañas o paginas
	GtkWidget *pag_reportes; //pagina de reportes
	GtkWidget *btn_add; //boton de añadir productos para el area de INVENTARIOS(parecido no igual :])
	GtkWidget *btn_del; //boton de borrar producto para el area de INVENTARIOS

	//crea el builder y cargar el archivo xml(.ui)
	builder = gtk_builder_new_from_file("pos_ALPS.ui");

	//obtener el objeto de ventana principal
	window = GTK_WIDGET(gtk_builder_get_object(builder,"main_window"));


	//pegar el puntero del builder a la ventana con etiqueta m_builder
	g_object_set_data_full(G_OBJECT(window), "m_builder", builder, g_object_unref);


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

	//obtener boton cancelar venta
	btn_cancelar = GTK_WIDGET(gtk_builder_get_object(builder, "btn_cancelar_venta"));	
	
	//obtener boton para generar ticket
	btn_generar_ticket = GTK_WIDGET(gtk_builder_get_object(builder, "btn_ticket"));

	notebook = GTK_NOTEBOOK(gtk_builder_get_object(builder, "nb_principal"));
	pag_ventas = GTK_WIDGET(gtk_builder_get_object(builder, "box_ventas"));
        pag_inventario = GTK_WIDGET(gtk_builder_get_object(builder, "box_inventario"));
	pag_reportes = GTK_WIDGET(gtk_builder_get_object(builder, "box_reportes"));
	
	//se asigna el nombre a las paginas del notebook
	gtk_notebook_set_tab_label_text(notebook, pag_ventas, "VENTAS");
        gtk_notebook_set_tab_label_text(notebook, pag_inventario, "INVENTARIO");
 	gtk_notebook_set_tab_label_text(notebook, pag_reportes, "REPORTES");

	//se obtienen los objetos para los botones de la pagina de inventarios
	btn_add = GTK_WIDGET(gtk_builder_get_object(builder, "btn_añadir_nuevo"));
	btn_del = GTK_WIDGET(gtk_builder_get_object(builder, "btn_eliminar"));

	//se usa la señal para conectar el boton cuando se de click y se usan las funciones para agregar o eliminar	
    if (btn_add) {
        g_signal_connect(btn_add, "clicked", G_CALLBACK(on_btn_inventario_agregar_clicked), data);
    }

    if (btn_del) {
        g_signal_connect(btn_del, "clicked", G_CALLBACK(on_btn_inventario_eliminar_clicked), data);
    }


	//señal para conectar el boton de generar ticket con la funcion de callback cuando se haga click
	if (btn_generar_ticket) {
    g_signal_connect(btn_generar_ticket, "clicked", G_CALLBACK(on_btn_generar_ticket_clicked), data);
}
    	if (cv_ventas) {
    GtkNoSelection *selection = gtk_no_selection_new(G_LIST_MODEL(data->venta_actual_store));
    gtk_column_view_set_model(cv_ventas, GTK_SELECTION_MODEL(selection));

    configurar_columnas_venta(builder, data->venta_actual_store); 
}

	//al pulsar el boton añadir abrir la ventana del buscador
	g_signal_connect(btn_añadir, "clicked", G_CALLBACK(on_btn_abrir_buscador_clicked), data);

	//señal para cuando se pulse el boton de cancelar se elimine el carrito
	if (btn_cancelar) {
        g_signal_connect(btn_cancelar, "clicked", G_CALLBACK(on_btn_cancelar_venta_clicked), data);
    }


	// --- LÓGICA PARA LA PESTAÑA DE INVENTARIO ---

    // 1. Configurar las columnas del inventario (Factories)
    // Esto vincula los IDs col_id, col_nombre, etc., del XML con las funciones de producto.c
    producto_configurar_columnas(builder);

    // 2. Obtener el ColumnView del inventario desde el XML
    GtkColumnView *cv_inv = GTK_COLUMN_VIEW(gtk_builder_get_object(builder, "cv_inventario"));

    // 3. Cargar los datos de la base de datos a la tabla
    if (cv_inv) {
        cargar_inventario(cv_inv, data->db);
    } else {
        g_warning("No se encontró el objeto 'cv_inventario' en el archivo .ui");
    }


	//mostrar la ventana
	gtk_window_present(GTK_WINDOW(window));




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

	//buscar la ventana activa
	GtkWindow *main_win = gtk_application_get_active_window(GTK_APPLICATION(g_application_get_default()));

	//Recuperar el builder que se guardó en la ventana activate
	GtkBuilder *builder_principal = g_object_get_data(G_OBJECT(main_win), "m_builder");

    GtkEditable *entry_nombre = GTK_EDITABLE(gtk_builder_get_object(builder, "entry_producto_seleccionado"));
    GtkSpinButton *spin_cant = GTK_SPIN_BUTTON(gtk_builder_get_object(builder, "spin_cantidad"));
    
    //Recuperar el objeto que guardamos en on_seleccion_producto_changed
    ProductoObj *producto_sel = g_object_get_data(G_OBJECT(entry_nombre), "producto_actual");
    int cantidad = gtk_spin_button_get_value_as_int(spin_cant);

    if (producto_sel && cantidad > 0) {
        //setear cantidad 
	producto_obj_set_cantidad(producto_sel, cantidad);

	//esta parte es para actualizar el valor en el resumen de venta
	//sumar al subtotal global: Precio unitario * cantidad
	venta.subtotal += (producto_obj_get_precio(producto_sel) * cantidad);

	//Añadir al store de la venta principal
        g_list_store_append(data->venta_actual_store, producto_sel);

	//actualizar los labels del resumen de la ventana principal con la funcion creada
	actualizar_resumen(builder_principal);

        // Cerrar la ventana del buscador
        GtkWidget *window = GTK_WIDGET(gtk_builder_get_object(builder, "search_window"));
        gtk_window_destroy(GTK_WINDOW(window));
    }
}

void actualizar_resumen(GtkBuilder *builder) {
    //El IVA es del 16%
    venta.iva = venta.subtotal * 0.16;
    venta.total = venta.subtotal + venta.iva;

    // Obtener los objetos Label por su ID en el ui
    GtkLabel *lbl_subtotal = GTK_LABEL(gtk_builder_get_object(builder, "lbl_subtotal_val"));
    GtkLabel *lbl_iva = GTK_LABEL(gtk_builder_get_object(builder, "lbl_iva_val"));
    GtkLabel *lbl_total = GTK_LABEL(gtk_builder_get_object(builder, "lbl_total_val"));

    char buffer[32];

    sprintf(buffer, "$%.2f", venta.subtotal);
    gtk_label_set_text(lbl_subtotal, buffer);

    sprintf(buffer, "$%.2f", venta.iva);
    gtk_label_set_text(lbl_iva, buffer);

    sprintf(buffer, "$%.2f", venta.total);
    gtk_label_set_text(lbl_total, buffer);
}

// --- FUNCIÓN PARA GENERAR EL TICKET CON NOMBRE ÚNICO ---
void generar_ticket_archivo(AppData *data, GtkWidget *parent_window) {
    GDateTime *now = g_date_time_new_now_local();
    
    // Nombre de archivo único (Timestamp)
    char *nombre_archivo = g_date_time_format(now, "ticket_%Y%m%d_%H%M%S.txt");
    // Fecha legible para el contenido
    char *fecha_visual = g_date_time_format(now, "%A %d/%m/%Y  %H:%M");

    FILE *f = fopen(nombre_archivo, "w");
    if (f == NULL) {
        g_free(nombre_archivo);
        g_free(fecha_visual);
        g_date_time_unref(now);
        return;
    }

    // --- Diseño del Ticket ---
    fprintf(f, "==========================================\n");
    fprintf(f, "            ドキドキショップ\n");
    fprintf(f, "             Doki Doki Shop\n");
    fprintf(f, "==========================================\n");
    fprintf(f, "Fecha: %s\n", fecha_visual);
    fprintf(f, "------------------------------------------\n");
    fprintf(f, "%-25s %-5s %10s\n", "PRODUCTO", "CANT", "PRECIO");
    fprintf(f, "------------------------------------------\n");

    guint n_items = g_list_model_get_n_items(G_LIST_MODEL(data->venta_actual_store));
    for (guint i = 0; i < n_items; i++) {
        ProductoObj *p = g_list_model_get_item(G_LIST_MODEL(data->venta_actual_store), i);
        fprintf(f, "%-25s x%-5d $%10.2f\n", 
                producto_obj_get_nombre(p), 
                producto_obj_get_cantidad(p),
                producto_obj_get_precio(p) * producto_obj_get_cantidad(p));
        g_object_unref(p);
    }

    fprintf(f, "------------------------------------------\n");
    fprintf(f, "小計\n");
    fprintf(f, "Subtotal: %31.2f\n", venta.subtotal);
    fprintf(f, "消費税\n");
    fprintf(f, "IVA 16%%: %32.2f\n", venta.iva);
    fprintf(f, "合計\n");
    fprintf(f, "TOTAL: %34.2f\n", venta.total);
    fprintf(f, "==========================================\n");
    fprintf(f, "          ありがとうございました！\n");
    fprintf(f, "             Muchas gracias\n");
    fprintf(f, "==========================================\n");

    fclose(f);

    // --- MOSTRAR CUADRO DE DIÁLOGO ---
    GtkAlertDialog *dialogo = gtk_alert_dialog_new("¡Venta Finalizada!");
    char *mensaje = g_strdup_printf("El ticket se ha guardado exitosamente como:\n%s", nombre_archivo);
    gtk_alert_dialog_set_detail(dialogo, mensaje);
    
    // Mostramos el diálogo de forma asíncrona sobre la ventana principal
    gtk_alert_dialog_show(dialogo, GTK_WINDOW(parent_window));

    // Limpiar memoria
    g_free(mensaje);
    g_free(nombre_archivo);
    g_free(fecha_visual);
    g_date_time_unref(now);
    g_object_unref(dialogo);
}   

void on_btn_generar_ticket_clicked(GtkButton *btn, gpointer user_data) {
    AppData *data = (AppData *)user_data;
   
    // Verificamos que haya algo que vender
    if (g_list_model_get_n_items(G_LIST_MODEL(data->venta_actual_store)) == 0) {
        return; 
    }    

    // Obtenemos la ventana raíz para que el diálogo sepa dónde aparecer
    GtkWidget *root = GTK_WIDGET(gtk_widget_get_root(GTK_WIDGET(btn)));

    // Generamos el archivo
    generar_ticket_archivo(data, root);

    //lógica de limpieza para borrar los elementos del carrito cuando ya se generó el ticket
    //Borrar los elementos de la GListStore
    g_list_store_remove_all(data->venta_actual_store);

    //Reiniciar los valores lógicos de la venta
    venta.subtotal = 0.0;
    venta.iva = 0.0;
    venta.total = 0.0;

    GtkWindow *main_win = gtk_application_get_active_window(GTK_APPLICATION(g_application_get_default()));
    if (main_win) {
        GtkBuilder *builder_principal = g_object_get_data(G_OBJECT(main_win), "m_builder");
        if (builder_principal) {
            actualizar_resumen(builder_principal);
        }
    }
}

void cargar_inventario(GtkColumnView *cv, sqlite3 *db) {
    GListStore *store = NULL;
    GtkSelectionModel *sel_model = gtk_column_view_get_model(cv);

    // 1. Intentar recuperar el store existente para no crear uno nuevo cada vez
    if (sel_model != NULL) {
        store = G_LIST_STORE(gtk_single_selection_get_model(GTK_SINGLE_SELECTION(sel_model)));
        // LIMPIAR el store actual para que no se dupliquen los productos
        g_list_store_remove_all(store);
    } else {
        // Si es la primera vez que se carga, creamos el store y lo asignamos
        store = g_list_store_new(PRODUCTO_TYPE_OBJ);
        GtkSingleSelection *selection = gtk_single_selection_new(G_LIST_MODEL(store));
        gtk_column_view_set_model(cv, GTK_SELECTION_MODEL(selection));
        g_object_unref(selection); // El ColumnView ya tiene la referencia
    }

    // 2. Obtener los datos actualizados de la base de datos
    GList *lista_db = obtener_lista_productos_db(db);

    // 3. Meter los objetos nuevos al store
    for (GList *l = lista_db; l != NULL; l = l->next) {
        g_list_store_append(store, l->data);
    }

    // 4. Liberar la lista temporal
    g_list_free(lista_db);
}


void on_btn_inventario_agregar_clicked(GtkButton *btn, gpointer user_data) {
    AppData *data = (AppData *)user_data;
    GtkWindow *parent = GTK_WINDOW(gtk_widget_get_root(GTK_WIDGET(btn)));
	
	GtkBuilder *main_builder = g_object_get_data(G_OBJECT(parent), "m_builder");

    GtkWidget *win = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(win), "Agregar Nuevo Producto");
    gtk_window_set_transient_for(GTK_WINDOW(win), parent);
    gtk_window_set_modal(GTK_WINDOW(win), TRUE);
    gtk_window_set_resizable(GTK_WINDOW(win), FALSE);
    
    // Contenedor principal con márgenes
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 15);
    gtk_widget_set_margin_start(vbox, 20);
    gtk_widget_set_margin_end(vbox, 20);
    gtk_widget_set_margin_top(vbox, 20);
    gtk_widget_set_margin_bottom(vbox, 20);
    gtk_window_set_child(GTK_WINDOW(win), vbox);

    // Título de la ventana
    GtkWidget *lbl_titulo = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(lbl_titulo), "<b>Ingrese los datos del producto</b>");
    gtk_box_append(GTK_BOX(vbox), lbl_titulo);

    // Usamos un Grid para alinear Labels y Entrys
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_box_append(GTK_BOX(vbox), grid);

    // Preparar estructura de datos
    DialogData *d = g_new0(DialogData, 1);
    d->data = data;
    d->win_dialog = win;
    d->builder = main_builder;
    d->ent_nom = gtk_entry_new();
    d->ent_pre = gtk_entry_new();
    d->ent_stk = gtk_entry_new();

    // FILA 0: Nombre
    GtkWidget *lbl_nom = gtk_label_new("Nombre:");
    gtk_widget_set_halign(lbl_nom, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(grid), lbl_nom, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), d->ent_nom, 1, 0, 1, 1);
    gtk_widget_set_hexpand(d->ent_nom, TRUE);

    // FILA 1: Precio
    GtkWidget *lbl_pre = gtk_label_new("Precio ($):");
    gtk_widget_set_halign(lbl_pre, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(grid), lbl_pre, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), d->ent_pre, 1, 1, 1, 1);

    // FILA 2: Stock
    GtkWidget *lbl_stk = gtk_label_new("Existencia:");
    gtk_widget_set_halign(lbl_stk, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(grid), lbl_stk, 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), d->ent_stk, 1, 2, 1, 1);

    // Botón de acción
    GtkWidget *btn_save = gtk_button_new_with_label("Guardar Producto");
    gtk_widget_add_css_class(btn_save, "suggested-action");
    gtk_widget_set_margin_top(btn_save, 10);
    g_signal_connect(btn_save, "clicked", G_CALLBACK(on_guardar_nuevo_producto), d);
    
    gtk_box_append(GTK_BOX(vbox), btn_save);

    // Limpieza de memoria al cerrar
    g_signal_connect_swapped(win, "destroy", G_CALLBACK(g_free), d);

    gtk_window_present(GTK_WINDOW(win));
}

static void on_guardar_nuevo_producto(GtkButton *btn, gpointer user_data) {
    DialogData *d = (DialogData *)user_data;

    // 1. Extraer los datos de las cajas de texto (Entrys)
    const char *nombre = gtk_editable_get_text(GTK_EDITABLE(d->ent_nom));
    const char *txt_precio = gtk_editable_get_text(GTK_EDITABLE(d->ent_pre));
    const char *txt_stock = gtk_editable_get_text(GTK_EDITABLE(d->ent_stk));

    // Convertir strings a valores numéricos
    double precio = atof(txt_precio);
    int stock = atoi(txt_stock);

    // 2. Validar que al menos tenga nombre antes de procesar
    if (strlen(nombre) > 0) {
        
        // Intentar la inserción en la base de datos SQLite
        if (db_insertar_producto(d->data->db, nombre, precio, stock)) {
            
            // 3. Refrescar la tabla (ColumnView) usando el builder guardado en d
            if (d->builder && GTK_IS_BUILDER(d->builder)) {
                
                // Obtenemos el ColumnView del inventario usando su ID del archivo .ui
                GtkColumnView *cv = GTK_COLUMN_VIEW(gtk_builder_get_object(d->builder, "cv_inventario"));
                
                if (cv) {
                    g_print("Producto guardado exitosamente: %s. Refrescando tabla...\n", nombre);
                    
                    // Llamamos a la función que limpia el store y recarga desde la DB
                    cargar_inventario(cv, d->data->db);
                } else {
                    g_print("Error: No se encontró el objeto 'cv_inventario' en el builder.\n");
                }
                
            } else {
                g_print("Error: El puntero del builder en DialogData es nulo o inválido.\n");
            }

            // 4. Cerrar la ventana de diálogo de "Añadir"
            if (d->win_dialog && GTK_IS_WINDOW(d->win_dialog)) {
                gtk_window_destroy(GTK_WINDOW(d->win_dialog));
            }
        } else {
            g_print("Error: No se pudo insertar el producto en la base de datos.\n");
        }
    } else {
        g_print("Advertencia: El nombre del producto no puede estar vacío.\n");
    }
}

void on_btn_inventario_eliminar_clicked(GtkButton *btn, gpointer user_data) {
    AppData *data = (AppData *)user_data;
    GtkWindow *main_win = GTK_WINDOW(gtk_widget_get_root(GTK_WIDGET(btn)));
    
    // Aseguramos que data esté disponible en la ventana para el callback
    g_object_set_data(G_OBJECT(main_win), "app_data", data);

    // Configuramos el diálogo de alerta
    GtkAlertDialog *dialog = gtk_alert_dialog_new("¿Eliminar producto?");
    gtk_alert_dialog_set_detail(dialog, "Esta acción no se puede deshacer. ¿Estás seguro?");
    
    // El primer botón (índice 0) será el de eliminar
    const char *botones[] = {"Eliminar", "Cancelar", NULL};
    gtk_alert_dialog_set_buttons(dialog, botones);
    gtk_alert_dialog_set_cancel_button(dialog, 1); // El botón 1 (Cancelar) es el de escape
   
   //aparentemente la version de gtk tiene que ser muy nueva asi que lo comentamos ni modo
   // gtk_alert_dialog_set_destructive_button(dialog, 0); // El botón 0 se pone rojo

    // Lanzamos el diálogo
    gtk_alert_dialog_choose(dialog, main_win, NULL, on_confirmar_eliminar_finish, main_win);
    
    g_object_unref(dialog);
}



static void on_confirmar_eliminar_finish(GObject *source_object, GAsyncResult *res, gpointer user_data) {
    GtkAlertDialog *dialog = GTK_ALERT_DIALOG(source_object);
    GtkWindow *parent = GTK_WINDOW(user_data); // Pasamos la ventana principal como user_data
    int response = gtk_alert_dialog_choose_finish(dialog, res, NULL);

    //los botones se cuentan por el índice (0 es el primero que agregamos)
    if (response == 0) {
        // Recuperamos los datos necesarios para borrar
        GtkBuilder *builder = g_object_get_data(G_OBJECT(parent), "m_builder");
        AppData *data = g_object_get_data(G_OBJECT(parent), "app_data");
        GtkColumnView *cv = GTK_COLUMN_VIEW(gtk_builder_get_object(builder, "cv_inventario"));

        GtkSelectionModel *model = gtk_column_view_get_model(cv);
        GtkBitset *selection = gtk_selection_model_get_selection(model);

        if (!gtk_bitset_is_empty(selection)) {
            guint pos = gtk_bitset_get_nth(selection, 0);
            ProductoObj *p = g_list_model_get_item(G_LIST_MODEL(model), pos);

            if (db_eliminar_producto(data->db, producto_obj_get_id(p))) {
                cargar_inventario(cv, data->db);
            }
            g_object_unref(p);
        }
        gtk_bitset_unref(selection);
    }
}
