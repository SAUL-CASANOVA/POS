#include "producto.h"

//implementacion del modelo y factory (setup y bind)

struct _ProductoObj {
    GObject parent_instance;
    int id;
    char *nombre;
    double precio;
    int existencia;
};

G_DEFINE_TYPE(ProductoObj, producto_obj, G_TYPE_OBJECT)

// Finalize: Se encarga de liberar la memoria cuando el objeto se destruye
static void producto_obj_finalize(GObject *object) {
    ProductoObj *self = PRODUCTO_OBJ(object);
    g_free(self->nombre); // Liberar el string dinámico
    G_OBJECT_CLASS(producto_obj_parent_class)->finalize(object);
}

static void producto_obj_init(ProductoObj *self) {
    // Inicialización por defecto (para valores por defecto, a manera de constructor no se usará mas allá aquí)
}

static void producto_obj_class_init(ProductoObjClass *klass) {
    G_OBJECT_CLASS(klass)->finalize = producto_obj_finalize;
}

// Implementación del Constructor
ProductoObj* producto_obj_new(int id, const char *nombre, double precio, int existencia) {
    ProductoObj *self = g_object_new(PRODUCTO_TYPE_OBJ, NULL);
    self->id = id;
    self->nombre = g_strdup(nombre); // Duplicamos el string para que el objeto sea dueño de su memoria
    self->precio = precio;
    self->existencia = existencia;
    return self;
}

// Implementación de Getters
int producto_obj_get_id(ProductoObj *self) { return self->id; }
const char* producto_obj_get_nombre(ProductoObj *self) { return self->nombre; }
double producto_obj_get_precio(ProductoObj *self) { return self->precio; }
int producto_obj_get_existencia(ProductoObj *self) { return self->existencia; }

// --- CALLBACKS DEL FACTORY (ESTÁTICOS) ---

static void setup_id_cb(GtkSignalListItemFactory *factory, GtkListItem *list_item) {
    gtk_list_item_set_child(list_item, gtk_label_new(NULL));
}
static void bind_id_cb(GtkSignalListItemFactory *factory, GtkListItem *list_item) {
    ProductoObj *p = PRODUCTO_OBJ(gtk_list_item_get_item(list_item));
    char *txt = g_strdup_printf("%d", p->id);
    gtk_label_set_label(GTK_LABEL(gtk_list_item_get_child(list_item)), txt);
    g_free(txt);
}

static void setup_nombre_cb(GtkSignalListItemFactory *factory, GtkListItem *list_item) {
    GtkWidget *label = gtk_label_new(NULL);
    gtk_label_set_xalign(GTK_LABEL(label), 0.0);
    gtk_list_item_set_child(list_item, label);
}
static void bind_nombre_cb(GtkSignalListItemFactory *factory, GtkListItem *list_item) {
    ProductoObj *p = PRODUCTO_OBJ(gtk_list_item_get_item(list_item));
    gtk_label_set_label(GTK_LABEL(gtk_list_item_get_child(list_item)), p->nombre);
}

static void setup_precio_cb(GtkSignalListItemFactory *factory, GtkListItem *list_item) {
    GtkWidget *label = gtk_label_new(NULL);
    gtk_label_set_xalign(GTK_LABEL(label), 1.0);
    gtk_list_item_set_child(list_item, label);
}
static void bind_precio_cb(GtkSignalListItemFactory *factory, GtkListItem *list_item) {
    ProductoObj *p = PRODUCTO_OBJ(gtk_list_item_get_item(list_item));
    char *txt = g_strdup_printf("$%.2f", p->precio);
    gtk_label_set_label(GTK_LABEL(gtk_list_item_get_child(list_item)), txt);
    g_free(txt);
}

static void setup_existencia_cb(GtkSignalListItemFactory *factory, GtkListItem *list_item) {
    gtk_list_item_set_child(list_item, gtk_label_new(NULL));
}
static void bind_existencia_cb(GtkSignalListItemFactory *factory, GtkListItem *list_item) {
    ProductoObj *p = PRODUCTO_OBJ(gtk_list_item_get_item(list_item));
    char *txt = g_strdup_printf("%d", p->existencia);
    gtk_label_set_label(GTK_LABEL(gtk_list_item_get_child(list_item)), txt);
    g_free(txt);
}

// --- FUNCIÓN PÚBLICA DE CONFIGURACIÓN ---

static void crear_factory(GtkColumnViewColumn *col, GCallback setup, GCallback bind) {
    if (!col) return;
    GtkListItemFactory *factory = gtk_signal_list_item_factory_new();
    g_signal_connect(factory, "setup", setup, NULL);
    g_signal_connect(factory, "bind", bind, NULL);
    gtk_column_view_column_set_factory(col, factory);
    g_object_unref(factory);
}

void producto_configurar_columnas(GtkBuilder *builder) {
    crear_factory(GTK_COLUMN_VIEW_COLUMN(gtk_builder_get_object(builder, "col_id")), 
                  G_CALLBACK(setup_id_cb), G_CALLBACK(bind_id_cb));
    crear_factory(GTK_COLUMN_VIEW_COLUMN(gtk_builder_get_object(builder, "col_nombre")), 
                  G_CALLBACK(setup_nombre_cb), G_CALLBACK(bind_nombre_cb));
    crear_factory(GTK_COLUMN_VIEW_COLUMN(gtk_builder_get_object(builder, "col_precio")), 
                  G_CALLBACK(setup_precio_cb), G_CALLBACK(bind_precio_cb));
    crear_factory(GTK_COLUMN_VIEW_COLUMN(gtk_builder_get_object(builder, "col_existencia")), 
                  G_CALLBACK(setup_existencia_cb), G_CALLBACK(bind_existencia_cb));
}
