#ifndef PRODUCTO_H
#define PRODUCTO_H

#include <gtk/gtk.h>

/* --- Este archivo define la estructura y las funciones públicas para crear y destruir objetos */

// Definición de tipos y macros estándar de GObject
#define PRODUCTO_TYPE_OBJ (producto_obj_get_type())
G_DECLARE_FINAL_TYPE(ProductoObj, producto_obj, PRODUCTO, OBJ, GObject)

// Constructor: Crea una nueva instancia con los datos
ProductoObj* producto_obj_new(int id, const char *nombre, double precio, int existencia);

// Getters (para el Factory del ColumnView)
int         producto_obj_get_id(ProductoObj *self);
const char* producto_obj_get_nombre(ProductoObj *self);
double      producto_obj_get_precio(ProductoObj *self);
int         producto_obj_get_existencia(ProductoObj *self);

#endif
