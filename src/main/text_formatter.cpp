/*
 * text_formatter.cpp
 *
 * This file is part of Knights.
 *
 * Copyright (C) Stephen Thompson, 2006 - 2012.
 * Copyright (C) Kalle Marjola, 1994.
 *
 * Knights is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
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

TextFormatter::TextFormatter(Printer &p, int width, bool rich)
    : printer(p), max_line_width(width), is_rich(rich), do_centre(false), y(0)
{ }

int TextFormatter::printString(const std::string &text)
{
    std::string line;
    std::string::size_type fit_chars = 0;
    std::string::const_iterator it = text.begin();

    while (1) {
        char c = 0;
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
            if (*it == '%') {
                // A normal % sign...
                c = '%';
            } else if (*it == 'c') {
                // Directive to centre text
                do_centre = true;
            } else if (*it == 'l') {
                // Directive to left-justify text
                do_centre = false;
            }
            // (%-anything-else is just swallowed.)
            ++it;
        } else if (*it == '\n') {
            at_newline = true;
            ++it;
        } else {
            // Normal character -- fetch it in, and increment the iterator.
            c = *it;
            ++it;
        }
        
        // Append the character to the line.
        if (c != 0) line += c;
        
        // Calculate the new line length
        const int line_width = printer.getTextWidth(line);
        
        // Check to see if we are at a possible line break point.
        // (This means a non-space character where the next character IS a space.)
        if (c != 0 && !IsSpace(c) && (it == text.end() || IsSpace(*it))) {
            if (line_width <= max_line_width) {
                // OK, approve this point as a possible line break position.
                fit_chars = line.length();
            }
        }
        
        // If the line has got too big (or we have reached an explicit
        // newline) then we should dump everything up to the previous line
        // break point, then start a new line.
        if (line_width > max_line_width || at_newline) {
            
            // Special case: if fit_chars == 0 (&& !line.empty()) then
            // reset fit_chars to line.length() [or line.length() - 1 if
            // the entire line would be too big]. This is the case where
            // the entire line is too big to print, but there were no
            // possible line break points.
            if (fit_chars == 0 && !line.empty()) {
                if (at_newline) fit_chars = line.length();
                else fit_chars = line.length() - 1;
            }

            // Print out the line (up to the last approved break point)
            const std::string to_print = line.substr(0, fit_chars);
            printer.printLine(to_print, y, do_centre);
            y += printer.getTextHeight();
            
            // Reset for the next line (skip any spaces)
            while (fit_chars < line.length() && IsSpace(line[fit_chars])) ++fit_chars;
            line = line.substr(fit_chars, std::string::npos);
            fit_chars = 0;
        }
    }

    return y;
}
