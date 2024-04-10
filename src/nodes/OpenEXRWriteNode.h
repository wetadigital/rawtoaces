// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the RawToAces Project.

#ifndef OPENEXR_WRITE_NODE_H_
#define OPENEXR_WRITE_NODE_H_

#include "Node.h"

class OpenEXRWriteNode : public WriteNode
{
    class Impl;
    std::unique_ptr<Impl> _impl;
    
public:
    OpenEXRWriteNode( std::shared_ptr<Node> input, std::string path);
    ~OpenEXRWriteNode () ;
    
    void validateAll() override;
    void writeFile() override;
};

#endif // OPENEXR_WRITE_NODE_H_
