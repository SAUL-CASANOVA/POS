#ifndef PRODUCTO_H
#define PRODUCTO_H

#include <gtk/gtk.h>

/* --- Este archivo define la estructura y las funciones públicas para crear y destruir objetos */

// Definición de tipos y macros estándar de GObject
#define PRODUCTO_TYPE_OBJ (producto_obj_get_type())
G_DECLARE_FINAL_TYPE(ProductoObj, producto_obj, PRODUCTO, OBJ, GObject)

// Constructor: Crea una nueva instancia con los datos
ProductoObj* producto_obj_new(int id, const char *nombre, double precio, int existencia);

//hará uso de factory para configurar
void producto_configurar_columnas(GtkBuilder *builder);

// Getters (para el Factory del ColumnView)
int         producto_obj_get_id(ProductoObj *self);
const char* producto_obj_get_nombre(ProductoObj *self);
double      producto_obj_get_precio(ProductoObj *self);
int         producto_obj_get_existencia(ProductoObj *self);

// Prototipo
void producto_obj_set_cantidad(ProductoObj *self, int cantidad);
int producto_obj_get_cantidad(ProductoObj *self);
void on_ventas_id_bind(GtkSignalListItemFactory *factory, GtkListItem *list_item);
void on_ventas_nombre_bind(GtkSignalListItemFactory *factory, GtkListItem *list_item);
void on_ventas_cant_bind(GtkSignalListItemFactory *factory, GtkListItem *list_item);
void on_ventas_precio_bind(GtkSignalListItemFactory *factory, GtkListItem *list_item);
void on_ventas_iva_bind(GtkSignalListItemFactory *factory, GtkListItem *list_item);
void on_ventas_eliminar_setup(GtkSignalListItemFactory *factory, GtkListItem *list_item);
void on_ventas_eliminar_bind(GtkSignalListItemFactory *factory, GtkListItem *list_item);
void configurar_columnas_venta(GtkBuilder *builder, GListStore *store);
void on_setup_label(GtkSignalListItemFactory *factory, GtkListItem *list_item);
#endif
