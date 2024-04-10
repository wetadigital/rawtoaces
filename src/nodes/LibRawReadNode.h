// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the RawToAces Project.

#ifndef LIBRAW_READER_NODE_H_
#define LIBRAW_READER_NODE_H_

#if USE_LIBRAW

#include "Node.h"

class LibrawReadNode : public Node
{
    class Impl;
    std::unique_ptr<Impl> _impl;
    
public:
    LibrawReadNode( std::string filename, bool demosaic );
    ~LibrawReadNode();
    
    const ImageInfo & imageInfo() override;
    void processLines(size_t startLine, size_t endLine, const float * buffer) override;
};

#endif // USE_LIBRAW
#endif // LIBRAW_READER_NODE_H_
