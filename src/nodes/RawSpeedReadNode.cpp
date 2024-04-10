// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the RawToAces Project.

#include "RawSpeedReadNode.h"

#if USE_RAWSPEED

#include <RawSpeed-API.h>
#include <TiffIFD.h>
#include <AbstractTiffDecoder.h>
#include <iostream>
#include <memory>

using namespace rawspeed;

struct RawSpeedReadNode::Impl
{
    ~Impl(){}
    std::unique_ptr<RawDecoder> decoder;
    
    float scale;
    ImageInfo imageInfo;
};

//TiffIFD * findMainIFD(TiffIFD * rootIFD, size_t prev)
//{
////    getif
//    
//    TiffIFD * bestIFD = nullptr;
//    size_t bestBitDepth = prev;
//    
//    
//}
    
size_t getBitsPerSample (TiffIFD * ifd, size_t prev)
{
    size_t result = prev;
    
    const TiffTag tags[3] = {
        TiffTag::BITSPERSAMPLE,
        TiffTag::PANASONIC_BITSPERSAMPLE,
        TiffTag::FUJI_BITSPERSAMPLE
    };
    
    for (size_t i = 0; i < 3; i++)
    {
        if (ifd->hasEntry (tags[i]))
        {
            size_t r = ifd->getEntry(tags[i])->getU32();
            if (r > result) result = r;
        }
    }
    
    for (size_t i = 0; i < ifd->getSubIFDs().size(); i++)
    {
        TiffIFD * s = ifd->getSubIFDs()[i].get();
        size_t r = getBitsPerSample(s, prev);
        if (r > result) result = r;
    }
    
    return result;
}


RawSpeedReadNode::RawSpeedReadNode ( std::string filename ) :
    Node (nullptr),
    _impl(std::make_unique<Impl> ())
{
    
    std::string data_path = getenv( "RAWSPEED_CAMERA_DATABASE" );
    
    std::unique_ptr<CameraMetaData> metadata;
    metadata.reset (new CameraMetaData(data_path));
    
    FileReader f(filename.c_str());
    auto map = f.readFile();
    
    RawParser parser (map. second);
    
    _impl->decoder = parser.getDecoder ();
    _impl->decoder->failOnUnknown = true;
    _impl->decoder->checkSupport (metadata. get ());
    
    /* Decode the RAW data and crop to the active image area */
    _impl->decoder->decodeRaw();
    _impl->decoder->decodeMetaData (metadata. get ());
    float minimum = _impl->decoder->mRaw->blackLevelSeparate[0];
    
        size_t bps = getBitsPerSample(_impl->decoder->getRootIFD(), 0);
    //    float maximum = (1u << bps) - 1;
    
    float maximum = _impl->decoder->mRaw->whitePoint;
    //    float maximum = (1u << 16) - 1;
    
    _impl->scale = 1.0 / maximum;
    std::cerr << "Rawspeed " << filename << "max=" << maximum << "min=" << minimum << std::endl;
    
    iPoint2D imageSize = _impl->decoder->mRaw->dim;
    iPoint2D bufferSize = _impl->decoder->mRaw->getUncroppedDim() ;
    iPoint2D offset = _impl->decoder->mRaw->getCropOffset () ;
    _impl->imageInfo.buffer_width = bufferSize.x;
    _impl->imageInfo.buffer_height = bufferSize.y;
    _impl->imageInfo.buffer_channels = 1;
    _impl->imageInfo.image_width = imageSize.x;
    _impl->imageInfo.image_height = imageSize.y;
    _impl->imageInfo.image_channels = 3;
    _impl->imageInfo.left_offset = offset.x;
    _impl->imageInfo.top_offset = offset.y;
    
    _impl->imageInfo.pattern = "    ";
    for (int y = 0; y < 2; y++)
    {
        for (int x = 0; x < 2; x++)
        {
            CFAColor color = _impl->decoder->mRaw->cfa.getColorAt(x, y);
            
            switch (color) {
                case CFAColor::RED:
                    _impl->imageInfo.pattern[y * 2 + x] = 'R';
                    break;
                case CFAColor::GREEN:
                    _impl->imageInfo.pattern[y * 2 + x] = 'G';
                    break;
                case CFAColor::BLUE:
                    _impl->imageInfo.pattern[y * 2 + x] = 'B';
                    break;
                default:
                    break;
            }
        }
    }
    
    _impl->imageInfo.metadata.neutralRGB.push_back(_impl->decoder->mRaw->metadata.wbCoeffs[0]);
    _impl->imageInfo.metadata.neutralRGB.push_back(_impl->decoder->mRaw->metadata.wbCoeffs[1]);
    _impl->imageInfo.metadata.neutralRGB.push_back(_impl->decoder->mRaw->metadata.wbCoeffs[2]);
    
    _impl->imageInfo.metadata.camera_make = _impl->decoder->mRaw->metadata.make;
    _impl->imageInfo.metadata.camera_model = _impl->decoder->mRaw->metadata.model;
    _impl->imageInfo.metadata.iso = _impl->decoder->mRaw->metadata.isoSpeed;
    
    
    rawspeed::AbstractTiffDecoder * tiffDecoder = dynamic_cast<rawspeed::AbstractTiffDecoder *>(_impl->decoder.get());
    
    if (tiffDecoder)
    {
        uint32_t bps = 0;
        
        const TiffIFD * ifd = tiffDecoder->getIFDWithLargestImage();
        
        TiffEntry * e = ifd->getEntryRecursive(TiffTag::BITSPERSAMPLE);
        if (e)
        {
            bps = e->getU32();
        }
        else
        {
            e = ifd->getEntryRecursive(TiffTag::PANASONIC_BITSPERSAMPLE);
            if (e)
            {
                bps = e->getU32();
            }
            else
            {
                e = ifd->getEntryRecursive(TiffTag::FUJI_BITSPERSAMPLE);
                if (e)
                {
                    bps = e->getU32();
                }
            }
        }
        
        if (bps > 0)
        {
            
        }
        
        e = ifd->getEntryRecursive(TiffTag::CALIBRATIONILLUMINANT1);
        if (e)
        {
            uint16_t i = e->getU16();
            _impl->imageInfo.metadata.calibrateIllum.push_back(i);
        }
        
        e = ifd->getEntryRecursive(TiffTag::CALIBRATIONILLUMINANT2);
        if (e)
        {
            uint16_t i = e->getU16();
            _impl->imageInfo.metadata.calibrateIllum.push_back(i);
        }
        
        e = ifd->getEntryRecursive(TiffTag::COLORMATRIX1);
        if (e)
        {
            const std::vector<NotARational<int32_t>> vec = e->getSRationalArray(e->count);
            
            for (size_t i = 0; i < e->count; i++)
            {
                double val = (double)vec[i].num / (double)vec[i].den;
                _impl->imageInfo.metadata.xyz2rgbMatrix1.push_back(val);
            }
        }
        
        e = ifd->getEntryRecursive(TiffTag::COLORMATRIX2);
        if (e)
        {
            const std::vector<NotARational<int32_t>> vec = e->getSRationalArray(e->count);
            
            for (size_t i = 0; i < e->count; i++)
            {
                double val = (double)vec[i].num / (double)vec[i].den;
                _impl->imageInfo.metadata.xyz2rgbMatrix2.push_back(val);
            }
        }
        
        
        
        
        ifd = tiffDecoder->getRootIFD();
        
        e = ifd->getEntryRecursive(TiffTag::MAKE);
        if (e)
        {
            _impl->imageInfo.metadata.camera_make = e->getString();
        }
        
        e = ifd->getEntryRecursive(TiffTag::MODEL);
        if (e)
        {
            _impl->imageInfo.metadata.camera_model = e->getString();
        }
        
        e = ifd->getEntryRecursive(TiffTag::ISOSPEEDRATINGS);
        if (e)
        {
            _impl->imageInfo.metadata.iso = e->getU32();
        }
        else
        {
            e = ifd->getEntryRecursive(TiffTag::PANASONIC_ISO_SPEED);
            if (e)
            {
                _impl->imageInfo.metadata.iso = e->getU32();
            }
        }
        
        e = ifd->getEntryRecursive(TiffTag::APERTUREVALUE);
        if (e)
        {
            _impl->imageInfo.metadata.aperture = e->getU32();
        }
        
        e = ifd->getEntryRecursive(TiffTag::SHUTTERSPEEDVALUE);
        if (e)
        {
            _impl->imageInfo.metadata.shutter = e->getU32();
        }
    }
    
    
    
}

RawSpeedReadNode::~RawSpeedReadNode ()
{
}
 
const ImageInfo & RawSpeedReadNode::imageInfo()
{
    return _impl->imageInfo;
}
    
void RawSpeedReadNode::processLines (size_t startLine, size_t endLine, const float * buffer)
{
    size_t width = _impl->imageInfo.buffer_width;
    size_t stride = width * _impl->imageInfo.buffer_channels;
    
    float scale = _impl->scale;
    
    for ( size_t line = startLine; line <= endLine; line++ )
    {
        size_t offset = stride * line;
        
        auto data = _impl->decoder->mRaw->getU16DataAsUncroppedArray2DRef();
        uint16_t * p1 = (uint16_t *) (&data[line](0));
        float * p2 = (float *) buffer + _impl->imageInfo.buffer_width * (line - startLine);
        
        for (size_t x = 0; x < stride; x++)
            *p2++ = *p1++ * scale;
    }
}

#endif // USE_RAWSPEED
