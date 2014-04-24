/*
 * FILE:
 *   primitive_batch_vs.hlsl
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

// vertex shader --> transforms from pixel coordinates to normalized
// device coordinates
PS_INPUT VS( VS_INPUT input )
{
    PS_INPUT output;
    output.Pos = float4( ( 2 * input.Pos.x - width)  / width,   
                         (-2 * input.Pos.y + height) / height,  
                         0.5f,   
                         1.0f ); 
    output.Col = input.Col;
    return output;
}
