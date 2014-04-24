/*
 * FILE:
 *   primitive_batch_ps.hlsl
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

// pixel shader --> just passes through the given colour
float4 PS( PS_INPUT input ) : SV_TARGET
{
    return input.Col;
}
