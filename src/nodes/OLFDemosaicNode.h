// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the RawToAces Project.

#ifndef OLF_DEMOSAIC_NODE_
#define OLF_DEMOSAIC_NODE_

#include "Node.h"

// Malvar-He-Cutler demosaicing algorithm with some optimisations

class OLFDemosaicNode : public Node
{
    ImageInfo _imageInfo;
    
public:
    
    OLFDemosaicNode( std::shared_ptr<Node> input );
    
    const ImageInfo & imageInfo() override;
    
    void validate (size_t startLine, size_t endline) override;
    void processLines (size_t startLine, size_t endLine, const float * buffer) override;
};

#endif // OLF_DEMOSAIC_NODE_
