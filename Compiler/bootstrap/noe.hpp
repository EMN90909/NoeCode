#pragma once
// Bootstrap-only compatibility include. Public code should include <noqeri.hpp>.
// NoqeriDB is deliberately compiled against its narrow public interface so its
// compact .nqd lexer/parser names cannot collide with the language frontend.
#ifdef NOQERI_DATABASE_SOURCE
#include "../../Include/noqeri/database.hpp"
#else
#include "../../Include/noqeri/noqeri.hpp"
#include "../../Include/noqeri/generics.hpp"
#include "../../Include/noqeri/register_alloc.hpp"
#include "../../Include/noqeri/workspace.hpp"
#include "../../Include/noqeri/web.hpp"
#endif
