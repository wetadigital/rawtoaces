// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the RawToAces Project.

#include "OpenEXRWriteNode.h"

#include <vector>
#include <OpenEXR/ImfAcesFile.h>
#include <OpenEXR/ImfArray.h>

#include <Imath/ImathBox.h>

struct OpenEXRWriteNode::Impl
{
    Impl (const std::string filename, const ImageInfo & info) :
    file(
        filename. c_str(),
        Imath::Box2i(Imath::V2i(0, 0), Imath::V2i((int)info.image_width - 1, (int)info.image_height - 1)),
        Imath::Box2i(Imath::V2i(-(int)info.left_offset, -(int)info.top_offset), Imath::V2i((int)info.buffer_width - 1 - (int)info.left_offset, (int)info.buffer_height - 1 - (int)info.top_offset)),
        info.buffer_channels == 4? Imf::WRITE_RGBA : Imf::WRITE_RGB,
        1.0f,
        Imath::V2f(0.0f, 0.0f),
        1,
        Imf::INCREASING_Y, Imf::NO_COMPRESSION
    )
    {
        auto display = Imath:: Box2i(Imath::V2i(0, 0), Imath::V2i((int)info.image_width - 1, (int)info. image_height - 1));
        auto data = Imath::Box2i(Imath::V2i(-(int)info.left_offset, -(int)info. top_offset), Imath::V2i((int)info.buffer_width - 1 - (int)info. left_offset, (int)info. buffer_height - 1-(int)info. top_offset));
        int x=9;
        x++;
    }
    
    Imf::AcesOutputFile file;
    
    ~Impl() {}
};

OpenEXRWriteNode::OpenEXRWriteNode( std::shared_ptr<Node> input, std::string path ) :
    WriteNode( input ),
    _impl(std::make_unique<Impl> (path, imageInfo()))
{
}

OpenEXRWriteNode::~OpenEXRWriteNode ()
{
}

void OpenEXRWriteNode::validateAll()
{
    if (_input)
    {
        for (size_t y = 0; y < imageInfo().buffer_height; y++ )
            _input->validate(y, y);
    }
}

void OpenEXRWriteNode::writeFile()
{
    const ImageInfo & info = imageInfo();
    
    std::vector<float> buffer1;
    buffer1.resize( info.buffer_width * info.image_channels );
    
    std::vector<Imf::Rgba> buffer2;
    buffer2.resize( info.buffer_width);
    
    Imf::Rgba * p = buffer2.data() + info.left_offset;
    
    _impl->file.setFrameBuffer (p, 1, 0);
    
    for ( size_t y = 0; y < info.buffer_height; y++ )
    {
        float * p1 = buffer1. data();
        _input->processLines (y, y, p1);
        
        Imf::Rgba * p2 = buffer2. data();
        
        if (info.image_channels == 3)
        {
            for (size_t x= 0; x < info.buffer_width; x++)
            {
                p2->r = *p1++;
                p2->g = *p1++;
                p2->b = *p1++;
                p2++;
            }
        }
        else
        {
            for (size_t x = 0; x < info.buffer_width; x++)
            {
                p2->r = *p1++;
                p2->g = *p1++;
                p2->b = *p1++;
                p2->a = *p1++;
                p2++;
            }
        }
        
        _impl->file.writePixels (1);
    }
}
