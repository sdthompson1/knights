/*
 * FILE:
 *   dx11_gfx_driver.hpp
 *
 * PURPOSE:
 *   DirectX 11 implementation of GfxDriver
 *
 * AUTHOR:
 *   Stephen Thompson <stephen@solarflare.org.uk>
 *
 * CREATED:
 *   18-Oct-2011
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

#ifndef COERCRI_DX11_GFX_DRIVER_HPP
#define COERCRI_DX11_GFX_DRIVER_HPP

#include "../core/com_ptr_wrapper.hpp"
#include "../../gfx/gfx_driver.hpp"

#include <list>

#include <d3d11.h>
#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif

namespace Coercri {

    class PrimitiveBatch;
    class DX11Window;

    class DX11GfxDriver : public GfxDriver {
    public:
        // Constructor
        // Parameters are the params to D3D11CreateDevice.
        DX11GfxDriver(D3D_DRIVER_TYPE driver_type,
                      UINT flags,
                      D3D_FEATURE_LEVEL feature_level);
        ~DX11GfxDriver();

        // Functions overridden from GfxDriver
        virtual boost::shared_ptr<Window> createWindow(int width, int height,
                                                       bool resizable, bool fullscreen,
                                                       const std::string &title);
        virtual boost::shared_ptr<Graphic> createGraphic(boost::shared_ptr<const PixelArray> pixels,
                                                         int hx = 0, int hy = 0);
        virtual bool pollEvents();

        // Functions for accessing the Direct3D objects
        ID3D11Device * getDevice() { return m_psDevice.get(); }
        ID3D11DeviceContext * getDeviceContext() { return m_psDeviceContext.get(); }

        // DXGI accessors
        // Note: getPrimaryOutput returns the primary monitor (i.e. the one we will go full-screen onto).
        IDXGIFactory * getDXGIFactory() { return m_psFactory.get(); }
        IDXGIOutput * getPrimaryOutput() { return m_psOutput.get(); }

        // Access the PrimitiveBatch
        PrimitiveBatch & getPrimitiveBatch() { return *m_psPrimitiveBatch; }
                
    private:
        void createDevice(D3D_DRIVER_TYPE driver_type, UINT flags, D3D_FEATURE_LEVEL feature_level);

    private:
        ComPtrWrapper<ID3D11Device> m_psDevice;
        ComPtrWrapper<ID3D11DeviceContext> m_psDeviceContext;
        ComPtrWrapper<IDXGIFactory> m_psFactory;
        ComPtrWrapper<IDXGIOutput> m_psOutput;

        std::auto_ptr<PrimitiveBatch> m_psPrimitiveBatch;

        std::list<boost::weak_ptr<DX11Window> > all_windows;
    };

}

#endif
