#include "MyModule.h"

bool PWire::is_port() const
{
    return wiretype == WireType::Port;
}

bool Module::has_wire(const std::string &name) const
{
    return wires.find(name) != wires.end();
}

const PWire *Module::find_wire(const std::string &name) const
{
    std::map<std::string, PWire>::const_iterator it = wires.find(name);
    return it == wires.end() ? nullptr : &it->second;
}

PWire &Module::ensure_wire(const std::string &name)
{
    std::map<std::string, PWire>::iterator it = wires.find(name);
    if (it == wires.end()) {
        PWire wire;
        wire.name = name;
        it = wires.insert(std::make_pair(name, wire)).first;
    }
    return it->second;
}

void Module::add_port(const std::string &name, PWire::PortType direction)
{
    PWire &wire = ensure_wire(name);
    wire.name = name;
    wire.wiretype = PWire::WireType::Port;
    if (wire.porttype == PWire::PortType::None || direction != PWire::PortType::None) {
        wire.porttype = direction;
    }
    for (size_t i = 0; i < port_order.size(); ++i) {
        if (port_order[i] == name) {
            return;
        }
    }
    port_order.push_back(name);
}

void Module::add_wire(const std::string &name)
{
    PWire &wire = ensure_wire(name);
    wire.name = name;
    if (wire.wiretype != PWire::WireType::Port) {
        wire.wiretype = PWire::WireType::Internal;
    }
}
