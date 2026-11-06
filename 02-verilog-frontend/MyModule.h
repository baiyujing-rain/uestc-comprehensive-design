#pragma once

#ifndef MYMODULE_H
#define MYMODULE_H

#include <map>
#include <string>
#include <vector>

struct PWire {
    enum class WireType { Port, Internal };
    enum class PortType { None, Input, Output, Inout };

    std::string name;
    WireType wiretype = WireType::Internal;
    PortType porttype = PortType::None;

    bool is_port() const;
};

struct Module {
    std::string modname;
    std::vector<std::string> port_order;
    std::map<std::string, PWire> wires;

    bool has_wire(const std::string &name) const;
    const PWire *find_wire(const std::string &name) const;
    PWire &ensure_wire(const std::string &name);
    void add_port(const std::string &name, PWire::PortType direction);
    void add_wire(const std::string &name);
};

#endif
