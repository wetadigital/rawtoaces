// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the RawToAces Project.

#ifndef NODE_H_
#define NODE_H_

#include <string>
#include <memory>
#include <vector>

#include "../../include/rawtoaces/metadata.h"

struct ImageInfo
{
    size_t buffer_width;
    size_t buffer_height;
    size_t buffer_channels;
    
    size_t image_width;
    size_t image_height;
    size_t image_channels;
    
    size_t top_offset;
    size_t left_offset;
    
    std::string pattern;
    
    struct Metadata : rta::Metadata
    {
        std::string camera_make;
        std::string camera_model;
        
        float iso;
        float shutter;
        float aperture;
    } metadata;
};

class Node
{
public:
    Node ( std::shared_ptr<Node> input ): _input(input) {}
    
    virtual const ImageInfo & imageInfo()
    {
        return _input->imageInfo();
    }
    
    virtual void validate(size_t startLine, size_t endLine)
    {
        if (_input)
            _input->validate(startLine, endLine);
    }
    
    virtual void processLines (size_t startLine, size_t endLine, const float * buffer)
    {
        _input->processLines (startLine, endLine, buffer);
    }
    
protected:
    std::shared_ptr<Node> _input;
};


class WriteNode : public Node
{
public:
    using Node::Node;
    
    virtual void validateAll() = 0;
    virtual void writeFile() = 0;
};

class FetchHelper
{
    std::shared_ptr<Node> _input;
    
    size_t _margin;
    size_t _height;
    size_t _first;
    
    size_t _size;
    size_t _current;
    
    size_t _lines_loaded;
    size_t _last_loaded;
    
    std::vector<size_t> _offsets;
    std::vector<float> _buffer;
    std::vector<float *> _buffer_offsets;
public:
    FetchHelper (std::shared_ptr<Node> input, size_t margin, size_t first):
    _input (input),
    _margin (margin),
    _height (input->imageInfo().buffer_height),
    _first (first),
    _size (margin + margin + 1),
    _current (first),
    _lines_loaded (0),
    _offsets(_size),
    _buffer (_size),
    _buffer_offsets(_size)
    {
        const ImageInfo & info = input->imageInfo();
        const size_t stride = info.buffer_width * info.buffer_channels;
        
        _buffer.resize(stride * _size);
        
        for (size_t i = 0; i < _size; i++)
        {
            _buffer_offsets[i] = _buffer.data() + stride * i;
        }
    }
    
    void step()
    {
        size_t first_to_load = _current + _size - _margin;
        size_t lines_to_load = _size;
        
        if (first_to_load < _size)
        {
            size_t diff = _size - first_to_load;
            first_to_load += diff;
            lines_to_load -= diff;
        }
        
        if (_lines_loaded > 0)
        {
            if (first_to_load < _last_loaded + _size)
            {
                size_t diff = _last_loaded + _size + 1;
                first_to_load += diff;
                lines_to_load -= diff;
            }
        }
        
        first_to_load -= _size;
        
        if (first_to_load + lines_to_load > _height)
        {
            lines_to_load -= first_to_load + lines_to_load - _height;
        }
        
        size_t central = (_current - first_to_load + _lines_loaded) % _size;
        
        _offsets[_margin] = central;
        
        for (size_t i = 1; i <= _margin; i++)
        {
            size_t above = (central + _size - i) % _size;
            size_t below = (central + i) % _size;
            
            _offsets[_margin - i] = _current < i ? below : above;
            _offsets[_margin + i] = _current + i < _height ? below : above;
        }
        
        
        if (_input != nullptr)
        {
            _input->processLines (first_to_load, first_to_load + lines_to_load - 1, _buffer_offsets[_lines_loaded]);
        }
        
        _current++;
        _lines_loaded += lines_to_load;
        _last_loaded = first_to_load + lines_to_load - 1;
    }
    
    float * operator[](size_t i)
    {
        return _buffer_offsets[i];
    };
};

#endif // NODE_H_
