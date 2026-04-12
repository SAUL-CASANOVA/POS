#include "database.h"
#include "producto.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Función privada para leer cualquier archivo de texto (se usará para leer el schema.sql con las tablas)
static char* leer_script_sql(const char* ruta) {
    FILE *f = fopen(ruta, "rb");
    if (f == NULL) {
        fprintf(stderr, "Error: No se pudo abrir el archivo %s\n", ruta);
        return NULL;
    }

    // Ir al final para saber el tamaño
    fseek(f, 0, SEEK_END);
    long tamano = ftell(f);
    fseek(f, 0, SEEK_SET);

    // Reservar memoria
    char *contenido = malloc(tamano + 1);
    if (contenido) {
        fread(contenido, tamano, 1, f);
        contenido[tamano] = '\0'; // Terminador de cadena para C
    }
    
    fclose(f);
    return contenido;
}

sqlite3* inicializar_db(const char* nombre_db) {
    sqlite3 *db;
    int rc = sqlite3_open(nombre_db, &db);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "No se pudo abrir la base de datos: %s\n", sqlite3_errmsg(db));
        return NULL;
    }

    // Lectura del archivo schema.sql que tiene las tablas
    char *script = leer_script_sql("schema.sql");
    
    if (script != NULL) {
        char *error_msg = 0;
        // sqlite3_exec puede ejecutar múltiples instrucciones separadas por ';'
        rc = sqlite3_exec(db, script, 0, 0, &error_msg);
        
        if (rc != SQLITE_OK) {
            fprintf(stderr, "Error al ejecutar el schema: %s\n", error_msg);
            sqlite3_free(error_msg);
        } else {
            printf("Estructura cargada exitosamente desde schema.sql\n");
        }
        free(script); // Liberamos la memoria del texto leído
    }

    return db;
}

void cerrar_db(sqlite3* db) {
    if (db) {
        sqlite3_close(db);
        printf("Base de datos cerrada.\n");
    }
}

/* --- GESTIÓN DE PRODUCTOS --- */

bool db_insertar_producto(sqlite3* db, const char* nombre, double precio, int existencia) {
    char *err_msg = 0;
    // %q es un formato especial de SQLite que escapa comillas automáticamente
    char *sql = sqlite3_mprintf(
        "INSERT INTO productos (nombre, precio, existencia) VALUES ('%q', %f, %d);",
        nombre, precio, existencia
    );

    int rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
    sqlite3_free(sql); //liberar la memoria de mprintf

    if (rc != SQLITE_OK) {
        fprintf(stderr, "Error al insertar producto: %s\n", err_msg);
        sqlite3_free(err_msg);
        return false;
    }
    return true;
}

/* --- GESTIÓN DE VENTAS (CON TRANSACCIONES) --- */

bool db_registrar_venta(sqlite3* db, int id_producto, int cantidad, double total) {
    char *err_msg = 0;
    
    // 1. Iniciamos transacción
    sqlite3_exec(db, "BEGIN TRANSACTION;", 0, 0, 0);

    // Registro en la tabla de ventas
    char *sql_venta = sqlite3_mprintf(
        "INSERT INTO ventas (id_producto, cantidad_vendida, total_venta) VALUES (%d, %d, %f);",
        id_producto, cantidad, total
    );
    
    int rc = sqlite3_exec(db, sql_venta, 0, 0, &err_msg);
    sqlite3_free(sql_venta);

    if (rc != SQLITE_OK) {
        sqlite3_exec(db, "ROLLBACK;", 0, 0, 0); // Cancela si algo falla
        fprintf(stderr, "Error en registro de venta: %s\n", err_msg);
        sqlite3_free(err_msg);
        return false;
    }

    // Actualización del stock en la tabla productos
    char *sql_stock = sqlite3_mprintf(
        "UPDATE productos SET existencia = existencia - %d WHERE id = %d;",
        cantidad, id_producto
    );

    rc = sqlite3_exec(db, sql_stock, 0, 0, &err_msg);
    sqlite3_free(sql_stock);

    if (rc != SQLITE_OK) {
        sqlite3_exec(db, "ROLLBACK;", 0, 0, 0);
        fprintf(stderr, "Error al actualizar stock: %s\n", err_msg);
        sqlite3_free(err_msg);
        return false;
    }

    // 2. Finalizamos transacción (Guardado permanente)
    sqlite3_exec(db, "COMMIT;", 0, 0, 0);
    return true;
}

// Implementación de la búsqueda (ejemplo base)
void db_consultar_producto(sqlite3* db, int id_producto) {
    // Aquí se implementa el select
}

/* --- GESTIÓN DE PRODUCTOS (CRUD INVENTARIO) --- */

// Función para actualizar SOLO el stock de un producto (CRUD: Update)
bool db_actualizar_stock(sqlite3* db, int id_producto, int nueva_existencia) {
    char *err_msg = 0;
    // Preparamos la sentencia SQL formateada
    char *sql = sqlite3_mprintf(
        "UPDATE productos SET existencia = %d WHERE id = %d;",
        nueva_existencia, id_producto
    );

    int rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
    sqlite3_free(sql); // Liberar memoria de mprintf

    if (rc != SQLITE_OK) {
        fprintf(stderr, "Error al actualizar stock del producto ID %d: %s\n", id_producto, err_msg);
        sqlite3_free(err_msg);
        return false;
    }
    g_print("Stock del producto ID %d actualizado exitosamente.\n", id_producto);
    return true;
}

// Función para eliminar un producto (CRUD: Delete)
bool db_eliminar_producto(sqlite3* db, int id_producto) {
    char *err_msg = 0;
    // Usamos el ID como Primary Key para el DELETE
    char *sql = sqlite3_mprintf("DELETE FROM productos WHERE id = %d;", id_producto);

    int rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
    sqlite3_free(sql);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "Error al eliminar el producto ID %d: %s\n", id_producto, err_msg);
        sqlite3_free(err_msg);
        return false;
    }
    g_print("Producto ID %d eliminado exitosamente.\n", id_producto);
    return true;
}
