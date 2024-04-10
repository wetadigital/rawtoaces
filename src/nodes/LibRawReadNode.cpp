// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the RawToAces Project.

#if USE_LIBRAW

#include "LibrawReadNode.h"

#include <libraw/libraw.h>
#include <iostream>

struct LibrawReadNode::Impl
{
    ~Impl() {}

    float scale; 
    bool demosaic;
    LibRaw libRaw;
    ImageInfo imageInfo;
};

LibrawReadNode::LibrawReadNode( std::string filename, bool demosaic) :
    Node (nullptr),
    _impl (std::make_unique<Impl> ())
{
    _impl->demosaic = demosaic;
    
    int result = _impl->libRaw.open_file(filename.c_str());
    if (result != LIBRAW_SUCCESS)
    {
        
    }
    
    _impl->imageInfo.image_width = _impl->libRaw.imgdata.sizes.width;
    _impl->imageInfo.image_height = _impl->libRaw.imgdata.sizes.height;
    _impl->imageInfo.image_channels = _impl->libRaw.imgdata.idata.colors;
    
    _impl->imageInfo.buffer_width =  _impl->imageInfo.image_width;
    _impl->imageInfo.buffer_height =_impl->imageInfo.image_height;
    
    float minimum = _impl->libRaw.imgdata.color.black;
    
    float maximum = (1u << 16) - 1;
    //    float maximum = (1u < _impl->libRaw.imgdata.color.raw_bps) - 1;
//        float maximum = _impl->libRaw.imgdata.color.maximum;
    
    _impl->scale = 1.0 / maximum;
    
    std::cerr << std::endl << std::endl << "Libraw " << filename << " max=" << maximum << " min=" << minimum << std::endl;
    
    result = _impl->libRaw.unpack();
    if (result != LIBRAW_SUCCESS)
    {
    }
    
    _impl->libRaw.imgdata.rawparams.options = LIBRAW_RAWOPTIONS_DNG_DISABLEWBADJUST;
    //    _impl->libRaw.imgdata.color.data_maximum = 0;
    
    _impl->libRaw.imgdata.params.output_bps = 16;
    _impl->libRaw.imgdata.params.use_auto_wb = 0;
    _impl->libRaw.imgdata.params.use_camera_wb = 0;
    //    _impl->libRaw.imgdata.params.use_fuji_rotate = 0;
    //    _impl->libRaw.imgdata.params.user_qual = 0;
    //    _impl->libRaw.imgdata.params.user_black = 0;
    //    _impl->libRaw.imgdata.params.no_auto_scale = 1;
    _impl->libRaw.imgdata.params.no_auto_bright = 1;
    _impl->libRaw.imgdata.params.highlight = 0;
    _impl->libRaw.imgdata.params.user_qual = 3;
    _impl->libRaw.imgdata.params.gamm[0] = 1.0;
    _impl->libRaw.imgdata.params.gamm[1] = 1.0;
    

    
    bool processColor = true;
    //    processColor = false;
    if (processColor)
    {
        _impl->libRaw.imgdata.params.output_color = 5;
        _impl->libRaw.imgdata.params.use_camera_matrix = 1;
        _impl->libRaw.imgdata.params.use_camera_wb = 1;
    }
    else
    {
        _impl->libRaw.imgdata.params.output_color = 0;
        _impl->libRaw.imgdata.params.use_camera_matrix = 0;
    }
    
    _impl->imageInfo.image_width = _impl->libRaw.imgdata.sizes.width;
    _impl->imageInfo.image_height = _impl->libRaw.imgdata.sizes.height;
    _impl->imageInfo.image_channels = _impl->libRaw. imgdata.idata.colors;
    
    if (_impl->libRaw.imgdata.rawdata.sizes.raw_inset_crops[0].cwidth > 0)
    {
        _impl->imageInfo.left_offset = _impl->libRaw.imgdata.rawdata.sizes.raw_inset_crops[0].cleft;
        _impl->imageInfo.top_offset = _impl->libRaw.imgdata.rawdata.sizes.raw_inset_crops[0].ctop;
    }
    else
    {
        _impl->imageInfo.left_offset = _impl->libRaw. imgdata.rawdata.sizes.left_margin;
        _impl->imageInfo.top_offset = _impl->libRaw.imgdata.rawdata.sizes.top_margin;
    }
    
    if (_impl->demosaic)
    {
//        std::cerr << "Demosaic pre " << std::endl;
//        
//        std::cerr << "   pre_mul ";
//        float ff = _impl->libRaw.imgdata.color.pre_mul[1];
//        for (size_t i = 0; i < 3; i++ ) std::cerr << _impl->libRaw.imgdata.color.pre_mul[i] / ff << " ";
//        std::cerr << std::endl;
//        
//        std::cerr << "   cam_mul ";
//        ff = _impl->libRaw.imgdata.color.cam_mul[1];
//        for (size_t i = 0; i < 3; i++ ) std::cerr << _impl->libRaw.imgdata.color.cam_mul[i] / ff << " ";
//        std::cerr << std::endl;
        
        
        
        
        
        // Disable cropping margins
        _impl->libRaw.imgdata.rawdata.sizes.width = _impl->libRaw.imgdata.rawdata.sizes.raw_width;
        _impl->libRaw.imgdata.rawdata.sizes.height = _impl->libRaw.imgdata.rawdata.sizes.raw_height;
        _impl->libRaw.imgdata.rawdata.sizes.iwidth = _impl->libRaw.imgdata.rawdata.sizes.raw_width;
        _impl->libRaw.imgdata.rawdata.sizes.iheight = _impl->libRaw.imgdata.rawdata.sizes.raw_height;
        _impl->libRaw.imgdata.rawdata.sizes.left_margin = 0;
        _impl->libRaw.imgdata.rawdata.sizes.top_margin = 0;
        
        
        _impl->libRaw.imgdata.params.user_qual = 0; // TEMPORARY LINEAR INTERPOLATION
        
        result = _impl->libRaw.dcraw_process();
        
//        std::cerr << "Demosaic post " << std::endl;
//        
//        std::cerr << "   pre_mul ";
//        for (size_t i = 0; i < 3; i++ ) std::cerr << _impl->libRaw.imgdata.color.pre_mul[i] << " ";
//        std::cerr << std::endl;
//        
//        std::cerr << "   cam_mul ";
//        for (size_t i = 0; i < 3; i++ ) std::cerr << _impl->libRaw.imgdata.color.cam_mul[i] << " ";
//        std::cerr << std::endl;
        
        
        
        if (result != LIBRAW_SUCCESS)
        {
        }
        
        _impl->imageInfo.buffer_width = _impl->libRaw.imgdata.sizes.width;
        _impl->imageInfo.buffer_height = _impl->libRaw.imgdata.sizes. height;
        _impl->imageInfo.buffer_channels = _impl->imageInfo.image_channels;
    }
    else
    {
        std::cerr << "Raw pre " << std::endl;
        
        std::cerr << "   pre_mul ";
        float ff = _impl->libRaw.imgdata.color.pre_mul[1];
        for (size_t i = 0; i < 3; i++ ) std::cerr << _impl->libRaw.imgdata.color.pre_mul[i] << " ";
        std::cerr << "    ";
        for (size_t i = 0; i < 3; i++ ) std::cerr << _impl->libRaw.imgdata.color.pre_mul[i] / ff << " ";
        std::cerr << std::endl;
        
        std::cerr << "   cam_mul ";
        ff = _impl->libRaw.imgdata.color.cam_mul[1];
        for (size_t i = 0; i < 3; i++ ) std::cerr << _impl->libRaw.imgdata.color.cam_mul[i] / ff << " ";
        std::cerr << std::endl;
        
        // This is not needed, but seems to modify pre_mul
//        result = _impl->libRaw.dcraw_process();
        
        std::cerr << "Raw post " << std::endl;
        
        std::cerr << "   pre_mul ";
        ff = _impl->libRaw.imgdata.color.pre_mul[1];
        for (size_t i = 0; i < 3; i++ ) std::cerr << _impl->libRaw.imgdata.color.pre_mul[i] << " ";
        std::cerr << "    ";
        for (size_t i = 0; i < 3; i++ ) std::cerr << _impl->libRaw.imgdata.color.pre_mul[i] / ff << " ";
        std::cerr << std::endl;
        
        std::cerr << "   cam_mul ";
        ff = _impl->libRaw.imgdata.color.cam_mul[1];
        for (size_t i = 0; i < 3; i++ ) std::cerr << _impl->libRaw.imgdata.color.cam_mul[i] / ff << " ";
        std::cerr << std::endl;
        
        _impl->imageInfo.buffer_width = _impl->libRaw. imgdata.rawdata.sizes.raw_width;
        _impl->imageInfo.buffer_height = _impl->libRaw.imgdata.rawdata.sizes.raw_height;
        _impl->imageInfo.buffer_channels = 1;
        
        std::string p = _impl->libRaw. imgdata. rawdata.iparams.cdesc;
        if (p. length () == 4)
        {
            _impl->imageInfo.pattern = "    " ;
            _impl->imageInfo.pattern[0] = p[_impl->libRaw.COLOR(0, 0)];
            _impl->imageInfo.pattern[1] = p[_impl->libRaw.COLOR(0, 1)];
            _impl->imageInfo.pattern[2] = p[_impl->libRaw.COLOR(1, 0)];
            _impl->imageInfo.pattern[3] = p[_impl->libRaw.COLOR(1, 1)];
        }
    }
    
    _impl->imageInfo.metadata.camera_make = _impl->libRaw.imgdata.idata.make;
    _impl->imageInfo.metadata.camera_model = _impl->libRaw.imgdata.idata.model;
    _impl->imageInfo.metadata.iso = _impl->libRaw.imgdata.other.iso_speed;
    _impl->imageInfo.metadata.shutter = _impl->libRaw.imgdata.other.shutter;
    _impl->imageInfo.metadata.aperture = _impl->libRaw.imgdata.other.aperture;
    
//    std::cerr << "ISO" << _impl->imageInfo.metadata.iso << std::endl;
//    std::cerr << "Shutter " << _impl->imageInfo.metadata.shutter << std::endl;
//    std::cerr << "Aperture " << _impl->imageInfo.metadata.aperture << std::endl;
    
    auto R = _impl->libRaw.imgdata.rawdata;
    ImageInfo::Metadata & metadata = _impl->imageInfo.metadata;
    
    metadata.cameraCalibration1 = std::vector<double> ( 9, 1.0 );
    metadata.cameraCalibration2 = std::vector<double> ( 9, 1.0 );
    metadata.xyz2rgbMatrix1 = std::vector<double> ( 9, 1.0 );
    metadata.xyz2rgbMatrix2 = std::vector<double> ( 9, 1.0 );
    metadata.analogBalance = std::vector<double> ( 3, 1.0 );
    metadata.neutralRGB = std::vector<double> ( 3, 1.0 );
    metadata. calibrateIllum = std::vector<double> ( 2, 1.0 );
    
#if LIBRAW_VERSION >= LIBRAW_MAKE_VERSION ( 0, 20, 0 )
    metadata. baseExpo = static_cast<double>(R.color.dng_levels. baseline_exposure );
#else
    metadata. baseExpo = static_cast<double> (R.color.baseline_exposure );
#endif
    
    metadata.calibrateIllum[0] = static_cast<double>( R.color.dng_color[0].illuminant );
    metadata.calibrateIllum[1] = static_cast<double>( R.color.dng_color[1].illuminant );
    
//    double smallest = std::numeric_limits<double>::max();
    
    for (size_t i = 0; i < 3; i++ )
    {
        metadata.neutralRGB[i] = static_cast<double> (R.color.dng_levels.asshotneutral[i] );
//        metadata.neutralRGB[i] = 1.0 / static_cast<double> (R.color.cam_mul[i] );
//        metadata.neutralRGB[i] = 1.0 / static_cast<double> (_impl->libRaw.imgdata.color.pre_mul[i] );
        
        for ( size_t j = 0; j < 3; j++ )
        {
            metadata.xyz2rgbMatrix1[i * 3 + j] = static_cast<double>( (R.color.dng_color[0].colormatrix )[i][j] );
            metadata.xyz2rgbMatrix2[i * 3 + j] = static_cast<double>( (R.color.dng_color[1].colormatrix )[i][j] );
            metadata.cameraCalibration1[i * 3 + j] = static_cast<double>((R.color.dng_color[0].calibration )[i][j] );
            metadata.cameraCalibration2[i * 3 + j] = static_cast<double>((R.color.dng_color[1].calibration )[i][j] );
        }
    }
}

LibrawReadNode::~LibrawReadNode()
{
    
}
    
const ImageInfo & LibrawReadNode::imageInfo()
{
    return _impl->imageInfo;
}
 
void LibrawReadNode::processLines (size_t startLine, size_t endLine, const float * buffer)
{
    size_t width = _impl->imageInfo.buffer_width;
    size_t stride = width * _impl->imageInfo.buffer_channels;
    float scale = 1;// _impl->scale;
    
    if (_impl->demosaic)
    {
        for ( size_t line = startLine; line <= endLine; line++ )
        {
            size_t offset = width * line;
            float * p = (float *) buffer + stride * (line - startLine);
            
            for (size_t x = 0; x < width; x++)
            {
                *p++ = _impl->libRaw.imgdata.image[x + offset][0] * scale;
                *p++ = _impl->libRaw.imgdata.image[x + offset][1] * scale;
                *p++ = _impl->libRaw.imgdata.image[x + offset][2] * scale;
            }
        }
    }
    else
    {
        float black = _impl->libRaw.imgdata.color.black;
        
        for ( size_t line = startLine; line <= endLine; line++ )
        {
            size_t offset = stride * line;
            uint16_t * p1 = (uint16_t *)_impl->libRaw. imgdata.rawdata.raw_image + offset;
            
            float * p2 = (float *) buffer + _impl->imageInfo.buffer_width * (line - startLine);
            for (size_t x = 0; x < stride; x++)
            {
                float value = *p1++;
                
                if (value < black) value = 0;
                else value -= black;
                
                *p2++ = value * scale;
            }
        }
    }
}

#endif // USE_LIBRAW
