/*
 * FILE:
 *   primitive_batch_ps_tex.hlsl
 *
 * AUTHOR:
 *   Stephen Thompson <stephen@solarflare.org.uk>
 *
 * CREATED:
 *   04-Mar-2014
 *
 * COPYRIGHT:
 *   Copyright (C) Stephen Thompson 2014.
 *
 *   This file is part of the "Coercri" software library. Usage of
 *   "Coercri" is permitted under the terms of the Boost Software
 *   Licence, version 1.0. 
 * 
 */

#include "primitive_batch_common.hlsl"

float4 PS_Tex( PS_INPUT input ) : SV_TARGET
{
    return myTexture.Sample(mySampler, float2(input.Col.x, input.Col.y));
}
