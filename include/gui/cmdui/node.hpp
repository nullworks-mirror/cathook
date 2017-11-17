/*
 * node.hpp
 *
 *  Created on: Nov 17, 2017
 *      Author: nullifiedcat
 */

#pragma once

#include "common.hpp"

namespace gui { namespace cmdui {

struct node_descriptor
{
    enum nodetype
    {
        UNDEFINED,
        LIST,
        VARIABLE,
        COMMAND
    };
    nodetype type;
    std::shared_ptr<node> node;
};

class node
{
public:
    int dummy;
};

class node_list: public node
{
public:
    node_list();
    std::vector<node_descriptor> children {};
};

class node_variable: public node
{
public:
    node_variable(const CatVar& catvar);

};

class node_command: public node
{
public:
    const node_descriptor type { node_descriptor::nodetype::COMMAND };
};

}}


