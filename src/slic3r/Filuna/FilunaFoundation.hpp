#pragma once

namespace Slic3r::Filuna {
// Foundation check; all persisted data will be managed by FilunaDb in phase 3.
int sqlite_version_number() noexcept;
}
