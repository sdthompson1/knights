/*
 * FILE:
 *   primitive_batch.hpp
 *
 * PURPOSE:
 *   PrimitiveBatch represents a batch of primitives (points, lines,
 *   rectangles, or texture blits) to be drawn. It manages all
 *   required resources (vertex buffer, vertex and pixel shaders, etc)
 *   and makes Draw calls on the device context supplied to the ctor.
 *   Draw calls are automatically batched as required.
 *
 *   Note that this is an implementation class and should not be used
 *   directly by end users.
 *   
 * AUTHOR:
 *   Stephen Thompson <stephen@solarflare.org.uk>
 *
 * CREATED:
 *   20-Oct-2011
 *   
 * COPYRIGHT:
 *   Copyright (C) Stephen Thompson, 2008 - 2011.
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

#ifndef COERCRI_PRIMITIVE_BATCH_HPP
#define COERCRI_PRIMITIVE_BATCH_HPP

#include "../core/com_ptr_wrapper.hpp"
#include "../../gfx/color.hpp"
#include "../../gfx/gfx_context.hpp"  // for Pixel struct
#include "../../gfx/rectangle.hpp"

#include <d3d11.h>
#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif

namespace Coercri {

    class DX11Graphic;
    struct PrimitiveBatchVertex;  // internal use
    
    class PrimitiveBatch {
    public:
        explicit PrimitiveBatch(ID3D11Device *device_,
                                ID3D11DeviceContext *device_context_);

        // Begin drawing on a particular render target.
        // 
        // NOTE: This calls ClearState() on the device context then
        // sets up various render states. The render states should not
        // be changed from outside PrimitiveBatch until end() is
        // called (unless you are careful!).
        //
        void begin(ID3D11RenderTargetView *render_target_view);
        
        // Available primitives.
        // These should not trigger Draw calls unless
        // 1) you change primitive type (e.g. from pixels to lines)
        // 2) the vertex buffer fills up
        // in which case a Draw will be done and a new batch started.
        void plotPixel(int x, int y, Color col);
        void plotPixelBatch(const Pixel *buf, int num);
        void drawLine(int x1, int y1, int x2, int y2, Color col);
        void fillRectangle(const Rectangle &rect, Color col);
        void drawGraphic(int x, int y, const DX11Graphic &graphic);
        
        // End drawing. This flushes any not-yet-drawn primitives
        // (making a Draw call) if necessary.
        void end();

        // Return width/height of render target
        int getWidth() const { return width; }
        int getHeight() const { return height; }

        // Scissor rectangle support.
        void setScissorRectangle(const Rectangle &rect);
        void clearScissorRectangle();
        Rectangle getScissorRectangle() const;
        
    private:
        void createShaders(ID3D11Device *device);
        void createVertexBuffer(ID3D11Device *device);
        void createRasterizerState(ID3D11Device *device);
        void createBlendState(ID3D11Device *device);
        void createSampler(ID3D11Device *device);
        void createConstantBuffer(ID3D11Device *device);
        
        void setupPrim(D3D11_PRIMITIVE_TOPOLOGY prim_type,
                       ID3D11ShaderResourceView *tex_resource_view,   // NULL if not a texture blit
                       int nverts);
        void startPrim(D3D11_PRIMITIVE_TOPOLOGY prim_type,
                       ID3D11ShaderResourceView *tex_resource_view);  // NULL if not a texture blit
        void endPrim();
        void addVertex(float x, float y, Color col);
        void addTexVertex(float x, float y, unsigned char tx, unsigned char ty);
        
    private:
        ID3D11DeviceContext *device_context;

        ComPtrWrapper<ID3D11VertexShader> m_psVertexShader;
        ComPtrWrapper<ID3D11VertexShader> m_psVertexShader_Tex;
        ComPtrWrapper<ID3D11PixelShader> m_psPixelShader;     // for points, lines, rectangles
        ComPtrWrapper<ID3D11PixelShader> m_psPixelShader_Tex; // for graphic blits
        ComPtrWrapper<ID3D11SamplerState> m_psSampler;

        ComPtrWrapper<ID3D11InputLayout> m_psInputLayout;
        ComPtrWrapper<ID3D11Buffer> m_psVertexBuffer;

        ComPtrWrapper<ID3D11RasterizerState> m_psRasterizerState;
        ComPtrWrapper<ID3D11BlendState> m_psBlendState;

        ComPtrWrapper<ID3D11Buffer> m_psConstantBuffer;

        // runtime state:
        
        // this is true if we are between begin() and end() calls. (used for error checking)
        bool begun;
        
        // if prim_started = true:
        // 
        //   -- curr_prim_type = what type of primitive is in progress
        //        (and this has been loaded into IASetPrimitiveTopology)
        //   -- start_vertex = where does it start in the buffer
        //   -- end_vertex = one-past-the-end of verts written so far.
        //   -- curr_tex_resource_view = texture being rendered (or NULL)
        //
        //   and the vertex buffer is Mapped, such that we can write
        //   any element between start_vertex and the end of the
        //   buffer.
        //   
        // if prim_started = false:
        // 
        //   -- curr_tex_resource_view, curr_prim_type, start_vertex are invalid
        //   -- end_vertex = where should the next prim be started from.
        //
        //   and the vertex buffer is Unmapped.
        //
        bool prim_started;
        D3D11_PRIMITIVE_TOPOLOGY curr_prim_type;
        ID3D11ShaderResourceView *curr_tex_resource_view;
        int start_vertex;
        int end_vertex;

        // if the vertex buffer is mapped, this contains the pointer,
        // otherwise it is undefined
        PrimitiveBatchVertex * vb_ptr;
        
        // these are set to the width, height of the render target
        // (they are undefined if begin() has not been called)
        int width, height;
    };

}

#endif
