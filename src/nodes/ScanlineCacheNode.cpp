// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the RawToAces Project.

#include "ScanlineCacheNode.h"
#include <iostream>

ScanlineCacheNode::ScanlineCacheNode( std::shared_ptr<Node> input ):
    Node (input),
    _counters(imageInfo().buffer_height),
    _cache(imageInfo().buffer_height)
{
}

ScanlineCacheNode::~ScanlineCacheNode () {}


void ScanlineCacheNode::validate(size_t startLine, size_t endLine)
{
    for (size_t i = startLine; i < endLine; i++)
        _counters[i]++;
}

void ScanlineCacheNode::processLines (size_t startLine, size_t endLine, const float * buffer)
{
    // TODO: add thread-safety
    
    //    std::cerr << "Request " << startLine < " - " << endLine << std::endl;
    
    size_t top = startLine;
    size_t stride = imageInfo().buffer_width * imageInfo().buffer_channels;
    
    for (size_t line = startLine; line <= endLine + 1; line++)
    {
        if ((line == endLine + 1) || _cache[line].size() == stride)
        {
            if (top < line)
            {
                float * dst1 = (float *)buffer + stride * (top - startLine);
                _input->processLines (top, line - 1, dst1);
                
                for (size_t i = top; i < line; i++)
                {
                    if (_counters[i] > 1)
                    {
                        //                        std::cerr << "Store " << line << std::endl;
                        _cache[i]. resize( stride);
                        
                        float * src2 = dst1 + stride * (i - top);
                        
                        float * dst2 = _cache[i].data ();
                        memcpy (dst2, src2, stride * sizeof(float));
                    }
                }
            }
            
            if (line <= endLine)
            {
                //                std::cerr << "Found " << line << std::endl;
                
                float * src = _cache[line].data();
                float * dst = (float *)buffer + stride * (line - startLine);
                
                memcpy (dst, src, stride * sizeof (float));
                top = line + 1;
            }
        }
    }
    
    for (size_t line = startLine; line <= endLine; line++)
    {
        if (_counters [line] > 0)
        {
            _counters[line]--;
        }
        
        if (_counters[line] == 0)
        {
//            std::cerr << "Clear " << line << std::endl;
            
            if (_cache[line].size() > 0)
            {
                _cache[line]. resize (0) ;
                _cache[line]. shrink_to_fit();
            }
        }
    }
}
