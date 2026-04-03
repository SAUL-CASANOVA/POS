#include <gtk/gtk.h>

//activar ventana principal
static void activate(GtkApplication *app, gpointer user_data);

//devuelve string con la hora
char *obtener_hora_actual();

//actualiza el label de la hora
static gboolean actualizar_reloj(gpointer user_data);

//funcion para aplicar estilo de css al programa
static void estilos();

int main(int argc, char **argv){
	GtkApplication *app;
	int status;

	app = gtk_application_new("besto.team.struct", G_APPLICATION_DEFAULT_FLAGS);
	g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);

	//se utiliza al final. deja a la ventana en bucle y espera al usuario
	//no escribir código despues de estas líneas. 
	status = g_application_run(G_APPLICATION(app), argc, argv);
	g_object_unref(app);
	return status;

}




static void activate(GtkApplication *app, gpointer user_data){
	//cargar estilos css con funcion personalizada
	estilos();

	GtkBuilder *builder; //builder que ayuda a obtener widgets del xml
	GtkWidget *window; //ventana principal
	GtkWidget *reloj; //label de fecha y hora

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
