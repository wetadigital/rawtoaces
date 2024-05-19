// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the RawToAces Project.

#include "OpenEXRWriteNode.h"

#include <vector>
#include <OpenEXR/ImfAcesFile.h>
#include <OpenEXR/ImfArray.h>

#include <Imath/ImathBox.h>

Imath::Box2i displayWindow(const ImageInfo & info, Options::CropMode cropMode)
{
    switch (cropMode) {
        case Options::CropMode::None:
            return Imath::Box2i(Imath::V2i(0, 0), Imath::V2i((int)info.buffer_width - 1, (int)info.buffer_height - 1));
        case Options::CropMode::Soft:
        case Options::CropMode::Hard:
            return Imath::Box2i(Imath::V2i(0, 0), Imath::V2i((int)info.image_width - 1, (int)info.image_height - 1));
    }
}

Imath::Box2i dataWindow(const ImageInfo & info, Options::CropMode cropMode)
{
    switch (cropMode) {
        case Options::CropMode::None:
            return Imath::Box2i(Imath::V2i(0, 0), Imath::V2i((int)info.buffer_width - 1, (int)info.buffer_height - 1));
        case Options::CropMode::Soft:
            return Imath::Box2i(Imath::V2i(-(int)info.left_offset, -(int)info.top_offset), Imath::V2i((int)info.buffer_width - 1 - (int)info.left_offset, (int)info.buffer_height - 1 - (int)info.top_offset));
        case Options::CropMode::Hard:
            return Imath::Box2i(Imath::V2i(0, 0), Imath::V2i((int)info.image_width - 1, (int)info.image_height - 1));
    }
}

struct OpenEXRWriteNode::Impl
{
    Impl (const std::string filename, const ImageInfo & info, Options::CropMode cropMode) :
    file(
        filename. c_str(),
        displayWindow(info, cropMode),
        dataWindow(info, cropMode),
         info.buffer_channels == 4? Imf::WRITE_RGBA : info.buffer_channels == 3 ? Imf::WRITE_RGB : Imf::WRITE_Y,
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

OpenEXRWriteNode::OpenEXRWriteNode( std::shared_ptr<Node> input, Options::CropMode cropMode, std::string path) :
    WriteNode(input, cropMode),
    _impl(std::make_unique<Impl> (path, imageInfo(), cropMode))
{
}

OpenEXRWriteNode::OpenEXRWriteNode( std::shared_ptr<Node> input, std::string path ) :
    WriteNode(input, Options::CropMode::Soft),
    _impl(std::make_unique<Impl> (path, imageInfo(), Options::CropMode::Soft))
{
}

OpenEXRWriteNode::~OpenEXRWriteNode ()
{
}

void OpenEXRWriteNode::validateAll()
{
    if (_input)
    {
        const ImageInfo & info = imageInfo();
        size_t first_y, last_y;
        
        switch (_cropMode) {
            case Options::CropMode::None:
            case Options::CropMode::Soft:
                first_y = 0;
                last_y = info.buffer_height - 1;
                break;
            case Options::CropMode::Hard:
                first_y = info.top_offset;
                last_y = info.top_offset + info.image_height - 1;
                break;
        }
        
        for (size_t y = first_y; y <= last_y; y++ )
            _input->validate(y, y);
    }
}

void OpenEXRWriteNode::writeFile()
{
    const ImageInfo & info = imageInfo();
    
    size_t first_y, last_y;
    size_t in_offset, out_offset, count;
    
    switch (_cropMode) {
        case Options::CropMode::None:
            first_y = 0;
            last_y = info.buffer_height - 1;
            in_offset = 0;
            out_offset = 0;
            count = info.buffer_width;
            break;
        case Options::CropMode::Soft:
            first_y = 0;
            last_y = info.buffer_height - 1;
            in_offset = 0;
            out_offset = info.left_offset;
            count = info.buffer_width;
            break;
        case Options::CropMode::Hard:
            first_y = info.top_offset;
            last_y = info.top_offset + info.image_height - 1;
            in_offset = info.left_offset * info.buffer_channels;
            out_offset = 0;
            count = info.image_width;
            break;
    }
    
    std::vector<float> buffer1;
    buffer1.resize( info.buffer_width * info.buffer_channels );
    
    std::vector<Imf::Rgba> buffer2;
    buffer2.resize(count);
    
    Imf::Rgba * p = buffer2.data() + out_offset;
    
    _impl->file.setFrameBuffer (p, 1, 0);
    
    for ( size_t y = first_y; y <= last_y; y++ )
    {
        float * p1 = buffer1.data();
        _input->processLines (y, y, p1);
        p1 += in_offset;
        
        Imf::Rgba * p2 = buffer2.data();
        
        if (info.buffer_channels == 3)
        {
            for (size_t x = 0; x < count; x++)
            {
                p2->r = *p1++;
                p2->g = *p1++;
                p2->b = *p1++;
                p2++;
            }
        }
        else if (info.buffer_channels == 4)
        {
            for (size_t x = 0; x < count; x++)
            {
                p2->r = *p1++;
                p2->g = *p1++;
                p2->b = *p1++;
                p2->a = *p1++;
                p2++;
            }
        }
        else
        {
            for (size_t x = 0; x < count; x++)
            {
                p2->r = *p1++;
                p2->g = p2->r;
                p2->b = p2->r;
                p2->a = 1.0f;
                p2++;
            }
        }
        
        _impl->file.writePixels (1);
    }
}
