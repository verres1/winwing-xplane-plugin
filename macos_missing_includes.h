/* 
 * Wrapper header to fix missing standard library includes in source files
 * This is a workaround for strict libc++ on macOS/Clang that doesn't 
 * transitively include headers like libstdc++ does
 */

#ifndef MACOS_MISSING_INCLUDES_H
#define MACOS_MISSING_INCLUDES_H

#include <sstream>
#include <vector>

#endif /* MACOS_MISSING_INCLUDES_H */

