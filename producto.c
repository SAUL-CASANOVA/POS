#include "producto.h"

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
