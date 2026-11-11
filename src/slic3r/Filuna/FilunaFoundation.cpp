#include "FilunaFoundation.hpp"

#include <sqlite3.h>

namespace Slic3r::Filuna {
int sqlite_version_number() noexcept
{
    return sqlite3_libversion_number();
}
}
