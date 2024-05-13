/*
 * FILE:
 *   load_dx11_dlls.hpp
 *
 * PURPOSE:
 *   Routines to dynamically load DirectX 11 DLLs using OpenLibrary
 *   and GetProcAddress.
 *
 * AUTHOR:
 *   Stephen Thompson <stephen@solarflare.org.uk>
 *
 * CREATED:
 *   05-Jul-2012
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

#ifndef COERCRI_LOAD_DX11_DLLS_HPP
#define COERCRI_LOAD_DX11_DLLS_HPP

#include <d3d11.h>

namespace Coercri {

    // Returns true if successfully loaded, or false if the required
    // version of DirectX is not available on this system.

    // Calling this multiple times is harmless.

    // NOTE: currently there is no way to specify which functions, or
    // which "parts" of directx to load. We just load everything that
    // Coercri/DX11 needs.
    
    bool LoadDX11();


    // Wrappers around dynamically linked DirectX functions.

    // These will throw if LoadDX11 wasn't called first.

    HRESULT D3D11CreateDevice_Wrapper(IDXGIAdapter* pAdapter,
                                      D3D_DRIVER_TYPE DriverType,
                                      HMODULE Software,
                                      UINT Flags,
                                      const D3D_FEATURE_LEVEL *pFeatureLevels,
                                      UINT FeatureLevels,
                                      UINT SDKVersion,
                                      ID3D11Device **ppDevice,
                                      D3D_FEATURE_LEVEL *pFeatureLevel,
                                      ID3D11DeviceContext **ppImmediateContext);
}

#endif

   
