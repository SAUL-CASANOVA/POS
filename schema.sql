-- 1. Habilitar el soporte de llaves foráneas en SQLite
PRAGMA foreign_keys = ON;

-- 2. Tabla de Catálogo de Productos
CREATE TABLE IF NOT EXISTS productos (
    id          INTEGER PRIMARY KEY AUTOINCREMENT,
    nombre      TEXT NOT NULL,
    precio      REAL NOT NULL CHECK(precio >= 0),
    existencia  INTEGER NOT NULL DEFAULT 0 CHECK(existencia >= 0)
);

-- 3. Tabla de Ventas (Historial de transacciones)
-- Cada vez que se haga una venta en el archivo .c, se guarda una fila aquí.
CREATE TABLE IF NOT EXISTS ventas (
    id               INTEGER PRIMARY KEY AUTOINCREMENT,
    id_producto      INTEGER NOT NULL,
    cantidad_vendida INTEGER NOT NULL CHECK(cantidad_vendida > 0),
    total_venta      REAL NOT NULL CHECK(total_venta >= 0),
    fecha_hora       DATETIME DEFAULT (datetime('now', 'localtime')),
    
    -- Esta línea une la venta con un producto real
    FOREIGN KEY (id_producto) REFERENCES productos(id) 
        ON DELETE CASCADE
);

-- 4. Índices para que las búsquedas sean rápidas en el POS
CREATE INDEX IF NOT EXISTS idx_nombre_prod ON productos(nombre);
