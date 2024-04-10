// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the RawToAces Project.

#include "OLFDemosaicNode.h"

OLFDemosaicNode::OLFDemosaicNode( std::shared_ptr<Node> input ) :
    Node ( input ),
    _imageInfo(input->imageInfo())
{
    _imageInfo.buffer_channels = _imageInfo.image_channels;
}

const ImageInfo & OLFDemosaicNode::imageInfo()
{
    return _imageInfo;
}

void OLFDemosaicNode::validate (size_t startLine, size_t endLine)
{
    // OLF demosaicing requires two lines above and below the target line.
    if (startLine > 1) startLine -= 2;
    else startLine = 0;
    
    if (endLine < imageInfo().buffer_height - 2) endLine += 2;
    else endLine = imageInfo(). buffer_height - 1;
    
    _input->validate (startLine, endLine);
}

// clang-format off
struct Diamond
{
    float
                         v_m2_0,
                v_m1_m1, v_m1_0, v_m1_p1,
        v_0_m2, v_0_m1,  v_0_0,  v_0_p1,  v_0_p2,
                v_p1_m1, v_p1_0, v_p1_p1,
                         v_p2_0;
};
// clang-format on
    
inline void build_diamond(
    size_t x,
    size_t width,
    float * lines[],
    Diamond & diamond
)
{
    size_t xm1 = (x == 0) ? 1 : (x - 1);
    size_t xm2 = (x > 1) ? (x-2) : (x + 2);
    size_t xp1 = (x == (width-1)) ? (width-2) : (x + 1);
    size_t xp2 = (x < (width - 2)) ? (x+2) : (x-2);
    
    diamond.v_m2_0 = lines[0][x];
    diamond.v_m1_m1 = lines[1][xm1];
    diamond.v_m1_0 = lines[1][x];
    diamond.v_m1_p1 = lines[1][xp1];
    diamond.v_0_m2 = lines[2][xm2];
    diamond.v_0_m1 = lines[2][xm1];
    diamond.v_0_0 = lines[2][x];
    diamond.v_0_p1 = lines[2][xp1];
    diamond.v_0_p2 = lines[2][xp2];
    diamond.v_p1_m1 = lines[3][xm1];
    diamond.v_p1_0 = lines[3][x];
    diamond.v_p1_p1 = lines[3][xp1];
    diamond.v_p2_0 = lines[4][x];
}

inline void mix1(
    const Diamond & d, 
    float & out_mix1,
    float & out_mix2
)
{
    out_mix1 = ( 4.F * d.v_0_0 + 2.F * ( d.v_m1_0 + d.v_p1_0 + d.v_0_m1 + d.v_0_p1 ) - ( d.v_m2_0 + d.v_p2_0 + d.v_0_m2 + d.v_0_p2 ) ) / 8.F;
    out_mix2 = (6.F * d.v_0_0 + 2.F * (d.v_m1_m1 + d.v_m1_p1 + d.v_p1_m1 + d.v_p1_p1 ) - 1.5F * ( d.v_m2_0 + d.v_p2_0 + d.v_0_m2 + d.v_0_p2 ) ) / 8.F;
    
    // handle really strong edges (not in original paper)
    float alt_mix1 = 0.25F * ( d.v_m1_0 + d.v_p1_0 + d.v_0_m1 + d.v_0_p1 );
    float alt_mix2 = 0.25F * ( d.v_m1_m1 + d.v_m1_p1 + d.v_p1_m1 + d.v_p1_p1 );
    
    if ( out_mix1 < 0.F || (out_mix1 > 0.F && (alt_mix1/out_mix1) > 1.5F) )
        out_mix1 = alt_mix1;
    
    if ( out_mix2 < 0.F || (out_mix2 > 0.F && (alt_mix2/out_mix2) > 1.5F) )
        out_mix2 = alt_mix2;
}
    
inline void mix2(
    const Diamond & d, 
    float & out_mix1,
    float & out_mix2
)
{
    out_mix1 = ( 5.F * d.v_0_0 + 4.F * ( d.v_0_m1 + d.v_0_p1) - ( d.v_m1_m1 + d.v_m1_p1 + d.v_0_m2 + d.v_0_p2 + d.v_p1_m1 + d.v_p1_p1 ) + 0.5F * ( d.v_m2_0 + d.v_p2_0 ) ) / 8.F;
    out_mix2 = ( 5.F * d.v_0_0 + 4.F * (d.v_m1_0 + d.v_p1_0 ) - ( d.v_m1_m1 + d.v_m1_p1 + d.v_m2_0 + d.v_p2_0 + d.v_p1_m1 + d.v_p1_p1 ) + 0.5F * ( d.v_0_m2 + d.v_0_p2 ) ) / 8.F;
    
    // REALLY strong edge, just fall back to linear... the original paper did not have this...
    float alt_mix1 = 0.5F * ( d.v_0_m1 + d.v_0_p1 );
    float alt_mix2 = 0.5F * ( d.v_m1_0 + d.v_p1_0 );
    
    if ( out_mix1 < 0.F || ( out_mix1 > 0.F && (alt_mix1/out_mix1) > 1.5F) )
        out_mix1 = alt_mix1;
    if ( out_mix2 < 0.F || ( out_mix2 > 0.F && (alt_mix2/out_mix2) > 1.5F) )
        out_mix2 = alt_mix2;
}

inline void calcRed(
    size_t x,
    size_t width,
    float * lines[],
    Diamond & diamond,
    float * & out
)
{
    float val1, val2;
    build_diamond(x, width, lines, diamond);
    mix1(diamond, val1, val2);

    *out++ = diamond.v_0_0;
    *out++ = val1;
    *out++ = val2;
}

inline void calcBlue (
    size_t x,
    size_t width,
    float * lines[],
    Diamond & diamond,
    float * & out
)
{
    float val1, val2;
    build_diamond (x, width, lines, diamond);
    mix1 (diamond, val1, val2);
    
    *out++ = val2;
    *out++ = val1;
    *out++ = diamond.v_0_0;
}

inline void calcGreenInRed (
    size_t x,
    size_t width,
    float * lines[],
    Diamond & diamond,
    float * & out
)
{
    float val1, val2;
    build_diamond(x, width, lines, diamond);
    mix2 (diamond, val1, val2);
    
    *out++ = val1;
    *out++ = diamond.v_0_0;
    *out++ = val2;
}

inline void calcGreenInBlue(
    size_t x,
    size_t width,
    float * lines[],
    Diamond & diamond,
    float * & out
)
{
    float val1, val2;
    build_diamond(x, width, lines, diamond);
    mix2 (diamond, val1, val2);
    
    *out++ = val2;
    *out++ = diamond.v_0_0;
    *out++ = val1;
}
    
void OLFDemosaicNode::processLines (size_t startLine, size_t endLine, const float * buffer)
{
    size_t width = imageInfo() .buffer_width;
    
    FetchHelper fetch(_input, 2, startLine);
    
    for (size_t line = startLine; line <= endLine; line++ )
    {
        fetch.step();
        
        //GRGBGR
        // BGBGBG
        // GRGBGR
        
        int ho = 0; // Horizontal offset
        int vo = 0; // Vertical offset
        
        if (_imageInfo.pattern == "GRBG")
        {
            ho = 0;
            vo = 0;
        }
        else if (_imageInfo.pattern == "RGGB" )
        {
            ho = 1;
            vo = 0;
        }
        else if (_imageInfo.pattern == "BGGR")
        {
            ho = 0;
            vo = 1;
        }
        else if (_imageInfo.pattern == "GBRG")
        {
            ho = 1;
            vo = 1;
        }
        
        if (line & 1) vo = 1 - vo;
        
        float * lines[5];
        for (size_t i = 0; i < 5; i++) lines[i] = fetch[i];
        
        float * p = (float *)buffer;
        
        Diamond diamond;
        
        float val1, val2;
        
        size_t x;
        
        //ho = 1 - ho;
        
        if (ho == 0)
        {
            if (vo == 0)
            {
                // GR<
                // BG
                for (x = 0; x < imageInfo(). buffer_width - 2;)
                {
                    calcGreenInRed(x++, width, lines, diamond, p);
                    calcRed(x++, width, lines, diamond, p);
                }
                
                if (x < imageInfo(). buffer_width - 1)
                    calcGreenInRed(x, width, lines, diamond, p);
            }
            else
            {
                // BG <
                // GR
                for (x = 0; x < imageInfo(). buffer_width - 2;)
                {
                    calcBlue(x++, width, lines, diamond, p);
                    calcGreenInBlue (x++, width, lines, diamond, p);
                }
                
                if (x < imageInfo(). buffer_width - 1)
                    calcBlue(x, width, lines, diamond, p);
            }
        }
        else
        {
            if (vo == 0)
            {
                // RG <
                // GB
                for (x = 0; x < imageInfo().buffer_width - 2;)
                {
                    calcRed(x++, width, lines, diamond, p);
                    calcGreenInRed(x++, width, lines, diamond, p);
                }
                if (x < imageInfo(). buffer_width - 1)
                    calcRed(x, width, lines, diamond, p);
            }
            else
            {
                // GB <
                // RG
                for (x = 0; x < imageInfo(). buffer_width - 2;)
                {
                    calcGreenInBlue (x++, width, lines, diamond, p);
                    calcBlue(x++, width, lines, diamond, p);
                }
                if (x < imageInfo(). buffer_width - 1)
                    calcGreenInBlue (x, width, lines, diamond, p);
            }
        }
    }
}
