// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the RawToAces Project.

#ifndef RAWSPEED_READ_NODE_H_
#define RAWSPEED_READ_NODE_H_

#if USE_RAWSPEED

#include "Node.h"

class RawSpeedReadNode : public Node
{
    class Impl;
    std::unique_ptr<Impl> _impl;
    
public:
    RawSpeedReadNode( std::string filename );
    ~RawSpeedReadNode ();
    
    const ImageInfo & imageInfo() override;
    void processLines (size_t startLine, size_t endLine, const float * buffer) override;
};

#endif // USE_RAWSPEED
#endif // RAWSPEED_READ_NODE_H_
