#include "producto.h"

// Implementación del modelo ProductoObj
struct _ProductoObj {
    GObject parent_instance;
    int id;
    char *nombre;
    double precio;
    int existencia;
    int cantidad;
};

G_DEFINE_TYPE(ProductoObj, producto_obj, G_TYPE_OBJECT)

static void producto_obj_finalize(GObject *object) {
    ProductoObj *self = PRODUCTO_OBJ(object);
    g_free(self->nombre);
    G_OBJECT_CLASS(producto_obj_parent_class)->finalize(object);
}

static void producto_obj_init(ProductoObj *self) {}

static void producto_obj_class_init(ProductoObjClass *klass) {
    G_OBJECT_CLASS(klass)->finalize = producto_obj_finalize;
}

// Constructor y Getters/Setters
ProductoObj* producto_obj_new(int id, const char *nombre, double precio, int existencia) {
    ProductoObj *self = g_object_new(PRODUCTO_TYPE_OBJ, NULL);
    self->id = id;
    self->nombre = g_strdup(nombre);
    self->precio = precio;
    self->existencia = existencia;
    self->cantidad = 0;
    return self;
}

int producto_obj_get_id(ProductoObj *self) { return self->id; }
const char* producto_obj_get_nombre(ProductoObj *self) { return self->nombre; }
double producto_obj_get_precio(ProductoObj *self) { return self->precio; }
int producto_obj_get_existencia(ProductoObj *self) { return self->existencia; }

void producto_obj_set_cantidad(ProductoObj *self, int cantidad) {
    g_return_if_fail(PRODUCTO_IS_OBJ(self));
    self->cantidad = cantidad;
}

int producto_obj_get_cantidad(ProductoObj *self) {
    g_return_val_if_fail(PRODUCTO_IS_OBJ(self), 0);
    return self->cantidad;
}

// --- FUNCIONES DE APOYO (DEBEN IR ARRIBA) ---

void on_setup_label(GtkSignalListItemFactory *factory, GtkListItem *list_item) {
    GtkWidget *label = gtk_label_new(NULL);
    gtk_list_item_set_child(list_item, label);
}

static void on_btn_eliminar_clicked(GtkButton *btn, gpointer user_data) {
    GListStore *store = G_LIST_STORE(g_object_get_data(G_OBJECT(btn), "mi_store"));
    ProductoObj *producto = PRODUCTO_OBJ(g_object_get_data(G_OBJECT(btn), "mi_producto"));

    if (store && producto) {
        guint posicion;
        if (g_list_store_find(store, producto, &posicion)) {
            g_list_store_remove(store, posicion);
        }
    }
}

// --- CALLBACKS PARA FACTORIES (VENTANA BUSCADOR) ---

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

static void crear_factory(GtkColumnViewColumn *col, GCallback setup, GCallback bind) {
    if (!col) return;
    GtkListItemFactory *factory = gtk_signal_list_item_factory_new();
    g_signal_connect(factory, "setup", setup, NULL);
    g_signal_connect(factory, "bind", bind, NULL);
    gtk_column_view_column_set_factory(col, factory);
    g_object_unref(factory);
}

void producto_configurar_columnas(GtkBuilder *builder) {
    crear_factory(GTK_COLUMN_VIEW_COLUMN(gtk_builder_get_object(builder, "col_id")), G_CALLBACK(setup_id_cb), G_CALLBACK(bind_id_cb));
    crear_factory(GTK_COLUMN_VIEW_COLUMN(gtk_builder_get_object(builder, "col_nombre")), G_CALLBACK(setup_nombre_cb), G_CALLBACK(bind_nombre_cb));
    crear_factory(GTK_COLUMN_VIEW_COLUMN(gtk_builder_get_object(builder, "col_precio")), G_CALLBACK(setup_precio_cb), G_CALLBACK(bind_precio_cb));
    crear_factory(GTK_COLUMN_VIEW_COLUMN(gtk_builder_get_object(builder, "col_existencia")), G_CALLBACK(setup_existencia_cb), G_CALLBACK(bind_existencia_cb));
}

// --- CALLBACKS PARA FACTORIES (VENTANA VENTAS PRINCIPAL) ---

void on_ventas_id_bind(GtkSignalListItemFactory *factory, GtkListItem *list_item) {
    ProductoObj *p = PRODUCTO_OBJ(gtk_list_item_get_item(list_item));
    GtkWidget *child = gtk_list_item_get_child(list_item);
    if (GTK_IS_LABEL(child)) {
        char *texto = g_strdup_printf("%d", p->id);
        gtk_label_set_text(GTK_LABEL(child), texto);
        g_free(texto);
    }
}

void on_ventas_nombre_bind(GtkSignalListItemFactory *factory, GtkListItem *list_item) {
    ProductoObj *p = PRODUCTO_OBJ(gtk_list_item_get_item(list_item));
    GtkWidget *child = gtk_list_item_get_child(list_item);
    if (GTK_IS_LABEL(child)) {
        gtk_label_set_text(GTK_LABEL(child), p->nombre);
    }
}

void on_ventas_cant_bind(GtkSignalListItemFactory *factory, GtkListItem *list_item) {
    ProductoObj *p = PRODUCTO_OBJ(gtk_list_item_get_item(list_item));
    GtkWidget *child = gtk_list_item_get_child(list_item);
    if (GTK_IS_LABEL(child)) {
        char *texto = g_strdup_printf("%d", p->cantidad);
        gtk_label_set_text(GTK_LABEL(child), texto);
        g_free(texto);
    }
}

void on_ventas_precio_bind(GtkSignalListItemFactory *factory, GtkListItem *list_item) {
    ProductoObj *p = PRODUCTO_OBJ(gtk_list_item_get_item(list_item));
    GtkWidget *child = gtk_list_item_get_child(list_item);
    if (GTK_IS_LABEL(child)) {
        char *texto = g_strdup_printf("$%.2f", p->precio);
        gtk_label_set_text(GTK_LABEL(child), texto);
        g_free(texto);
    }
}

void on_ventas_iva_bind(GtkSignalListItemFactory *factory, GtkListItem *list_item) {
    ProductoObj *p = PRODUCTO_OBJ(gtk_list_item_get_item(list_item));
    GtkWidget *child = gtk_list_item_get_child(list_item);
    if (GTK_IS_LABEL(child)) {
        double iva = (p->precio * p->cantidad) * 0.16;
        char *texto = g_strdup_printf("$%.2f", iva);
        gtk_label_set_text(GTK_LABEL(child), texto);
        g_free(texto);
    }
}

void on_ventas_eliminar_setup(GtkSignalListItemFactory *factory, GtkListItem *list_item) {
    GtkWidget *button = gtk_button_new_from_icon_name("edit-delete-symbolic");
    gtk_widget_add_css_class(button, "destructive-action");
    gtk_list_item_set_child(list_item, button);
}

void on_ventas_eliminar_bind(GtkSignalListItemFactory *factory, GtkListItem *list_item) {
    GtkWidget *button = gtk_list_item_get_child(list_item);
    ProductoObj *p = PRODUCTO_OBJ(gtk_list_item_get_item(list_item));
    GListStore *store = G_LIST_STORE(g_object_get_data(G_OBJECT(factory), "mi_store"));

    if (GTK_IS_BUTTON(button)) {
        g_object_set_data(G_OBJECT(button), "mi_producto", p);
        g_object_set_data(G_OBJECT(button), "mi_store", store);

        g_signal_handlers_disconnect_by_func(button, G_CALLBACK(on_btn_eliminar_clicked), NULL);
        g_signal_connect(button, "clicked", G_CALLBACK(on_btn_eliminar_clicked), NULL);
    }
}

void configurar_columnas_venta(GtkBuilder *builder, GListStore *venta_store) {
    struct {
        char *id;
        void (*bind)(GtkSignalListItemFactory*, GtkListItem*);
        void (*setup)(GtkSignalListItemFactory*, GtkListItem*);
    } columnas[] = {
        {"id_column_view",        on_ventas_id_bind,       NULL},
        {"producto_column_view",  on_ventas_nombre_bind,   NULL},
        {"cant_column_view",      on_ventas_cant_bind,     NULL},
        {"precio_column_view",    on_ventas_precio_bind,   NULL},
        {"iva_column_view",       on_ventas_iva_bind,      NULL},
        {"descuento_column_view", on_ventas_precio_bind,   NULL},
        {"eliminar_column_view",  on_ventas_eliminar_bind, on_ventas_eliminar_setup}
    };

    for (int i = 0; i < 7; i++) {
        GtkColumnViewColumn *col = GTK_COLUMN_VIEW_COLUMN(gtk_builder_get_object(builder, columnas[i].id));
        if (col) {
	       	GtkListItemFactory *factory = gtk_signal_list_item_factory_new();
            if (g_strcmp0(columnas[i].id, "eliminar_column_view") == 0) {
                g_object_set_data(G_OBJECT(factory), "mi_store", venta_store);
            }
            g_signal_connect(factory, "setup", G_CALLBACK(columnas[i].setup ? columnas[i].setup : on_setup_label), NULL);
            if (columnas[i].bind) g_signal_connect(factory, "bind", G_CALLBACK(columnas[i].bind), NULL);
            gtk_column_view_column_set_factory(col, factory);
            g_object_unref(factory);
        }
    }
}
