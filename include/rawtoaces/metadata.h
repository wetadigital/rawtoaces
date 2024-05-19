//
//  metadata.h
//  RawToAces2
//
//  Created by Anton Dukhovnikov on 12/12/23.
//

#ifndef RTA_METADATA_H_
#define RTA_METADATA_H_

namespace rta
{

struct Metadata
{
    
    // Colorimetry
    std::vector<double> cameraCalibration1;
    std::vector<double> cameraCalibration2;
    std::vector<double> xyz2rgbMatrix1;
    std::vector<double> xyz2rgbMatrix2;
    std::vector<double> analogBalance;
    std::vector<double> neutralRGB;
    std::vector<double> calibrateIllum;
    
    double baseExpo;
};

} // namespace rta

#endif // RTA_METADATA_H_
