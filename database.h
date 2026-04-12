#ifndef DATABASE_H
#define DATABASE_H

#include <sqlite3.h>
#include <stdbool.h>
#include <gtk/gtk.h> 

/* --- CONFIGURACIÓN INICIAL --- */

// Abre la conexión y crea las tablas usando el archivo schema.sql
sqlite3* inicializar_db(const char* nombre_db);

// Cierra la base de datos de forma limpia
void cerrar_db(sqlite3* db);


/* --- GESTIÓN DE PRODUCTOS (CATÁLOGO) --- */

// Agrega un producto nuevo al inventario
bool db_insertar_producto(sqlite3* db, const char* nombre, double precio, int existencia);

// Actualiza el stock de un producto (Usada en inventario)
bool db_actualizar_stock(sqlite3* db, int id_producto, int nueva_existencia);

// Elimina un producto por ID
bool db_eliminar_producto(sqlite3* db, int id_producto);

// Búsqueda 
void db_consultar_producto(sqlite3* db, int id_producto);


/* --- GESTIÓN DE VENTAS (TRANSACCIONES) --- */

// Registra una venta:
// Inserta el movimiento en la tabla 'ventas'
// Resta la cantidad vendida de la tabla 'productos'
bool db_registrar_venta(sqlite3* db, int id_producto, int cantidad, double total);

// Devuelve una lista de las ventas realizadas 
void db_listar_ventas(sqlite3* db);

#endif
