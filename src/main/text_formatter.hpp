/*
 * text_formatter.hpp
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

/*
 * Prints out lines of text. Wraps long lines, and also handles "rich text"
 * directives if asked to.
 * 
 */

#ifndef TEXT_FORMATTER_HPP
#define TEXT_FORMATTER_HPP

#include "utf8string.hpp"

#include <string>

struct Printer {
    virtual int getTextWidth(const Coercri::UTF8String &text) = 0;
    virtual int getTextHeight() = 0;
    virtual void printLine(const Coercri::UTF8String &text, int y, bool do_centre) = 0;
};

class TextFormatter {
public:
    TextFormatter(Printer &p, int width, bool rich);

    // print a string, breaking into separate lines if necessary
    // returns the total height printed.
    int printString(const Coercri::UTF8String &msg);

private:
    Printer &printer;
    int max_line_width;
    bool is_rich;
    bool do_centre; // false initially; turned on by %c directives.
    int y;
};

#endif
