// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the RawToAces Project.

#include "MatrixNode.h"
#include <Imath/ImathMatrix.h>

struct MatrixNode::Impl
{
    Impl (){}
    Impl (const float matrix[3][3]) :
        _matrix (matrix)
    {
    }
     
    
    ~Impl() {}

    Imath::M33f _matrix;
};


MatrixNode::MatrixNode( std::shared_ptr<Node> input, const float matrix[3][3] ):
    Node ( input),
    _impl (std::make_unique<Impl> (matrix))
{
}

MatrixNode::~MatrixNode ()
{
}

void MatrixNode::processLines (size_t startLine, size_t endLine, const float * buffer)
{
    size_t width = imageInfo().buffer_width;
    size_t chans = imageInfo().buffer_channels;
    size_t stride = width * chans;
    
    _input->processLines (startLine, endLine, buffer);
    
    for (size_t line = startLine; line <= endLine; line++)
    {
        float * p = (float *)buffer + stride * (line - startLine);
        Imath::V3f * v;
        for (size_t x = 0; x < width; x++)
        {
            v = (Imath::V3f *) (p + x * chans);
            
#if defined(DEBUG_PIXEL_X) && defined(DEBUG_PIXEL_Y)
            if (line == DEBUG_PIXEL_Y && x == DEBUG_PIXEL_X)
                std::cerr << "Matrix node: " << *v << " -> ";
#endif // defined(DEBUG_PIXEL_X) && defined(DEBUG_PIXEL_Y)
            
            *v *= _impl->_matrix;
            
#if defined(DEBUG_PIXEL_X) && defined(DEBUG_PIXEL_Y)
            if (line == DEBUG_PIXEL_Y && x == DEBUG_PIXEL_X)
                std::cerr << *v << std::endl;
#endif // defined(DEBUG_PIXEL_X) && defined(DEBUG_PIXEL_Y)
        }
    }
}
