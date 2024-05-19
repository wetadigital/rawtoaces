// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the RawToAces Project.

#include "nodes/LibRawReadNode.h"
#include "nodes/RawSpeedReadNode.h"
#include "nodes/ScanlineCacheNode.h"
#include "nodes/BilinearDemosaicNode.h"
#include "nodes/OLFDemosaicNode.h"
#include "nodes/MatrixNode.h"
#include "nodes/ScaleNode.h"
#include "nodes/OpenEXRWriteNode.h"

#include "options.h"

#include <iostream>
#include <filesystem>

#include <rawtoaces/rta.h>

#include <Imath/ImathMatrix.h>

#include <boost/program_options.hpp>


bool separateDemosaic = false;





// TODO: refactor. Temporarily copied from acesrender.cpp
std::vector<string> findFiles( std::string filePath, std::vector<string> searchPaths )
{
    std::vector<std::string> foundFiles;

    for ( auto &i: searchPaths )
    {
        std::string path = i + "/" + filePath;

        if ( boost::filesystem::exists( path ) )
            foundFiles.push_back( path );
    }

    return foundFiles;
}

enum class Decoder
{
#if USE_LIBRAW
    LibRaw,
#endif
    
#if USE_RAWSPEED
    Rawspeed,
#endif
    
    Decoder_Last
};


float scale = 6.0f;


std::shared_ptr<Node> readNodeForFile(const std::string & inputFilename)
{
    // TODO: Make the decoder selectable at run time
    
#if USE_LIBRAW
    return std::make_shared<LibrawReadNode>(inputFilename, !separateDemosaic);
#endif
    
#if USE_RAWSPEED
    return std::make_shared<RawSpeedReadNode>(inputFilename);
#endif
    
    return nullptr;
}

void run_libraw_combined(const Options & options, const std::string & inputFilename, const std::string & outputFilename)
{
    float image_scale = 1.0f;
    
    std::shared_ptr<Node> node = std::make_shared<LibrawReadNode>(inputFilename, false);
    
    auto info = node->imageInfo();
    bool isDNG = info.metadata.xyz2rgbMatrix1[0] != 0;
    
    float m[3][3];
    
    if (isDNG)
    {
        auto metadata = node->imageInfo().metadata;
        auto idt = rta::DNGIdt(metadata);
        
        auto _idtm = idt.getDNGIDTMatrix();
        
        for (size_t i = 0; i < 3; i++)
            for (size_t j = 0; j < 3; j++)
                m[i][j] = _idtm[j][i];
        
    }
    else
    {
        dataPath dataPaths = pathsFinder();
        if (dataPaths.paths.size() == 0)
        {
            std::cerr << "Data path not found. Check the AMPAS_DATA_PATH environment variable." << std::endl;
            exit(1);
        }
        
        
        auto metadata = node->imageInfo().metadata;
        auto idt = rta::Idt();
        idt.setVerbosity(1);
        
        
        std::filesystem::path camera_path;
        
        std::filesystem::path dir(dataPaths.paths[0] + "/camera");
        
        for (const auto & i : std::filesystem::directory_iterator(dir) )
        {
            std::string ext = i.path().extension();
            
            if (ext == ".json")
            {
                std::string filename = i.path().string();
                
                int result = idt.loadCameraSpst(filename, metadata.camera_make.c_str(), metadata.camera_model.c_str());
                if (result)
                {
                    camera_path = i.path();
                    break;
                }
            }
        }
        
        if (camera_path.empty())
        {
            std::cerr << "Spectral sensitivities not found for " << metadata.camera_make << " " << metadata.camera_model << std::endl;
            exit(1);
        }
        
        idt.loadTrainingData(dataPaths.paths[0] + "/training/training_spectral.json");
        idt.loadCMF(dataPaths.paths[0] + "/cmf/cmf_1931.json");
        
        std::vector<std::string> illuminants;
        illuminants.push_back(dataPaths.paths[0] + "/illuminant/iso7589_stutung_380_780_5.json");
        
        idt.loadIlluminant(illuminants, "na");
        
        
        std::vector<double> white(3);
        white[0] = 1.0 / metadata.neutralRGB[0];
        white[1] = 1.0 / metadata.neutralRGB[1];
        white[2] = 1.0 / metadata.neutralRGB[2];
        
        idt.chooseIllumSrc(white, 0);
        
        if ( idt.calIDT() )
        {
            vector<vector<double>> idtm = idt.getIDT();
            vector<double> vec = idt.getWB();
            
            for (size_t i = 0; i < 3; i++)
                for (size_t j = 0; j < 3; j++)
                    m[i][j] = idtm[j][i];
        }
    }
    
    node = std::make_shared<MatrixNode> (node, m);
    
    // Scale output
    {
        float maximum = (1u << 16) - 1;
        image_scale = scale / maximum;
    }
        
    // Scale output
    {
        image_scale *= options.scale;
        float scale_m[3][3] = {0};
        for (size_t i = 0; i < 3; i++)
            scale_m[i][i] = image_scale;

        node = std::make_shared<MatrixNode> (node, scale_m);
    }
    
    std::shared_ptr<WriteNode> writeNode = std::make_shared<OpenEXRWriteNode> (node, outputFilename);
    
    writeNode->validateAll();
    writeNode->writeFile();
}

void run(const Options & options, const std::string & inputFilename, const std::string & outputFilename)
{
    float image_scale = 1.0f;
    separateDemosaic = true;
    
    std::shared_ptr<Node> node = readNodeForFile(inputFilename);
    
    if (options.mode != Options::Mode::Raw)
    {
        node = std::make_shared<ScanlineCacheNode> (node) ;
        node = std::make_shared<BilinearDemosaicNode>(node);
        //node = std::make_shared<OLFDemosaicNode> (node);
        
        
        if (options.mode != Options::Mode::Demosaic)
        {
            
            auto info = node->imageInfo();
            
            
            
            
            // White-balance
            {
                auto info = node->imageInfo();
                float m[3][3] = {0};
                m[0][0] = 1.0 / info.metadata.neutralRGB[0];
                m[1][1] = 1.0 / info.metadata.neutralRGB[1];
                m[2][2] = 1.0 / info.metadata.neutralRGB[2];
                
                float maximum = 59818;
                m[0][0] *= 65535.0 / maximum;
                m[1][1] *= 65535.0 / maximum;
                m[2][2] *= 65535.0 / maximum;
                
                node = std::make_shared<MatrixNode> (node, m);
            }
            
            if (options.mode != Options::Mode::Native)
            {
                
                {
                    // Camera RGB to XYZ
                    
                    auto info = node->imageInfo();
                    
                    Imath::M33d m1, m2, m3;
                    
                    // sRGB to XYZ
                    const double xyz_rgb[3][3] = {
                        {0.4124564, 0.3575761, 0.1804375},
                        {0.2126729, 0.7151522, 0.0721750},
                        {0.0193339, 0.1191920, 0.9503041}};
                    
                    for (int i = 0; i < 3; i++)
                    {
                        for (int j = 0; j < 3; j++)
                        {
                            m1[i][j] = info.metadata.xyz2rgbMatrix2[i * 3 + j];
                            m2[i][j] = xyz_rgb[i][j];
                        }
                    }
                    
                    //            xyz_to_cameraRGB * sRGB_to_xyz
                    
                    m3 = m1 * m2;
                    
                    double pre_mul[3];
                    
                    double num;
                    for (int i = 0; i < 3; i++)
                    {                               /* Normalize cam_rgb so that */
                        
                        num = 0;
                        for (int j = 0; j < 3; j++) /* cam_rgb * (1,1,1) is (1,1,1,1) */
                            num += m3[i][j];
                        if (num > 0.00001)
                        {
                            for (int j = 0; j < 3; j++)
                                m3[i][j] /= num;
                            pre_mul[i] = 1 / num;
                        }
                        else
                        {
                            for (int j = 0; j < 3; j++)
                                m3[i][j] = 0.0;
                            pre_mul[i] = 1.0;
                        }
                    }
                    
                    
                    
                    
                    
                    
                    m3.invert();
                    
                    //m3 = m2 * m3;
                    
                    float mm[3][3];
                    
                    for (int i = 0; i < 3; i++)
                        for (int j = 0; j < 3; j++)
                            mm[i][j] = m3[j][i];
                    
                    node = std::make_shared<MatrixNode> (node, mm);
                }
                
                if (options.mode != Options::Mode::CIEXYZ)
                {
                    
                    if (1)
                    {
                        // 709 to XYZ
                        const float xyz_rgb_tr[3][3] = {
                            {0.4124564, 0.2126729, 0.0193339},
                            {0.3575761, 0.7151522, 0.1191920},
                            {0.1804375, 0.0721750, 0.9503041}};
                        
                        node = std::make_shared<MatrixNode> (node, xyz_rgb_tr);
                    }
                    else
                    {
                        
                    }
                    
                    
                    bool isDNG = info.metadata.xyz2rgbMatrix1[0] != 0;
                    
                    float m[3][3];
                    
                    if (isDNG)
                    {
                        auto metadata = node->imageInfo().metadata;
                        auto idt = rta::DNGIdt(metadata);
                        
                        auto _idtm = idt.getDNGIDTMatrix();
                        
                        for (size_t i = 0; i < 3; i++)
                            for (size_t j = 0; j < 3; j++)
                                m[i][j] = _idtm[j][i];
                        
                    }
                    else
                    {
                        dataPath dataPaths = pathsFinder();
                        if (dataPaths.paths.size() == 0)
                        {
                            std::cerr << "Data path not found. Check the AMPAS_DATA_PATH environment variable." << std::endl;
                            exit(1);
                        }
                        
                        
                        auto metadata = node->imageInfo().metadata;
                        auto idt = rta::Idt();
                        idt.setVerbosity(1);
                        
                        
                        std::filesystem::path camera_path;
                        
                        std::filesystem::path dir(dataPaths.paths[0] + "/camera");
                        
                        for (const auto & i : std::filesystem::directory_iterator(dir) )
                        {
                            std::string ext = i.path().extension();
                            
                            if (ext == ".json")
                            {
                                std::string filename = i.path().string();
                                
                                int result = idt.loadCameraSpst(filename, metadata.camera_make.c_str(), metadata.camera_model.c_str());
                                if (result)
                                {
                                    camera_path = i.path();
                                    break;
                                }
                            }
                        }
                        
                        if (camera_path.empty())
                        {
                            std::cerr << "Spectral sensitivities not found for " << metadata.camera_make << " " << metadata.camera_model << std::endl;
                            exit(1);
                        }
                        
                        idt.loadTrainingData(dataPaths.paths[0] + "/training/training_spectral.json");
                        idt.loadCMF(dataPaths.paths[0] + "/cmf/cmf_1931.json");
                        
                        std::vector<std::string> illuminants;
                        illuminants.push_back(dataPaths.paths[0] + "/illuminant/iso7589_stutung_380_780_5.json");
                        
                        idt.loadIlluminant(illuminants, "na");
                        
                        
                        std::vector<double> white(3);
                        white[0] = 1.0 / metadata.neutralRGB[0];
                        white[1] = 1.0 / metadata.neutralRGB[1];
                        white[2] = 1.0 / metadata.neutralRGB[2];
                        
                        idt.chooseIllumSrc(white, 0);
                        
                        if ( idt.calIDT() )
                        {
                            vector<vector<double>> idtm = idt.getIDT();
                            vector<double> vec = idt.getWB();
                            
                            for (size_t i = 0; i < 3; i++)
                                for (size_t j = 0; j < 3; j++)
                                    m[i][j] = idtm[j][i];
                        }
                    }
                    
                    node = std::make_shared<MatrixNode> (node, m);
                }
            }
            
            
            // Scale output
            {
                float maximum = (1u << 16) - 1;
                image_scale = scale / maximum;
            }
        }
    }
    
    
    
    
    // Scale output
    {
        image_scale *= options.scale;
        node = std::make_shared<ScaleNode> (node, image_scale);
    }
    
    std::shared_ptr<WriteNode> writeNode = std::make_shared<OpenEXRWriteNode> (node, options.cropMode, outputFilename);
    
    writeNode->validateAll();
    writeNode->writeFile();
}
    
    
    
int main(int argc, const char * argv[])
{
    
    Options options;
    options.parse(argc, argv);
    
    if (options.status != -1)
        exit(options.status);
    
    
    auto dataPath = pathsFinder();
    
    
    
    
    
    if (options.outputFile.length() == 0)
    {
        // Legacy mode.
        // No output file provided, generate the file name for each input.
        
        for (auto input_path : options.inputFiles)
        {
            std::string output_path = input_path.substr(0, input_path.find_last_of(".")) + "_aces.exr";
            
            run(options, input_path, output_path);
        }
    }
    else
    {
        if ( options.inputFiles.size() > 0)
        {
            run(options, options.inputFiles[0], options.outputFile);
        }
    }
    
    return 0;
}

