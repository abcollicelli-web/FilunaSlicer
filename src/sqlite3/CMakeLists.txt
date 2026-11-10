# Filuna v1 — SQLite official public-domain amalgamation.
# Source: https://www.sqlite.org/2025/sqlite-amalgamation-3500400.zip
# Version: 3.50.4 (source archive SHA-256 in FILUNA/guia-v1/VERIFICACOES.md)
add_library(filuna_sqlite3 STATIC sqlite3.c)
target_include_directories(filuna_sqlite3 PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_definitions(filuna_sqlite3 PRIVATE SQLITE_THREADSAFE=1 SQLITE_OMIT_LOAD_EXTENSION=1)
