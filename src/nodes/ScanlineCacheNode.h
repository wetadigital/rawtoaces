// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the RawToAces Project.

#ifndef SCANLINE_CACHE_NODE_H_
#define SCANLINE_CACHE_NODE_H_

#include "Node.h"
#include <vector>

class ScanlineCacheNode: public Node
{
    std::vector<size_t> _counters;
    std::vector<std::vector<float>> _cache;
public:
    ScanlineCacheNode( std::shared_ptr<Node> input );
    ~ScanlineCacheNode();

    void validate(size_t startLine, size_t endLine) override;
    void processLines (size_t startLine, size_t endline, const float * buffer) override;
};

#endif // SCANLINE_CACHE_NODE_H_
