/*
 * FILE:
 *   dx11_gfx_context.cpp
 *
 * AUTHOR:
 *   Stephen Thompson <stephen@solarflare.org.uk>
 *
 * CREATED:
 *   21-Oct-2011
 *   
 * COPYRIGHT:
 *   Copyright (C) Stephen Thompson, 2008 - 2024.
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

#include "dx11_gfx_context.hpp"
#include "dx11_graphic.hpp"
#include "primitive_batch.hpp"
#include "../core/dx_error.hpp"
#include "../../gfx/pixel_array.hpp"
#include "../../gfx/rectangle.hpp"

namespace Coercri {
    
    DX11GfxContext::DX11GfxContext(ID3D11Device *pDevice,
                                   ID3D11DeviceContext *pDeviceContext,
                                   ID3D11RenderTargetView *pRenderTargetView,
                                   IDXGISwapChain *pSwapChain,
                                   PrimitiveBatch &prim_batch)
        : device(pDevice),
          device_context(pDeviceContext),
          render_target_view(pRenderTargetView),
          swap_chain(pSwapChain),
          primitive_batch(prim_batch)
    {
        primitive_batch.begin(render_target_view);
    }

    DX11GfxContext::~DX11GfxContext()
    {
        primitive_batch.end();

        swap_chain->Present(0, 0);
        // NOTE: HRESULT from Present is unchecked, because
        // destructors should not throw exceptions.
    }
    
    void DX11GfxContext::setClipRectangle(const Rectangle &rect)
    {
        primitive_batch.setScissorRectangle(rect);
    }

    void DX11GfxContext::clearClipRectangle()
    {
        primitive_batch.clearScissorRectangle();
    }

    Rectangle DX11GfxContext::getClipRectangle() const
    {
        return primitive_batch.getScissorRectangle();
    }

    int DX11GfxContext::getWidth() const
    {
        return primitive_batch.getWidth();
    }

    int DX11GfxContext::getHeight() const
    {
        return primitive_batch.getHeight();
    }

    void DX11GfxContext::clearScreen(Color col)
    {
        float rgba[] = {
            col.r / 255.0f,
            col.g / 255.0f,
            col.b / 255.0f,
            1.0f  // Coercri does not use the back buffer alpha channel. May as well just set to 1.
        };
        device_context->ClearRenderTargetView(render_target_view, rgba);
    }

    void DX11GfxContext::plotPixel(int x, int y, Color col)
    {
        primitive_batch.plotPixel(x, y, col);
    }

    void DX11GfxContext::drawGraphic(int x, int y, const Graphic &graphic)
    {
        primitive_batch.drawGraphic(x, y, dynamic_cast<const DX11Graphic&>(graphic));
    }

    void DX11GfxContext::drawLine(int x1, int y1, int x2, int y2, Color col)
    {
        primitive_batch.drawLine(x1, y1, x2, y2, col);
    }

    void DX11GfxContext::fillRectangle(const Rectangle &rect, Color col)
    {
        primitive_batch.fillRectangle(rect, col);
    }

    void DX11GfxContext::plotPixelBatch(const Pixel *buf, int num)
    {
        primitive_batch.plotPixelBatch(buf, num);
    }

    namespace {
        class MapTexture {
        public:
            MapTexture(ID3D11DeviceContext &cxt, ID3D11Texture2D &tex)
                : context(cxt), texture(tex)
            {
                HRESULT hr = context.Map(&texture, 0, D3D11_MAP_READ, 0, &msr);
                if (FAILED(hr)) {
                    throw DXError("MapTexture failed", hr);
                }
            }

            ~MapTexture()
            {
                context.Unmap(&texture, 0);
            }

            D3D11_MAPPED_SUBRESOURCE msr;

        private:
            ID3D11DeviceContext &context;
            ID3D11Texture2D &texture;  // must be a staging texture
        };
    }
    
    boost::shared_ptr<PixelArray> DX11GfxContext::takeScreenshot()
    {
        // We must temporarily flush the primitive batch so that we
        // get a correct screenshot.
        // NOTE: It might be better if there was a PrimitiveBatch::flush()
        // call, but there isn't one, so we have to do this "kludge" of
        // ending and then re-beginning again after we have taken the
        // screenshot.
        primitive_batch.end();

        // Get access to the back buffer
        ID3D11Texture2D *pBuffer;
        HRESULT hr = swap_chain->GetBuffer(0, __uuidof(pBuffer), reinterpret_cast<void**>(&pBuffer));
        if (FAILED(hr)) {
            throw DXError("takeScreenshot: GetBuffer failed", hr);
        }
        ComPtrWrapper<ID3D11Texture2D> psBuffer(pBuffer);

        // Get description of the back buffer, and modify it
        // to be suitable for creating a staging texture
        D3D11_TEXTURE2D_DESC desc;
        pBuffer->GetDesc(&desc);
        desc.BindFlags = 0;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        desc.Usage = D3D11_USAGE_STAGING;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

        // Create a staging texture
        ID3D11Texture2D *pStaging;
        hr = device->CreateTexture2D(&desc, 0, &pStaging);
        if (FAILED(hr)) {
            throw DXError("takeScreenshot: CreateTexture2D failed", hr);
        }
        ComPtrWrapper<ID3D11Texture2D> psStaging(pStaging);

        // Copy the back-buffer to the staging texture
        device_context->CopyResource(pStaging, pBuffer);

        // Map the staging texture into memory
        MapTexture m(*device_context, *pStaging);

        // Copy to a PixelArray.
        boost::shared_ptr<PixelArray> result(new PixelArray(desc.Width, desc.Height));
        for (int y = 0; y < int(desc.Height); ++y) {
            const unsigned char *q = reinterpret_cast<const unsigned char*>(m.msr.pData) + y * m.msr.RowPitch;
            for (int x = 0; x < int(desc.Width); ++x) {
                const unsigned char *p = q + x * 4;
                (*result)(x,y) = Color(p[0], p[1], p[2], 255);
            }
        }

        // Can now put the primitive_batch back how it was.
        primitive_batch.begin(render_target_view);

        return result;
    }
}
