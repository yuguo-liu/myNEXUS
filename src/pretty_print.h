#ifndef PRETTY_PRINT_H
#define PRETTY_PRINT_H

#include <iostream>
#include <cstdio>
#include <cstdarg>
#include <string>

// --- ANSI Color Code Definitions ---
// Format: \033[...m is the ANSI escape sequence format
// 30-37 are standard foreground colors
#define ANSI_COLOR_RESET    "\033[0m"
#define ANSI_COLOR_RED      "\033[1;31m"    // Bright Red (Err)
#define ANSI_COLOR_GREEN    "\033[1;32m"    // Bright Green (Ok)
#define ANSI_COLOR_YELLOW   "\033[1;33m"    // Bright Yellow (Info)
#define ANSI_COLOR_BLUE     "\033[1;34m"    // Bright Blue (Debug)
// ANSI_COLOR_WHITE is implicitly handled by ANSI_COLOR_RESET 
// before the message part, ensuring the content is the default color.

// --- Core Print Function ---

/**
 * @brief Core implementation for the formatted pretty print function.
 *
 * @param color ANSI color code used for the type name.
 * @param type_name The type name to print (e.g., "Info", "Debug").
 * @param format The format string (same as printf).
 * @param ... Format arguments.
 */
void pretty_print_core(const char* color, const char* type_name, const char* format, ...);

// --- Exported User Interface Macros ---

/**
 * @brief Prints an Info message (Yellow).
 * @param format Format string.
 * @param ... Format arguments.
 */
#define INFO_PRINT(format, ...) \
    pretty_print_core(ANSI_COLOR_YELLOW, "Info", format, ##__VA_ARGS__)

/**
 * @brief Prints a Debug message (Blue).
 * @param format Format string.
 * @param ... Format arguments.
 */
#define DEBUG_PRINT(format, ...) \
    pretty_print_core(ANSI_COLOR_BLUE, "Debug", format, ##__VA_ARGS__)

/**
 * @brief Prints an Ok message (Green).
 * @param format Format string.
 * @param ... Format arguments.
 */
#define OK_PRINT(format, ...) \
    pretty_print_core(ANSI_COLOR_GREEN, "Ok", format, ##__VA_ARGS__)

/**
 * @brief Prints an Err message (Red).
 * @param format Format string.
 * @param ... Format arguments.
 */
#define ERR_PRINT(format, ...) \
    pretty_print_core(ANSI_COLOR_RED, "Err", format, ##__VA_ARGS__)

#endif // PRETTY_PRINT_H