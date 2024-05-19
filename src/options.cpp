// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the RawToAces Project.


#include "options.h"

#include <iostream>
#include <fstream>
#include <boost/program_options.hpp>

std::istream & operator>> (std::istream & in, Options::MatrixMethod & result)
{
    std::string token;
    in >> token;

    if (token == "auto")
    {
        result = Options::MatrixMethod::Auto;
    }
    else if (token == "spectral")
    {
        result = Options::MatrixMethod::Spectral;
    }
    else if (token == "metadata")
    {
        result = Options::MatrixMethod::Metadata;
    }
    else if (token == "adobe")
    {
        result = Options::MatrixMethod::Adobe;
    }
    else if (token == "custom")
    {
        result = Options::MatrixMethod::Custom;
    }
    else
    {
        throw boost::program_options::validation_error (boost::program_options::validation_error::invalid_option_value, "Invalid Matrix Method");
    }

    return in;
}




std::istream & operator>> (std::istream & in, Options::Mode & result)
{
    std::string token;
    in >> token;
    
    if (token == "ACES")
    {
        result = Options::Mode::ACES;
    }
    else if (token == "XYZ")
    {
        result = Options::Mode::CIEXYZ;
    }
    else if (token == "native")
    {
        result = Options::Mode::Native;
    }
    else if (token == "demosaic")
    {
        result = Options::Mode::Demosaic;
    }
    else if (token == "raw")
    {
        result = Options::Mode::Raw;
    }
    else if (token == "custom")
    {
        result = Options::Mode::Custom;
    }
    else
    {
        throw boost::program_options::validation_error (boost::program_options::validation_error::invalid_option_value, "Invalid Mode");
    }
    
    return in;
}

std::istream & operator>> (std::istream & in, Options::CropMode & result)
{
    std::string token;
    in >> token;
    
    if (token == "none")
    {
        result = Options::CropMode::None;
    }
    else if (token == "soft")
    {
        result = Options::CropMode::Soft;
    }
    else if (token == "hard")
    {
        result = Options::CropMode::Hard;
    }
    else
    {
        throw boost::program_options::validation_error (boost::program_options::validation_error::invalid_option_value, "Invalid Crop Mode");
    }
    
    return in;
}

void Options::parse(int argc, const char * argv[])
{
    boost::program_options::options_description desc("Allowed options");
    
    desc.add_options()
        ("help,h", "print this message")
        ("version,v", "print the version number")

        ("config,c", boost::program_options::value<std::string>(), "config file")
        ("matrix_method,m", boost::program_options::value<Options::MatrixMethod>(), "matrix method")
        ("input_files,i", boost::program_options::value<std::vector<std::string>>(), "input files")
        ("output,o", boost::program_options::value<std::string>(), "output file path")
    
        ("scale", boost::program_options::value<float>(), "Scale to be applied to the output. Defaults to 6.")
    
        ("output_mode", boost::program_options::value<Options::Mode>(),
            "conversion mode\n"
            "  'ACES' - convert to the ACES AP0 colour space, the default mode.\n"
            "  'CIEXYZ' - convert to CIE XYZ.\n"
            "  'native' - demosaiced image in the native camera colour space.\n"
            "  'demosaiced' - demosaiced image with no further processing.\n"
            "  'raw' - raw code values from the source file, scaled by the 'scale' factor.\n"
            "  'custom' - use a custom 'xyz_to_output' matrix."
        )
    
        ("crop_mode", boost::program_options::value<Options::CropMode>(),
            "conversion mode\n"
            "  'soft' - mark the crop region as the display window (default).\n"
            "  'hard' - write out only the visible pixels.\n"
            "  'none' - do not crop."
        )
    ;
    
    boost::program_options::positional_options_description positional;
    positional.add("input_files", -1);
    
    
    boost::program_options::variables_map vm;
    boost::program_options::store(boost::program_options::command_line_parser(argc, argv).options(desc).positional(positional).run(), vm);
    boost::program_options::notify(vm);

    if (vm.count("help")) {
        std::cout << desc << "\n";
        status = 0;
    }
    
    
    if (vm.count("version")) {
        std::cout << "TODO version number" << "\n";
        status = 0;
    }
    
    if (vm.count("config"))
    {
        std::string config_file = vm["config"].as<std::string>();
        std::ifstream stream(config_file);
        boost::program_options::store(boost::program_options::parse_config_file(stream, desc), vm);
        boost::program_options::notify(vm);
    }


    if (vm.count("matrix_method")) {
        matrixMethod = vm["matrix_method"].as<Options::MatrixMethod>();
    }
    
    if (vm.count("output_mode")) {
        mode = vm["output_mode"].as<Options::Mode>();
    }
    
    if (vm.count("crop_mode")) {
        cropMode = vm["crop_mode"].as<Options::CropMode>();
    }
    
    if (vm.count("scale")) {
        scale = vm["scale"].as<float>();
    }
    else {
        if (mode == Options::Mode::Raw) scale = 0.001;
    }
    
    
    if (vm.count("output"))
    {
        outputFile = vm["output"].as<std::string>();
    }
    
    if (vm.count("input_files"))
    {
        inputFiles = vm["input_files"].as<std::vector<std::string>>();
    }
    
    
    
}




