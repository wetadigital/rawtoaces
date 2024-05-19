cmake_minimum_required(VERSION 3.5)
include_directories( "${CMAKE_CURRENT_SOURCE_DIR}" )

option( USE_LIBRAW "Use the libraw library for decoding" ON )
option( USE_RAWSPEED "Use the rawspeed library for decoding" OFF )



add_executable( rawtoaces2
    rawtoaces_main.cpp
    options.cpp
    options.h

    nodes/Node.h
    nodes/LibrawReadNode.h
    nodes/LibrawReadNode.cpp
    nodes/RawSpeedReadNode.h
    nodes/RawSpeedReadNode.cpp
    nodes/BilinearDemosaicNode.h
    nodes/BilinearDemosaicNode.cpp
    nodes/OLFDemosaicNode.h
    nodes/OLFDemosaicNode.cpp
    nodes/ScanlineCacheNode.h
    nodes/ScanlineCacheNode.cpp
    nodes/MatrixNode.h
    nodes/MatrixNode.cpp
    nodes/OpenEXRWriteNode.h
    nodes/OpenEXRWriteNode.cpp
)

target_link_libraries ( rawtoaces2
    PUBLIC
        ${RAWTOACESIDTLIB}
        Boost::program_options
        OpenEXR::OpenEXR
)

if ( USE_LIBRAW )
    if ( LIBRAW_CONFIG_FOUND )
        target_link_libraries ( rawtoaces2 PUBLIC libraw::raw )
    else ( LIBRAW_CONFIG_FOUND )
        target_link_directories( rawtoaces2 PUBLIC ${libraw_LIBRARY_DIRS} )
        target_link_libraries( rawtoaces2 PUBLIC ${libraw_LIBRARIES} ${libraw_LDFLAGS_OTHER} )
    endif ( LIBRAW_CONFIG_FOUND )
endif ( USE_LIBRAW )


target_compile_definitions ( rawtoaces2
    PRIVATE
        USE_LIBRAW=$<BOOL:${USE_LIBRAW}>
        USE_RAWSPEED=$<BOOL:${USE_RAWSPEED}>
)
