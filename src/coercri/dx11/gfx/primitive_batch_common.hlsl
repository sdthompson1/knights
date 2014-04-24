/*
 * FILE:
 *   primitive_batch_common.hlsl
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

Texture2D myTexture : register( t0 );
SamplerState mySampler : register( s0 );

cbuffer MyConstBuffer : register( b0 )
{
    int width;
    int height;
};

struct VS_INPUT
{
    float2 Pos : POSITION;
    unorm float4 Col : COLOR;
};

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float4 Col : COLOR;
};

