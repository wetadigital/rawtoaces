// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the RawToAces Project.

#ifndef BILINEAR_DEMOSAIC_NODE_H_
#define BILINEAR_DEMOSAIC_NODE_H_

#include "Node.h"

class
BilinearDemosaicNode : public Node
{
    ImageInfo _imageInfo;

public:
    BilinearDemosaicNode( std::shared_ptr<Node> input );

    const ImageInfo & imageInfo() override;
    void validate (size_t startline, size_t endLine) override;
    void processLines (size_t startLine, size_t endline, const float * buffer) override;
};

#endif // BILINEAR_DEMOSAIC_NODE_H_
