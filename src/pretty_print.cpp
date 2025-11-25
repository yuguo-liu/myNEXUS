#include "pretty_print.h"

void pretty_print_core(const char* color, const char* type_name, const char* format, ...) {
    // 1. Print the colored type name part
    // Format: [TYPE]
    std::cout << color << "[" << type_name << "]" << ANSI_COLOR_RESET << " ";

    // 2. Print the formatted content part (using vprintf for formatting)
    // The ANSI_COLOR_RESET above ensures the content is the default/white color.
    std::va_list args;
    va_start(args, format);
    
    // vprintf writes the formatted output to stdout
    std::vprintf(format, args);
    va_end(args);

    // 3. Ensure a newline is added at the end
    std::cout << std::endl;
    // 4. Flush the buffer immediately to ensure timely display
    std::cout.flush();
}
