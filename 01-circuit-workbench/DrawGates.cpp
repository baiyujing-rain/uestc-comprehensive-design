#include "DrawGates.h"
wxString GateName(GateType t) {
    static const wxString names[]={L"与门 AND",L"或门 OR",L"非门 NOT",L"与非门 NAND",L"或非门 NOR",L"同或门 XNOR",L"异或门 XOR",L"输入",L"输出",L"全加器"};
    return names[static_cast<int>(t)];
}
void GetGatePinPositions(GateType t,const wxPoint& p,std::vector<wxPoint>& in,std::vector<wxPoint>& out) {
    in.clear(); out.clear();
    if(t==GateType::START_NODE) {out.push_back(p+wxPoint(60,20));return;}
    if(t==GateType::END_NODE) {in.push_back(p+wxPoint(-20,20));return;}
    in.push_back(p+wxPoint(-20,t==GateType::NOT_GATE?20:10));
    if(t!=GateType::NOT_GATE) in.push_back(p+wxPoint(-20,30));
    if(t==GateType::ADDER) in.push_back(p+wxPoint(-20,50));
    out.push_back(p+wxPoint(80,t==GateType::ADDER?10:20));
    if(t==GateType::ADDER) out.push_back(p+wxPoint(80,50));
}
bool IsPointNearPin(const wxPoint&a,const wxPoint&b,int t) {auto d=a-b;return d.x*d.x+d.y*d.y<=t*t;}
void DrawGate(wxGraphicsContext* gc,GateType t,const wxPoint&p,const wxColour& color,bool selected) {
    int w=(t==GateType::START_NODE||t==GateType::END_NODE)?40:60,h=t==GateType::ADDER?60:40;
    if(selected) {gc->SetPen(wxPen(wxColour(46,108,230),1));gc->SetBrush(wxBrush(wxColour(228,238,255)));gc->DrawRoundedRectangle(p.x-7,p.y-7,w+14,h+14,6);}
    gc->SetPen(wxPen(color,2));gc->SetBrush(*wxWHITE_BRUSH);
    gc->DrawRoundedRectangle(p.x,p.y,w,h,4);
    std::vector<wxPoint> in,out;GetGatePinPositions(t,p,in,out);
    for(auto pin:in) gc->StrokeLine(pin.x,pin.y,p.x,pin.y);
    bool inv=t==GateType::NOT_GATE||t==GateType::NAND_GATE||t==GateType::NOR_GATE||t==GateType::XNOR_GATE;
    for(auto pin:out) {if(inv){gc->DrawEllipse(p.x+w,pin.y-4,8,8);gc->StrokeLine(p.x+w+8,pin.y,pin.x,pin.y);}else gc->StrokeLine(p.x+w,pin.y,pin.x,pin.y);}
    const wxString symbols[]={"&",wxString(L"≥1"),"1","&",wxString(L"≥1"),"=1","=1","IN","OUT","FA"};
    wxString s=symbols[static_cast<int>(t)];
    gc->SetFont(wxFontInfo(12).FaceName("Segoe UI").Bold(),color);double tw,th;gc->GetTextExtent(s,&tw,&th);gc->DrawText(s,p.x+(w-tw)/2,p.y+(h-th)/2);
    gc->SetBrush(*wxWHITE_BRUSH);gc->SetPen(wxPen(color,1));
    for(auto pin:in) gc->DrawEllipse(pin.x-3,pin.y-3,6,6);
    for(auto pin:out) gc->DrawEllipse(pin.x-3,pin.y-3,6,6);
    if(t==GateType::ADDER){gc->SetFont(wxFontInfo(8).FaceName("Segoe UI"),color);gc->DrawText("S",p.x+44,p.y+1);gc->DrawText("C",p.x+44,p.y+42);}
}
