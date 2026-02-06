/*
 * text_formatter.cpp
 *
 * This file is part of Knights.
 *
 * Copyright (C) Stephen Thompson, 2006 - 2025.
 * Copyright (C) Kalle Marjola, 1994.
 *
 * Knights is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * Knights is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Knights.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "my_ctype.hpp"
#include "text_formatter.hpp"

#include "external/utf8.h"

TextFormatter::TextFormatter(Printer &p, int width, bool rich)
    : printer(p), max_line_width(width), is_rich(rich), do_centre(false), y(0)
{ }

int TextFormatter::printString(const Coercri::UTF8String &text_utf8)
{
    Coercri::UTF8String line;
    std::string::size_type fit_bytes = 0;
    const std::string &text = text_utf8.asUTF8();
    std::string::const_iterator it = text.begin();

    while (1) {
        int cp = 0;
        bool at_newline = false;
        if (it == text.end()) {
            if (!line.empty()) {
                // We've reached the end of the input but there is still stuff in the buffer.
                // Manufacture an extra newline character, this should force it to print.
                at_newline = true;
            } else {
                // We've printed everything -- exit.
                break;
            }
        } else if (*it == '%' && is_rich) {
            // Intercept % as "rich text" marker
            ++it;
            // Read next code point after the "%"
            if (*it == '%') {
                // A normal % sign...
                cp = '%';
                ++it;
            } else if (*it == 'c') {
                // Directive to centre text
                do_centre = true;
                ++it;
            } else if (*it == 'l') {
                // Directive to left-justify text
                do_centre = false;
                ++it;
            } else {
                // %-anything-else is an error, but we just skip over the % in this
                // case and continue processing from the next byte.
            }
        } else if (*it == '\n') {
            at_newline = true;
            ++it;
        } else {
            // Normal character -- fetch it in, and increase the iterator
            // Note: utf8::next should never throw here because UTF8String's invariant is that
            // the string is always UTF-8.
            cp = utf8::next(it, text.end());
        }
        
        // If we got a code-point then append it to our line
        std::string::size_type old_line_length = line.asUTF8().length();
        if (cp != 0) line += UTF8String::fromCodePoint(cp);

        // Calculate the new line length
        const int line_width = printer.getTextWidth(line);

        // Check to see if we are at a possible line break point.
        // (This means a non-space character where the next character IS a space.)
        if (!IsAsciiSpace(cp) && (it == text.end() || IsAsciiSpace(*it))) {
            if (line_width <= max_line_width) {
                // OK, approve this point as a possible line break position.
                fit_bytes = line.asUTF8().length();
            }
        }

        // If the line has got too big (or we have reached an explicit
        // newline) then we should dump everything up to the previous line
        // break point, then start a new line.
        if (line_width > max_line_width || at_newline) {

            // Special case: if fit_bytes == 0 (&& !line.empty()) then
            // reset fit_bytes to old_line_length. This is the case
            // where the entire line is too big to print, but there
            // were no possible line break points.
            if (fit_bytes == 0 && !line.empty()) {
                fit_bytes = old_line_length;
            }

            // Print out the line (up to the last approved break point)
            const std::string to_print = line.asUTF8().substr(0, fit_bytes);
            printer.printLine(Coercri::UTF8String::fromUTF8Safe(to_print), y, do_centre);
            y += printer.getTextHeight();

            // Reset for the next line (skip any spaces)
            while (fit_bytes < line.asUTF8().length() && IsAsciiSpace(line.asUTF8()[fit_bytes])) {
                ++fit_bytes;
            }
            line = Coercri::UTF8String::fromUTF8Safe(line.asUTF8().substr(fit_bytes, std::string::npos));
            fit_bytes = 0;
        }
    }

    return y;
}
