// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the RawToAces Project.

#include "BilinearDemosaicNode.h"

#include <iostream>

BilinearDemosaicNode::BilinearDemosaicNode( std::shared_ptr<Node> input ) :
    Node ( input ),
    _imageInfo(input->imageInfo())
{
    _imageInfo.buffer_channels = _imageInfo.image_channels;
}

const ImageInfo & BilinearDemosaicNode::imageInfo()
{
    return _imageInfo;
}

void BilinearDemosaicNode::validate (size_t startLine, size_t endLine)
{
    // Bilinear demosaicing requires one line above and below the target line.
    if (startLine > 0) startLine--;
    if (endLine < imageInfo(). buffer_height - 1) endLine++;
    
    _input->validate (startLine, endLine);
}

void BilinearDemosaicNode::processLines (size_t startLine, size_t endLine, const float * buffer)
{
    FetchHelper fetch(_input, 1, startLine);
    
    for (size_t line = startLine; line <= endLine; line++ )
    {
        fetch.step();
        
        // GRGBGR
        // BGBGBG
        // GRGBGR
        
        int ho = 0; // Horizontal offset
        int vo = 0; // Vertical offset
        
        if (_imageInfo.pattern == "GRBG")
        {
            ho = 0;
            vo = 0;
        }
        else if (_imageInfo.pattern == "RGGB")
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
        
        float * ll1 = fetch[0];
        float * ll2 = fetch[1];
        float * ll3 = fetch[2];
        
        float * p = (float *)buffer;
        p+= 3;
        
        ho = 1 - ho;
        
        if (ho == 0)
        {
            if (vo == 0)
            {
                // GR <
                // BG
                for (size_t x = 1; x < imageInfo(). buffer_width - 1; x += 2)
                {
                    *p++ = (ll2[x - 1] + ll2[x + 1]) / 2.0; // -
                    *p++ = ll2[x];                          // .
                    *p++ = (ll1[x] + ll3[x]) / 2.0;         // |
                    
                    *p++ = ll2[x + 1];                                              // .
                    *p++ = (ll2[x] + ll2[x + 2] + ll1[x + 1] + ll3[x + 1]) / 4.0;   // +
                    *p++ = (ll1[x] + ll1[x + 2] + ll3[x] + ll3[x + 2]) / 4.0;       // x
                }
            }
            else
            {
                // BG <
                // GR
                for (size_t x = 1; x < imageInfo(). buffer_width - 1; x += 2)
                {
                    *p++ = (ll1[x-1] + ll1[x + 1] + ll3[x - 1] + ll3[x + 1]) / 4.0; // x
                    *p++ = (ll2[x-1] + ll2[x + 1] + ll1[x] + ll3[x]) / 4.0;         // +
                    *p++ = ll2[x];                                                  // .
                    
                    *p++ = (ll1[x + 1] + ll3[x + 1]) / 2.0;                         // |
                    *p++ = ll2[x+1];                                                // .
                    *p++ = (ll2[x] + ll2[x + 2]) / 2.0;                             // -
                }
            }
        }
        else
        {
            if (vo == 0)
            {
                // RG <
                // GB
                for (size_t x = 1; x < imageInfo(). buffer_width - 1; x += 2)
                {
                    *p++ = ll2[x];                                                  // .
                    *p++ = (ll2[x-1] + ll2[x + 1] + ll1[x] + ll3[x]) / 4.0;         // +
                    *p++ = (ll1[x-1] + ll1[x + 1] + ll3[x - 1] + ll3[x + 1]) / 4.0; // x
                    
                    *p++ = (ll2[x] + ll2[x + 2]) / 2.0;         // -
                    *p++ = ll2[x+1];                            // .
                    *p++ = (ll1[x + 1] + ll3[x + 1]) / 2.0;     // |
                }
            }
            else
            {
                // GB <
                // RG
                for (size_t x = 1; x < imageInfo().buffer_width - 1; x += 2)
                {
                    *p++ = (ll1[x] + ll3[x]) / 2.0;             // |
                    *p++ = ll2[x];                              // .
                    *p++ = (ll2[x - 1] + ll2[x + 1]) / 2.0;     // -
                    
                    *p++ = (ll1[x] + ll1[x + 2] + ll3[x] + ll3[x + 2]) / 4.0;        // x
                    *p++ = (ll2[x] + ll2[x + 2] + ll1[x + 1] + ll3[x + 1]) / 4.0;   // +
                    *p++ = ll2[x+1];                                                // .
                }
            }
        }
        
#if defined(DEBUG_PIXEL_X) && defined(DEBUG_PIXEL_Y)
        if (line == DEBUG_PIXEL_Y)
        {
            float * p = (float *)buffer + DEBUG_PIXEL_X * 3;
            std::cerr << "Bilinear demosaic node: (" << p[0] << " " << p[1] << " " << p[2] << ")" << std::endl;
        }
#endif // defined(DEBUG_PIXEL_X) && defined(DEBUG_PIXEL_Y)
    }
}
 
