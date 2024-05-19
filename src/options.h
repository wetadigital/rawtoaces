// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the RawToAces Project.

#ifndef OPTIONS_H_
#define OPTIONS_H_

#include <vector>
#include <string>

class Options
{
public:
    enum class MatrixMethod
    {
        Auto,
        Spectral,
        Metadata,
        Adobe,
        Custom
    } matrixMethod = MatrixMethod::Auto;

    enum class WhiteBalanceMethod
    {
        Auto,
        Illuminant,
        Metadata,
        AverageAll,
        AverageBox,
        Custom
    } whiteBalanceMethod = WhiteBalanceMethod::Auto;

    enum class Mode
    {
        ACES,
        CIEXYZ,
        Native,
        Demosaic,
        Raw,
        Custom
    } mode = Mode::ACES;
    
    enum class Decoder
    {
    #if USE_LIBRAW
        LibRaw,
    #endif
        
    #if USE_RAWSPEED
        Rawspeed,
    #endif
        
        Decoder_Last
    } decoder = static_cast<Decoder>(0);
    
    enum class CropMode
    {
        None,
        Soft,
        Hard
    } cropMode = CropMode::Soft;
    
    float customCameraMatrix[9] = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f};
    float customOutputMatrix[9] = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f};
    float customWhiteBalanceMultipliers[3] = {1.0f, 1.0f, 1.0f};
    float customCCT = 5600;
    
    std::vector<std::string> inputFiles;
    std::string outputFile;
    
    float scale = 1.0;
    
    int status = -1;
    
    void parse(int argc, const char * argv[]);
    
};



std::istream & operator>> (std::istream & in, Options::MatrixMethod & result);
std::istream & operator>> (std::istream & in, Options::Mode & result);
std::istream & operator>> (std::istream & in, Options::CropMode & result);

#endif // OPTIONS_H_
