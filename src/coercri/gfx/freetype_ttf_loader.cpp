/*
 * FILE:
 *   freetype_ttf_loader.cpp
 *
 * AUTHOR:
 *   Stephen Thompson <stephen@solarflare.org.uk>
 *
 * CREATED:
 *   21-May-2012
 *   
 * COPYRIGHT:
 *   Copyright (C) Stephen Thompson, 2012.
 *
 *   This file is part of the "Coercri" software library. Usage of "Coercri"
 *   is permitted under the terms of the Boost Software License, Version 1.0, 
 *   the text of which is displayed below.
 *
 *   Boost Software License - Version 1.0 - August 17th, 2003
 *
 *   Permission is hereby granted, free of charge, to any person or organization
 *   obtaining a copy of the software and accompanying documentation covered by
 *   this license (the "Software") to use, reproduce, display, distribute,
 *   execute, and transmit the Software, and to prepare derivative works of the
 *   Software, and to permit third-parties to whom the Software is furnished to
 *   do so, all subject to the following:
 *
 *   The copyright notices in the Software and this entire statement, including
 *   the above license grant, this restriction and the following disclaimer,
 *   must be included in all copies of the Software, in whole or in part, and
 *   all derivative works of the Software, unless such copies or derivative
 *   works are solely in the form of machine-executable object code generated by
 *   a source language processor.
 *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *   FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
 *   SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
 *   FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
 *   ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 *   DEALINGS IN THE SOFTWARE.
 *
 */

#include "bitmap_font.hpp"
#include "freetype_ttf_loader.hpp"
#include "kern_table.hpp"
#include "../core/coercri_error.hpp"

#include <ft2build.h>
#include FT_FREETYPE_H

#include <sstream>
#include <vector>

namespace {
    // Globals
    bool g_initialized = false;
    FT_Library g_library;

    // Error checking
    void ThrowFTError(FT_Error err)
    {
        if (err) {
            std::ostringstream str;
            str << "Freetype error code " << err;
            throw Coercri::CoercriError(str.str());
        }
    }

#define FT_CHK_ERR(code) { FT_Error err = code; if (err) ThrowFTError(err); }

    // Conversion from Freetype 26.6 format
    inline int MyCeil(int x)
    {
        return (((x + 63) & -64) / 64);
    }

    // Freetype IO functions
    unsigned long ReadFunc(FT_Stream stream,
                           unsigned long offset,
                           unsigned char *buffer,
                           unsigned long count)
    {
        std::istream &str = *static_cast<std::istream*>(stream->descriptor.pointer);
        str.seekg(offset, std::ios_base::beg);
        
        // sometimes FT calls us w/ buffer == count == 0
        if (count == 0) return 0;

        str.read(reinterpret_cast<char*>(buffer), count);
        return str.gcount();
    }

    void CloseFunc(FT_Stream stream)
    {
        // We cannot actually close the stream until the FreetypeFont object is destroyed.
        // Therefore, we just do nothing in this function.
    }


    // Our kerning implementation class.

    struct KernEntry {
        unsigned char first, second;
        short int value;
    };

    bool operator<(const KernEntry &lhs, const KernEntry &rhs)
    {
        // compare the characters only
        return lhs.first < rhs.first ? true
             : lhs.first > rhs.first ? false
             : lhs.second < rhs.second;
    }
    
    class FreetypeKernTable : public Coercri::KernTable {
    public:
        virtual int getKern(char first, char second) const;

        // make the vector public so it can be easily set in the code below.
        std::vector<KernEntry> kern_table;
    };

    int FreetypeKernTable::getKern(char first, char second) const
    {
        KernEntry search;
        search.first = first;
        search.second = second;
    
        std::vector<KernEntry>::const_iterator it = std::lower_bound(kern_table.begin(), kern_table.end(), search);

        if (it != kern_table.end() && it->first == first && it->second == second) {
            return it->value;
        } else {
            return 0;
        }
    }


    // This function copies an FT_Bitmap into a given glyph of a Coercri::BitmapFont
    void CopyBitmapToFont(const FT_Bitmap &bitmap, Coercri::BitmapFont &dest, char ch)
    {
        const unsigned char * ptr = bitmap.buffer;
        for (int y = 0; y < bitmap.rows; ++y) {
            for (int x = 0; x < bitmap.width; ++x) {
                dest.plotPixel(ch, x, y, ptr[x]);
            }
            ptr += bitmap.pitch;
        }
    }
}


boost::shared_ptr<Coercri::Font> Coercri::FreetypeTTFLoader::loadFont(boost::shared_ptr<std::istream> str, int size)
{
    // Initialize Freetype library if required
    if (!g_initialized) {
        FT_CHK_ERR( FT_Init_FreeType(&g_library) );
        g_initialized = true;
    }

    // Calculate the file size, this is needed by Freetype
    str->seekg(0, std::ios::end);
    size_t file_size = str->tellg();
    str->seekg(0, std::ios::beg);

    // Setup the Freetype "stream" object
    FT_StreamRec stream_rec;
    std::memset(&stream_rec, 0, sizeof(stream_rec));
    stream_rec.size = file_size;
    stream_rec.pos = 0;
    stream_rec.descriptor.pointer = str.get();
    stream_rec.read = &ReadFunc;
    stream_rec.close = &CloseFunc;

    // Setup the FT_Open_Args
    FT_Open_Args args;
    std::memset(&args, 0, sizeof(args));
    args.flags = FT_OPEN_STREAM;
    args.stream = &stream_rec;

    // Open the font
    FT_Face face = 0;
    FT_CHK_ERR( FT_Open_Face(g_library, &args, 0, &face) );

    // not using RAII for face, so need explicit try/catch here.
    try {

        // Set the size
        FT_CHK_ERR( FT_Set_Char_Size(face, 0, size*64, 0, 0) );

        // Get the font height
        const int ascent = MyCeil(face->size->metrics.ascender);
        const int descent = MyCeil(face->size->metrics.descender);
        const int ft_height = MyCeil(face->size->metrics.height);
        const int height = std::max(ascent - descent + 1, ft_height);

        // Create the BitmapFont
        boost::shared_ptr<FreetypeKernTable> kern_table(new FreetypeKernTable);
        boost::shared_ptr<Coercri::BitmapFont> bitmap_font(new Coercri::BitmapFont(
            kern_table, height));

        // Render each character glyph in turn.
        for (int ch = 0; ch < 256; ++ch) {
            
            // Convert ASCII/Latin1 code to glyph index
            FT_UInt glyph_index = FT_Get_Char_Index(face, ch);

            // Load glyph image into the slot (erase previous one).
            // NOTE: we use FT_LOAD_FORCE_AUTOHINT, this is because
            // the results (in my opinion) look better with the
            // freetype autohinter, as compared to the native TrueType
            // hinting.
            FT_Error err = FT_Load_Glyph(face, glyph_index, FT_LOAD_RENDER | FT_LOAD_FORCE_AUTOHINT);
            if (err) continue;  // Skip glyphs with errors

            // Create the target surface
            bitmap_font->setupCharacter(ch, face->glyph->bitmap.width, face->glyph->bitmap.rows,
                                        face->glyph->bitmap_left,
                                        ascent - face->glyph->bitmap_top,
                                        face->glyph->advance.x >> 6);
            
            // Now draw to the target surface
            CopyBitmapToFont(face->glyph->bitmap, *bitmap_font, ch);
        }

        // Kerning
        for (int c1 = 1; c1 < 256; ++c1) {
            for (int c2 = 1; c2 < 256; ++c2) {
                FT_UInt prev = FT_Get_Char_Index(face, c1);
                FT_UInt next = FT_Get_Char_Index(face, c2);
                if (prev && next) {
                    FT_Vector delta;
                    FT_Get_Kerning(face, prev, next, FT_KERNING_DEFAULT, &delta);
                    const int kern = delta.x >> 6;
                    if (kern != 0) {
                        KernEntry entry;
                        entry.first = c1;
                        entry.second = c2;
                        entry.value = kern;
                        kern_table->kern_table.push_back(entry);
                    }
                }
            }
        }
        
        // clean up.
        FT_Done_Face(face);

        return bitmap_font;
        
    } catch (...) {
        // Make sure face gets freed if there was an exception
        FT_Done_Face(face);
        throw;
    }
}

