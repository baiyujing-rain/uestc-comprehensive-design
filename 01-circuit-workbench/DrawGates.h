#pragma once
#include <wx/wx.h>
#include <wx/graphics.h>
#include <vector>
enum class GateType { AND_GATE, OR_GATE, NOT_GATE, NAND_GATE, NOR_GATE, XNOR_GATE, XOR_GATE, START_NODE, END_NODE, ADDER };
wxString GateName(GateType type);
void DrawGate(wxGraphicsContext*, GateType, const wxPoint&, const wxColour&, bool selected=false);
void GetGatePinPositions(GateType, const wxPoint&, std::vector<wxPoint>&, std::vector<wxPoint>&);
bool IsPointNearPin(const wxPoint&,const wxPoint&,int threshold=10);
