// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the RawToAces Project.

#ifndef MATRIX_NODE_H_
#define MATRIX_NODE_H_

#include "Node.h"

class MatrixNode: public Node
{
    class Impl;
    std::unique_ptr<Impl> _impl;
public:
    MatrixNode ( std::shared_ptr<Node> input, const float matrix[3][3] );
    ~MatrixNode ();
    
    void processLines (size_t startLine, size_t endLine, const float * buffer) override;
};

#endif // MATRIX_NODE_H_
