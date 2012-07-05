/*
 * FILE:
 *   primitive_batch.cpp
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

#include "dx11_graphic.hpp"
#include "primitive_batch.hpp"
#include "../core/dx_error.hpp"
#include "../core/load_dx11_dlls.hpp"

#include <cmath>
#include <string>

#include <d3dcompiler.h>
#include <d3dx10math.h>

namespace Coercri {

    struct PrimitiveBatchVertex {
        D3DXVECTOR2 pos;
        unsigned char r;
        unsigned char g;
        unsigned char b;
        unsigned char a;
    };

    struct MyConstBuffer {
        int width;
        int height;
    };

    namespace {
        const int NUM_VERTS_IN_BUFFER = 2000;   // what is best value for this??

        void CompileShader(const char *shader_code,
                           const char *filename,
                           const char *entry_point,
                           const char *profile,
                           ComPtrWrapper<ID3DBlob> &compiled_code)
        {
            DWORD compile_flags = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_WARNINGS_ARE_ERRORS;
#if defined(DEBUG) | defined(_DEBUG)
            compile_flags |= D3DCOMPILE_DEBUG;
#endif

            ID3DBlob *output_blob;
            ID3DBlob *error_blob;

            HRESULT hr = D3DCompile_Wrapper(shader_code,
                                            strlen(shader_code),
                                            filename,
                                            0,  // macro definitions
                                            0,  // include file handler
                                            entry_point,
                                            profile,
                                            compile_flags,
                                            0,   // effect flags (ignored for vertex/pixel shaders)
                                            &output_blob,
                                            &error_blob);

            compiled_code.reset(output_blob);
            ComPtrWrapper<ID3DBlob> error_sentinel(error_blob);
            
            if (FAILED(hr)) {
                std::string msg = "D3DCompile failed";
                if (error_blob) {
                    // Include the compiler errors in the exception message.
                    // Also output using OutputDebugStringA (in case it is too long for a message box)
                    OutputDebugStringA((char*)error_blob->GetBufferPointer());
                    msg += "\n\n";
                    msg += (char*)error_blob->GetBufferPointer();
                }
                throw DXError(msg, hr);
            }
        }

        int RoundUpTo16(int x)
        {
            return (x + 15) & (~15);
        }
    }

    //
    // Initialization
    //
    
    PrimitiveBatch::PrimitiveBatch(ID3D11Device *device,
                                   ID3D11DeviceContext *device_context_)
        : device_context(device_context_),
          begun(false),
          prim_started(false),
          end_vertex(0)
    {
        createShaders(device);
        createVertexBuffer(device);
        createRasterizerState(device);
        createBlendState(device);
        createSampler(device);
        createConstantBuffer(device);
    }

    // Create the required vertex and pixel shaders
    // Also create corresponding input layout
    void PrimitiveBatch::createShaders(ID3D11Device *device)
    {
        // code for vertex and pixel shader
        // vertex shader --> transforms from pixel coordinates to normalized device coordinates
        // pixel shader --> just passes through the given colour
        const char * shader_code =
            "Texture2D myTexture : register( t0 );\n"
            "SamplerState mySampler : register( s0 );\n"
            "\n"
            "cbuffer MyConstBuffer : register( b0 )\n"
            "{\n"
            "    int width;\n"
            "    int height;\n"
            "};\n"
            "\n"
            "struct VS_INPUT\n"
            "{\n"
            "    float2 Pos : POSITION;\n"
            "    unorm float4 Col : COLOR;\n"
            "};\n"
            "\n"
            "struct PS_INPUT\n"
            "{\n"
            "    float4 Pos : SV_POSITION;\n"
            "    float4 Col : COLOR;\n"
            "};\n"
            "\n"
            "PS_INPUT VS( VS_INPUT input )\n"
            "{\n"
            "    PS_INPUT output;\n"
            "    output.Pos = float4( ( 2 * input.Pos.x - width)  / width,   \n"
            "                         (-2 * input.Pos.y + height) / height,  \n"
            "                         0.5f,   \n"
            "                         1.0f ); \n"
            "    output.Col = input.Col;\n"
            "    return output;\n"
            "}\n"
            "\n"
            "float4 PS( PS_INPUT input ) : SV_TARGET\n"
            "{\n"
            "    return input.Col;\n"
            "}\n"
            "\n"
            "float4 PS_Tex( PS_INPUT input ) : SV_TARGET\n"
            "{\n"
            "    return myTexture.Sample(mySampler, float2(input.Col.x, input.Col.y));\n"
            "}\n";

        const char * shader_code_name = "coercri_shaders";  // dummy "filename" for the shader code

        // compile the vertex shader to a Blob
        ComPtrWrapper<ID3DBlob> code_blob;
        CompileShader(shader_code,
                      shader_code_name,
                      "VS",
                      "vs_4_0_level_9_1",  // TODO might need to change this for diff feature levels?
                      code_blob);

        // now we can create the vertex shader
        ID3D11VertexShader *vert_shader = 0;
        HRESULT hr = device->CreateVertexShader(code_blob->GetBufferPointer(),
                                                code_blob->GetBufferSize(),
                                                0,
                                                &vert_shader);
        if (FAILED(hr)) {
            throw DXError("CreateVertexShader failed", hr);
        }
        m_psVertexShader.reset(vert_shader);

        // create input layout while we still have the code blob
        D3D11_INPUT_ELEMENT_DESC layout[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 8, D3D11_INPUT_PER_VERTEX_DATA, 0 }
        };

        ID3D11InputLayout *input_layout;
        hr = device->CreateInputLayout(&layout[0],
                                       2,
                                       code_blob->GetBufferPointer(),
                                       code_blob->GetBufferSize(),
                                       &input_layout);
        if (FAILED(hr)) {
            throw DXError("CreateInputLayout failed", hr);
        }
        m_psInputLayout.reset(input_layout);
        
        // compile the pixel shader to a Blob
        CompileShader(shader_code,
                      shader_code_name,
                      "PS",
                      "ps_4_0_level_9_1",
                      code_blob);

        // create the pixel shader
        ID3D11PixelShader *pix_shader = 0;
        hr = device->CreatePixelShader(code_blob->GetBufferPointer(),
                                       code_blob->GetBufferSize(),
                                       0,
                                       &pix_shader);
        if (FAILED(hr)) {
            throw DXError("CreatePixelShader failed", hr);
        }
        m_psPixelShader.reset(pix_shader);

        // compile the 2nd pixel shader (for texture blits)
        CompileShader(shader_code, shader_code_name, "PS_Tex", "ps_4_0_level_9_1", code_blob);
        hr = device->CreatePixelShader(code_blob->GetBufferPointer(), code_blob->GetBufferSize(), 0, &pix_shader);
        if (FAILED(hr)) {
            throw DXError("CreatePixelShader (2) failed", hr);
        }
        m_psPixelShader_Tex.reset(pix_shader);
    }

    // Create the dynamic vertex buffer
    void PrimitiveBatch::createVertexBuffer(ID3D11Device *device)
    {
        D3D11_BUFFER_DESC bd;
        memset(&bd, 0, sizeof(bd));
        bd.ByteWidth = sizeof(PrimitiveBatchVertex) * NUM_VERTS_IN_BUFFER;
        bd.Usage = D3D11_USAGE_DYNAMIC;
        bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        ID3D11Buffer *vert_buf = 0;
        HRESULT hr = device->CreateBuffer(&bd, 0, &vert_buf);
        if (FAILED(hr)) {
            throw DXError("Vertex buffer creation failed", hr);
        }
        m_psVertexBuffer.reset(vert_buf);
    }

    // Create the rasterizer state
    void PrimitiveBatch::createRasterizerState(ID3D11Device *device)
    {
        D3D11_RASTERIZER_DESC rd;
        memset(&rd, 0, sizeof(rd));
        rd.FillMode = D3D11_FILL_SOLID;
        rd.CullMode = D3D11_CULL_NONE;   // we are not interested in backface culling
        rd.DepthClipEnable = true;  // just leave this at the default. it doesn't make any difference as we always set z=0.5.
        rd.ScissorEnable = true;
        // all other values set to zero

        ID3D11RasterizerState *state = 0;
        HRESULT hr = device->CreateRasterizerState(&rd, &state);
        if (FAILED(hr)) {
            throw DXError("CreateRasterizerState failed", hr);
        }
        m_psRasterizerState.reset(state);        
    }

    // Create the blend state
    void PrimitiveBatch::createBlendState(ID3D11Device *device)
    {
        D3D11_BLEND_DESC bd;
        memset(&bd, 0, sizeof(bd));
        bd.RenderTarget[0].BlendEnable = true;
        bd.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
        bd.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        bd.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        bd.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
        bd.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
        bd.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        bd.RenderTarget[0].RenderTargetWriteMask =
            D3D11_COLOR_WRITE_ENABLE_RED
            | D3D11_COLOR_WRITE_ENABLE_GREEN
            | D3D11_COLOR_WRITE_ENABLE_BLUE;

        ID3D11BlendState *state = 0;
        HRESULT hr = device->CreateBlendState(&bd, &state);
        if (FAILED(hr)) {
            throw DXError("CreateBlendState failed", hr);
        }
        m_psBlendState.reset(state);
    }

    // Create the sampler state
    void PrimitiveBatch::createSampler(ID3D11Device *device)
    {
        D3D11_SAMPLER_DESC sd;
        memset(&sd, 0, sizeof(sd));
        sd.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
        sd.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
        sd.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
        sd.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        sd.ComparisonFunc = D3D11_COMPARISON_NEVER;
        sd.MinLOD = 0;
        sd.MaxLOD = D3D11_FLOAT32_MAX;

        ID3D11SamplerState *sampler;
        HRESULT hr = device->CreateSamplerState(&sd, &sampler);
        if (FAILED(hr)) {
            throw DXError("ID3D11Device::CreateSamplerState failed", hr);
        }
        m_psSampler.reset(sampler);
    }

    // Create the constant buffer
    void PrimitiveBatch::createConstantBuffer(ID3D11Device *device)
    {
        D3D11_BUFFER_DESC bd;
        memset(&bd, 0, sizeof(bd));
        bd.ByteWidth = RoundUpTo16(sizeof(MyConstBuffer));
        bd.Usage = D3D11_USAGE_DEFAULT;
        bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

        ID3D11Buffer *buf = 0;
        HRESULT hr = device->CreateBuffer(&bd, 0, &buf);
        if (FAILED(hr)) {
            throw DXError("Constant buffer creation failed", hr);
        }
        m_psConstantBuffer.reset(buf);
    }
    

    //
    // Begin and end
    //
    
    void PrimitiveBatch::begin(ID3D11RenderTargetView *render_target_view)
    {
        // Error check
        if (begun) {
            throw CoercriError("Can only create one gfx context at a time");
        }
        begun = true;
        
        // Get the width and height of the render target
        ID3D11Resource *resource;
        render_target_view->GetResource(&resource);
        ComPtrWrapper<ID3D11Resource> psResource(resource);

        ID3D11Texture2D *texture;
        resource->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&texture));
        ComPtrWrapper<ID3D11Texture2D> psTexture(texture);

        D3D11_TEXTURE2D_DESC td;
        texture->GetDesc(&td);

        width = td.Width;
        height = td.Height;

        // Write width, height to the constant buffer.
        MyConstBuffer cb;
        cb.width = width;
        cb.height = height;
        device_context->UpdateSubresource(m_psConstantBuffer.get(),
                                          0,  // subresource
                                          0,  // overwrite whole resource
                                          &cb,
                                          0,  // row pitch
                                          0); // slab pitch
        
        
        // Set up all rendering state necessary for our drawing routines to work.
        
        // start by clearing any existing state.
        device_context->ClearState();

        // input assembler
        // (setup the vertex buffer)
        device_context->IASetInputLayout(m_psInputLayout.get());

        ID3D11Buffer * vert_buf = m_psVertexBuffer.get();
        const UINT stride = sizeof(PrimitiveBatchVertex);
        const UINT offset = 0;
        device_context->IASetVertexBuffers(0, 1, &vert_buf, &stride, &offset);

        // vertex shader
        device_context->VSSetShader(m_psVertexShader.get(), 0, 0);
        
        ID3D11Buffer * cst_buf = m_psConstantBuffer.get();
        device_context->VSSetConstantBuffers(0, 1, &cst_buf);
        
        // rasterizer
        device_context->RSSetState(m_psRasterizerState.get());
        clearScissorRectangle();  // set scissor rectangle to entire screen, initially.
        
        D3D11_VIEWPORT vp;
        memset(&vp, 0, sizeof(vp));
        vp.Width = float(width);
        vp.Height = float(height);
        vp.MinDepth = 0.0f;
        vp.MaxDepth = 1.0f;
        vp.TopLeftX = 0;
        vp.TopLeftY = 0;
        device_context->RSSetViewports(1, &vp);

        // pixel shader 
        // (sampler state -- the shader itself is set dynamically
        // depending on whether we are drawing flat-shaded shapes or blitting textures)
        ID3D11SamplerState * sampler = m_psSampler.get();
        device_context->PSSetSamplers(0, 1, &sampler);
        
        // output merger
        // (enable alpha blending)
        FLOAT BlendFactor[4] = { 0, 0, 0, 0 };
        device_context->OMSetBlendState(m_psBlendState.get(), BlendFactor, 0xffffffff);

        // (render to the given render target view; no depth/stencil required)
        device_context->OMSetRenderTargets(1, &render_target_view, 0);
    }
    
    void PrimitiveBatch::end()
    {
        // error check
        if (!begun) throw CoercriError("Incorrect call to PrimitiveBatch::end()");
        begun = false;
        
        // make sure there is no primitive still in progress
        if (prim_started) endPrim();
    }

    
    //
    // drawing routines
    //

    void PrimitiveBatch::plotPixel(int x, int y, Color col)
    {
        setupPrim(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST, 0, 1);
        addVertex(x + 0.5f, y + 0.5f, col);
    }
    
    void PrimitiveBatch::plotPixelBatch(const Pixel *buf, int num)
    {
        if (num > NUM_VERTS_IN_BUFFER) {
            plotPixelBatch(buf, NUM_VERTS_IN_BUFFER);
            plotPixelBatch(buf + NUM_VERTS_IN_BUFFER, num - NUM_VERTS_IN_BUFFER);
        } else {
        
            setupPrim(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST, 0, num);
            
            for (int i = 0; i < num; ++i) {
                addVertex(buf->x + 0.5f, buf->y + 0.5f, buf->col);
                ++buf;
            }
        }
    }

    void PrimitiveBatch::drawLine(int x1, int y1, int x2, int y2, Color col)
    {
        setupPrim(D3D11_PRIMITIVE_TOPOLOGY_LINELIST, 0, 2);

        // be careful about direct3d rasterization rules.
        // in Coercri, both endpoints are to be included in the line.
        
        if (x1==x2 && y1==y2) {
            plotPixel(x1, y1, col);

        } else {
            float xstart = x1 + 0.5f;
            float ystart = y1 + 0.5f;

            float xend = x2 + 0.5f;
            float yend = y2 + 0.5f;

            // extend the line by 1 pixel to account for diamond exit rule.
            float addx = float(x2 - x1);
            float addy = float(y2 - y1);
            float inv_len = 1.0f / std::sqrt(addx*addx + addy*addy);
            addx *= inv_len;
            addy *= inv_len;
            xend += addx;
            yend += addy;
            
            addVertex(xstart, ystart, col);
            addVertex(xend, yend, col);
        }
    }

    void PrimitiveBatch::fillRectangle(const Rectangle &rect, Color col)
    {
        setupPrim(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST, 0, 6);

        addVertex(float(rect.getLeft()), float(rect.getTop()), col);
        addVertex(float(rect.getRight()), float(rect.getTop()), col);
        addVertex(float(rect.getLeft()), float(rect.getBottom()), col);

        addVertex(float(rect.getRight()), float(rect.getTop()), col);
        addVertex(float(rect.getRight()), float(rect.getBottom()), col);
        addVertex(float(rect.getLeft()), float(rect.getBottom()), col);
    }

    void PrimitiveBatch::drawGraphic(int x, int y, const DX11Graphic &gfx)
    {
        setupPrim(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST, gfx.getShaderResourceView(), 6);

        int hx, hy;
        gfx.getHandle(hx, hy);
        x -= hx;
        y -= hy;

        const float left = float(x);
        const float top = float(y);
        const float right = float(x + gfx.getWidth());
        const float bottom = float(y + gfx.getHeight());
        
        addTexVertex(left, top, 0, 0);
        addTexVertex(right, top, 255, 0);
        addTexVertex(left, bottom, 0, 255);

        addTexVertex(right, top, 255, 0);
        addTexVertex(right, bottom, 255, 255);
        addTexVertex(left, bottom, 0, 255);
    }

    //
    // scissor rectangle
    //

    void PrimitiveBatch::setScissorRectangle(const Rectangle &rect)
    {
        // make sure to flush any old geometry (that used the old scissor rectangle) first
        if (prim_started) endPrim();
        
        D3D11_RECT dr;
        dr.left = rect.getLeft();
        dr.top = rect.getTop();
        dr.right = rect.getRight();
        dr.bottom = rect.getBottom();
        device_context->RSSetScissorRects(1, &dr);
    }

    void PrimitiveBatch::clearScissorRectangle()
    {
        if (prim_started) endPrim();
        D3D11_RECT dr;
        dr.left = 0;
        dr.top = 0;
        dr.right = getWidth();
        dr.bottom = getHeight();
        device_context->RSSetScissorRects(1, &dr);
    }

    Rectangle PrimitiveBatch::getScissorRectangle() const
    {
        UINT num_rects;
        D3D11_RECT rects[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
        device_context->RSGetScissorRects(&num_rects, &rects[0]);
        if (num_rects == 0) {  // shouldn't happen, unless user has been messing with scissor state...
            return Rectangle(0, 0, getWidth(), getHeight());
        } else {
            return Rectangle(rects[0].left, rects[0].top, rects[0].right - rects[0].left, rects[0].bottom - rects[0].top);
        }
    }

    
    //
    // private methods
    //

    void PrimitiveBatch::setupPrim(D3D11_PRIMITIVE_TOPOLOGY prim_type,
                                   ID3D11ShaderResourceView *tex_resource_view,
                                   int nverts)
    {
        // Check if there is enough space in the buffer. If not, close
        // out the current primitive, and "free up" space in the
        // buffer (by setting end_vertex back to zero).
        if (end_vertex + nverts >= NUM_VERTS_IN_BUFFER) {
            if (prim_started) endPrim();
            end_vertex = 0;
        }

        // If a primitive is in progress, but it is of the wrong type,
        // then close out the current primitive so we can start a new
        // one. (but keep same location in the buffer.)
        if (prim_started && (curr_prim_type != prim_type || curr_tex_resource_view != tex_resource_view)) {
            endPrim();
        }

        // If no primitive is in progress, then start one.
        if (!prim_started) {
            startPrim(prim_type, tex_resource_view);
        }

        // at this point we know that prim_started == true,
        // curr_prim_type == prim_type, start_vertex and end_vertex
        // are correctly set, buffer is mapped, and there is enough
        // space for nverts vertices to be added.
    }

    void PrimitiveBatch::startPrim(D3D11_PRIMITIVE_TOPOLOGY prim_type,
                                   ID3D11ShaderResourceView *tex_resource_view)
    {
        // precondition: !prim_started

        // set D3D states accordingly
        device_context->IASetPrimitiveTopology(prim_type);

        if (tex_resource_view) {
            // texture blit
            device_context->PSSetShader(m_psPixelShader_Tex.get(), 0, 0);
            device_context->PSSetShaderResources(0, 1, &tex_resource_view);
        } else {
            // flat shaded primitive
            device_context->PSSetShader(m_psPixelShader.get(), 0, 0);
        }

        // ensure the "runtime state" variables are set correctly
        prim_started = true;
        start_vertex = end_vertex;
        curr_prim_type = prim_type;
        curr_tex_resource_view = tex_resource_view;

        // map the vertex buffer
        // If start_vertex == 0 then discard any existing buffer contents
        // If start_vertex > 0 then use NO_OVERWRITE so GPU can keep using the earlier verts.
        D3D11_MAPPED_SUBRESOURCE msr;
        HRESULT hr = device_context->Map(m_psVertexBuffer.get(),
                                         0,  // subresource
                                         start_vertex == 0 ? D3D11_MAP_WRITE_DISCARD
                                                           : D3D11_MAP_WRITE_NO_OVERWRITE,
                                         0,  // flags (0 = Block if GPU is busy)
                                         &msr);
        if (FAILED(hr)) {
            throw DXError("PrimitiveBatch: Failed to map vertex buffer", hr);
        }
        vb_ptr = static_cast<PrimitiveBatchVertex*>(msr.pData);
    }
    
    void PrimitiveBatch::endPrim()
    {
        // precondition: prim_started

        // unmap the buffer (I believe this is necessary before we can call Draw)
        device_context->Unmap(m_psVertexBuffer.get(), 0);

        // draw the primitive
        device_context->Draw(end_vertex - start_vertex, start_vertex);

        // update the state variables
        prim_started = false;
    }
    
    void PrimitiveBatch::addVertex(float x, float y, Color col)
    {
        PrimitiveBatchVertex &vert = vb_ptr[end_vertex];
        vert.pos.x = x;
        vert.pos.y = y;
        vert.r = col.r;
        vert.g = col.g;
        vert.b = col.b;
        vert.a = col.a;
        ++end_vertex;
    }

    void PrimitiveBatch::addTexVertex(float x, float y, unsigned char tx, unsigned char ty)
    {
        PrimitiveBatchVertex &vert = vb_ptr[end_vertex];
        vert.pos.x = x;
        vert.pos.y = y;
        vert.r = tx;
        vert.g = ty;
        ++end_vertex;
    }
}
