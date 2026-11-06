#include "DrawBoard.h"
#include "cMain.h"
#include "NetModel.h"
#include <wx/dcbuffer.h>
#include <wx/file.h>
#include <wx/filename.h>
#include <wx/textdlg.h>
#include <sstream>

#include <iomanip>
#include <cmath>
#include <algorithm>
DrawBoard::DrawBoard(wxWindow*p,cMain*m):wxPanel(p,wxID_ANY,wxDefaultPosition,wxDefaultSize,wxWANTS_CHARS),mainFrame(m){
    SetBackgroundStyle(wxBG_STYLE_PAINT);SetBackgroundColour(wxColour(249,251,254));
    Bind(wxEVT_PAINT,&DrawBoard::Paint,this);Bind(wxEVT_LEFT_DOWN,&DrawBoard::Down,this);Bind(wxEVT_LEFT_UP,&DrawBoard::Up,this);Bind(wxEVT_MOTION,&DrawBoard::Motion,this);Bind(wxEVT_RIGHT_DOWN,&DrawBoard::Right,this);Bind(wxEVT_KEY_DOWN,&DrawBoard::Key,this);
    Bind(wxEVT_LEFT_DCLICK,[this](wxMouseEvent&e){if(tool==Tool::Select){Select(World(e.GetPosition()));EditSelected();}});
    Bind(wxEVT_MOUSEWHEEL,[this](wxMouseEvent&e){wxPoint anchor=World(e.GetPosition());zoomFactor=std::clamp(zoomFactor*(e.GetWheelRotation()>0?1.1:1/1.1),0.3,3.0);pan=e.GetPosition()-wxPoint(std::lround(anchor.x*zoomFactor),std::lround(anchor.y*zoomFactor));UpdateZoomStatus();Refresh();});
    Bind(wxEVT_MIDDLE_DOWN,[this](wxMouseEvent&e){if(drawing||dragging)return;panning=true;panStart=e.GetPosition()-pan;if(!HasCapture())CaptureMouse();SetCursor(wxCursor(wxCURSOR_HAND));});
    Bind(wxEVT_MIDDLE_UP,[this](wxMouseEvent&){panning=false;if(HasCapture())ReleaseMouse();SetCursor(wxCursor(wxCURSOR_ARROW));});
    Bind(wxEVT_MOUSE_CAPTURE_LOST,[this](wxMouseCaptureLostEvent&){Cancel();});
    Bind(wxEVT_SIZE,[this](wxSizeEvent&e){Refresh();e.Skip();});
}
wxPoint DrawBoard::World(const wxPoint&p)const{return wxPoint(std::lround((p.x-pan.x)/zoomFactor),std::lround((p.y-pan.y)/zoomFactor));}
wxPoint DrawBoard::Snap(wxPoint p)const{return wxPoint(std::lround(p.x/10.0)*10,std::lround(p.y/10.0)*10);}
wxPoint DrawBoard::Nearest(wxPoint p)const{
    wxPoint best=Snap(p);double d=144/(zoomFactor*zoomFactor);
    auto test=[&](wxPoint q){double v=std::pow(p.x-q.x,2)+std::pow(p.y-q.y,2);if(v<d){d=v;best=q;}};
    for(auto&c:components){std::vector<wxPoint>in,out;GetGatePinPositions(c.type,c.position,in,out);for(auto q:in)test(q);for(auto q:out)test(q);}
    for(auto w:wires){test(w.first);test(w.second);if(w.first.x==w.second.x)test(wxPoint(w.first.x,std::clamp(p.y,std::min(w.first.y,w.second.y),std::max(w.first.y,w.second.y))));if(w.first.y==w.second.y)test(wxPoint(std::clamp(p.x,std::min(w.first.x,w.second.x),std::max(w.first.x,w.second.x)),w.first.y));}return best;
}
void DrawBoard::SaveState(){history.push_back(Snapshot());if(history.size()>80)history.pop_front();future.clear();}
void DrawBoard::Changed(){dirty=true;simulationMode=false;Refresh();if(mainFrame)mainFrame->BoardChanged();}
void DrawBoard::Restore(const State&s){components=s.components;wires=s.wires;texts=s.texts;selected=selectedWire=selectedText=-1;for(auto&c:components)c.isSelected=false;Changed();}
void DrawBoard::UndoLastAction(){Cancel();if(history.empty())return;future.push_back(Snapshot());auto s=history.back();history.pop_back();Restore(s);}
void DrawBoard::Redo(){Cancel();if(future.empty())return;history.push_back(Snapshot());auto s=future.back();future.pop_back();Restore(s);}
void DrawBoard::Cancel(){if(dragging&&dragSaved){components=dragState.components;wires=dragState.wires;texts=dragState.texts;history.pop_back();}drawing=dragging=panning=dragSaved=false;if(HasCapture())ReleaseMouse();Refresh();}
void DrawBoard::SetTool(Tool t){Cancel();tool=t;SetCursor(wxCursor(t==Tool::Wire||t==Tool::Place?wxCURSOR_CROSS:wxCURSOR_ARROW));Refresh();}
void DrawBoard::SetSelectedGate(GateType t){SetTool(Tool::Place);selectedGate=t;}
void DrawBoard::Route(wxPoint a,wxPoint b){wxPoint mid(b.x,a.y);if(a!=mid)wires.emplace_back(a,mid);if(mid!=b)wires.emplace_back(mid,b);}
void DrawBoard::Paint(wxPaintEvent&){wxAutoBufferedPaintDC dc(this);dc.SetBackground(wxBrush(GetBackgroundColour()));dc.Clear();auto gc=wxGraphicsContext::Create(dc);if(!gc)return;gc->Translate(pan.x,pan.y);gc->Scale(zoomFactor,zoomFactor);
    wxPoint lo=World({0,0}),hi=World({GetClientSize().x,GetClientSize().y});gc->SetPen(*wxTRANSPARENT_PEN);gc->SetBrush(wxBrush(wxColour(204,216,232)));
    for(int x=(int)std::floor(lo.x/20.0)*20;x<hi.x;x+=20)for(int y=(int)std::floor(lo.y/20.0)*20;y<hi.y;y+=20)gc->DrawEllipse(x-0.8,y-0.8,1.6,1.6);
    for(int i=0;i<(int)wires.size();++i){auto w=wires[i];gc->SetPen(wxPen(i==selectedWire?wxColour(46,108,230):lineColour,i==selectedWire?3:2));gc->StrokeLine(w.first.x,w.first.y,w.second.x,w.second.y);}
    // Junction dots only where endpoints meet another segment.
    gc->SetBrush(wxBrush(lineColour));gc->SetPen(*wxTRANSPARENT_PEN);for(int i=0;i<(int)wires.size();++i)for(auto p:{wires[i].first,wires[i].second})for(int j=0;j<i;++j)if(OnSegment(p,wires[j].first,wires[j].second)){gc->DrawEllipse(p.x-3,p.y-3,6,6);break;}
    for(int i=0;i<(int)components.size();++i){auto&c=components[i];wxColour color=lineColour;int state=-1;if(simulationMode){state=mainFrame->GetCircuitSimulator()->GetComponentState(i);color=state==1?wxColour(22,143,103):wxColour(103,121,143);}DrawGate(gc,c.type,c.position,color,i==selected);
        gc->SetFont(wxFontInfo(9).FaceName("Microsoft YaHei UI"),color);wxString label=wxString::Format("U%d",i+1);if(c.type==GateType::START_NODE)label+=" = "+c.value;else if(state>=0)label+=wxString::Format(" = %d",state);else if(!c.value.empty())label+=" "+c.value;gc->DrawText(label,c.position.x,c.position.y-23);}
    for(int i=0;i<(int)texts.size();++i){auto&t=texts[i];gc->PushState();gc->Translate(t.position.x,t.position.y);gc->Rotate(t.angle);gc->SetFont(GetFont(),i==selectedText?wxColour(46,108,230):lineColour);gc->DrawText(t.content,0,0);gc->PopState();}
    if(drawing){auto end=Nearest(mousePos);gc->SetPen(wxPen(wxColour(46,108,230),2,wxPENSTYLE_SHORT_DASH));gc->StrokeLine(start.x,start.y,end.x,start.y);gc->StrokeLine(end.x,start.y,end.x,end.y);}
    if(tool==Tool::Place)DrawGate(gc,selectedGate,Snap(mousePos),wxColour(105,154,222));delete gc;
    if(Empty()&&tool==Tool::Select){dc.SetTextForeground(wxColour(118,136,157));dc.SetFont(wxFontInfo(12).FaceName("Microsoft YaHei UI"));dc.DrawText(L"从左侧选择元件，开始搭建电路",45,50);dc.SetFont(wxFontInfo(10).FaceName("Microsoft YaHei UI"));dc.DrawText(L"也可以载入全加器示例，了解连接与仿真。",45,82);}
}
void DrawBoard::Select(wxPoint p){selected=selectedWire=selectedText=-1;for(auto&c:components)c.isSelected=false;
    for(int i=(int)components.size()-1;i>=0;--i){auto&c=components[i];int w=(c.type==GateType::START_NODE||c.type==GateType::END_NODE)?40:60;int h=c.type==GateType::ADDER?60:40;if(wxRect(c.position-wxPoint(6,6),wxSize(w+12,h+12)).Contains(p)){selected=i;c.isSelected=true;break;}}
    if(selected<0)for(int i=(int)texts.size()-1;i>=0;--i){auto&t=texts[i];wxClientDC dc(this);auto z=dc.GetTextExtent(t.content);double dx=p.x-t.position.x,dy=p.y-t.position.y;wxPoint q(std::lround(dx*cos(t.angle)+dy*sin(t.angle)),std::lround(-dx*sin(t.angle)+dy*cos(t.angle)));if(wxRect(wxPoint(-3,-3),z+wxSize(6,6)).Contains(q)){selectedText=i;break;}}
    if(selected<0&&selectedText<0)for(int i=(int)wires.size()-1;i>=0;--i){auto w=wires[i];double dx=w.second.x-w.first.x,dy=w.second.y-w.first.y,len=dx*dx+dy*dy;if(!len)continue;double u=std::clamp(((p.x-w.first.x)*dx+(p.y-w.first.y)*dy)/len,0.0,1.0);if(std::hypot(p.x-w.first.x-u*dx,p.y-w.first.y-u*dy)<6/zoomFactor){selectedWire=i;break;}}
    if(mainFrame)mainFrame->UpdateSelection();Refresh();
}
void DrawBoard::Down(wxMouseEvent&e){SetFocus();mousePos=World(e.GetPosition());if(tool==Tool::Wire){start=Nearest(mousePos);drawing=true;CaptureMouse();}
    else if(tool==Tool::Place){SaveState();components.push_back({Snap(mousePos),selectedGate,selectedGate==GateType::START_NODE?"1":""});Changed();mainFrame->SetMode(Tool::Select);Select(Snap(mousePos)+wxPoint(10,10));}
    else if(tool==Tool::Text){wxTextEntryDialog d(this,L"输入注释内容",L"添加文字");if(d.ShowModal()==wxID_OK&&!d.GetValue().empty()){SaveState();texts.push_back({d.GetValue(),Snap(mousePos)});Changed();}}
    else {Select(mousePos);if(selected>=0){dragOffset=mousePos-components[selected].position;dragState=Snapshot();dragging=true;dragSaved=false;CaptureMouse();}}
}
void DrawBoard::MoveSelected(wxPoint target){
    if(components[selected].position==target)return;
    auto previous=Snapshot();components=dragState.components;wires=dragState.wires;
    NetModel nets(this);int base=0;for(auto&v:nets.inputs)base+=(int)v.size();for(auto&v:nets.outputs)base+=(int)v.size();
    std::vector<int> wireNets;for(int k=0;k<(int)wires.size();++k)wireNets.push_back(nets.Root(base+2*k));
    std::vector<std::pair<wxPoint,int>> allPins;
    components[selected].position=target;
    for(int i=0;i<(int)components.size();++i){std::vector<wxPoint>in,out;GetGatePinPositions(components[i].type,components[i].position,in,out);for(int j=0;j<(int)in.size();++j)allPins.push_back({in[j],nets.Root(nets.inputs[i][j])});for(int j=0;j<(int)out.size();++j)allPins.push_back({out[j],nets.Root(nets.outputs[i][j])});}
    std::vector<wxPoint>oldIn,oldOut,newIn,newOut;auto& c=components[selected];GetGatePinPositions(c.type,dragState.components[selected].position,oldIn,oldOut);GetGatePinPositions(c.type,target,newIn,newOut);
    std::vector<int> ids=nets.inputs[selected];ids.insert(ids.end(),nets.outputs[selected].begin(),nets.outputs[selected].end());oldIn.insert(oldIn.end(),oldOut.begin(),oldOut.end());newIn.insert(newIn.end(),newOut.begin(),newOut.end());
    std::vector<std::vector<wxPoint>> anchors(ids.size());
    std::vector<std::pair<wxPoint,wxPoint>> kept;std::vector<int> keptNets;
    for(size_t k=0;k<dragState.wires.size();++k){auto w=dragState.wires[k];bool incident=false;
        for(size_t j=0;j<ids.size();++j){if(w.first==oldIn[j]||w.second==oldIn[j]){wxPoint anchor=w.first==oldIn[j]?w.second:w.first;for(size_t z=0;z<ids.size();++z)if(anchor==oldIn[z])anchor=newIn[z];anchors[j].push_back(anchor);incident=true;}else if(OnSegment(oldIn[j],w.first,w.second))anchors[j].push_back(oldIn[j]);}
        if(!incident){kept.push_back(w);keptNets.push_back(wireNets[k]);}}
    wires=std::move(kept);wireNets=std::move(keptNets);
    for(int j=0;j<(int)ids.size();++j)for(wxPoint old:anchors[j]){wxPoint end=newIn[j];if(old==end)continue;int net=nets.Root(ids[j]);
        auto accept=[&](std::vector<wxPoint> pts){std::vector<std::pair<wxPoint,wxPoint>> edges;for(size_t k=1;k<pts.size();++k)if(pts[k-1]!=pts[k])edges.emplace_back(pts[k-1],pts[k]);
            for(auto e:edges){for(size_t k=0;k<wires.size();++k){if(wireNets[k]==net)continue;auto w=wires[k];if(OnSegment(e.first,w.first,w.second)||OnSegment(e.second,w.first,w.second)||OnSegment(w.first,e.first,e.second)||OnSegment(w.second,e.first,e.second))return false;}for(auto pin:allPins)if(pin.second!=net&&OnSegment(pin.first,e.first,e.second))return false;}
            for(auto e:edges){wires.push_back(e);wireNets.push_back(net);}return true;};
        bool found=accept({old,{end.x,old.y},end})||accept({old,{old.x,end.y},end});
        for(int d=10;!found&&d<=1000;d+=10)for(int sign:{-1,1}){int x=old.x+sign*d,y=old.y+sign*d;if(accept({old,{x,old.y},{x,end.y},end})||accept({old,{old.x,y},{end.x,y},end})){found=true;break;}}
        if(!found){components=previous.components;wires=previous.wires;if(mainFrame)mainFrame->SetStatusText(L"当前位置无法保持连接，请换一个位置或先调整导线。");return;}
    }
    if(!dragSaved){auto moved=Snapshot();components=dragState.components;wires=dragState.wires;SaveState();components=moved.components;wires=moved.wires;dragSaved=true;}Refresh();
}
void DrawBoard::Motion(wxMouseEvent&e){mousePos=World(e.GetPosition());if(panning){pan=e.GetPosition()-panStart;Refresh();return;}if(dragging)MoveSelected(Snap(mousePos-dragOffset));if(mainFrame)mainFrame->GetCoordLabel()->SetLabel(wxString::Format(L"坐标 %d, %d     缩放 %.0f%%",mousePos.x,mousePos.y,zoomFactor*100));Refresh(false);}
void DrawBoard::Up(wxMouseEvent&e){if(drawing){auto end=Nearest(World(e.GetPosition()));if(end!=start){SaveState();Route(start,end);Changed();}drawing=false;}
    if(dragging){dragging=false;if(dragSaved){dragSaved=false;Changed();}}if(HasCapture()&&!panning)ReleaseMouse();Refresh();}
void DrawBoard::Right(wxMouseEvent&e){if(tool!=Tool::Select||drawing||dragging){mainFrame->SetMode(Tool::Select);return;}Select(World(e.GetPosition()));wxMenu menu;menu.Append(wxID_PROPERTIES,L"编辑属性 / 切换输入");menu.Append(wxID_DELETE,L"删除选中");PopupMenu(&menu);}
void DrawBoard::Key(wxKeyEvent&e){if(e.GetKeyCode()==WXK_ESCAPE){mainFrame->SetMode(Tool::Select);return;}if(e.GetKeyCode()==WXK_DELETE||e.GetKeyCode()==WXK_BACK){DeleteSelectedComponent();return;}e.Skip();}
wxString DrawBoard::SelectionDescription()const{if(selected>=0){auto&c=components[selected];return wxString::Format(L"U%d  %s\n坐标 %d, %d\n",selected+1,GateName(c.type),c.position.x,c.position.y)+(c.type==GateType::START_NODE?L"输入电平："+c.value:L"名称："+c.value);}if(selectedWire>=0)return L"已选择一段导线\n按 Delete 删除";if(selectedText>=0)return L"已选择文字注释\n双击编辑，Delete 删除";return L"选择元件查看属性\n双击输入切换 0 / 1";}
void DrawBoard::EditSelected(){if(selected>=0){auto&c=components[selected];if(c.type==GateType::START_NODE){SaveState();c.value=c.value=="1"?"0":"1";Changed();return;}wxTextEntryDialog d(this,L"元件名称",L"编辑属性",c.value);if(d.ShowModal()==wxID_OK&&d.GetValue()!=c.value){SaveState();c.value=d.GetValue();Changed();}}
    else if(selectedText>=0){auto&t=texts[selectedText];wxTextEntryDialog d(this,L"注释内容",L"编辑文字",t.content);if(d.ShowModal()==wxID_OK&&!d.GetValue().empty()&&d.GetValue()!=t.content){SaveState();t.content=d.GetValue();Changed();}}}
void DrawBoard::DeleteSelectedComponent(){if(selected<0&&selectedWire<0&&selectedText<0)return;SaveState();if(selected>=0){std::vector<wxPoint>in,out;auto&c=components[selected];GetGatePinPositions(c.type,c.position,in,out);in.insert(in.end(),out.begin(),out.end());wires.erase(std::remove_if(wires.begin(),wires.end(),[&](auto w){for(auto p:in)if(w.first==p||w.second==p)return true;return false;}),wires.end());components.erase(components.begin()+selected);}else if(selectedWire>=0)wires.erase(wires.begin()+selectedWire);else texts.erase(texts.begin()+selectedText);selected=selectedWire=selectedText=-1;Changed();}
void DrawBoard::ClearCanvas(){Cancel();SaveState();components.clear();wires.clear();texts.clear();selected=selectedWire=selectedText=-1;Changed();}
void DrawBoard::ZoomIn(){zoomFactor=std::min(3.0,zoomFactor*1.1);UpdateZoomStatus();Refresh();}void DrawBoard::ZoomOut(){zoomFactor=std::max(0.3,zoomFactor/1.1);UpdateZoomStatus();Refresh();}void DrawBoard::ResetZoom(){pan={0,0};zoomFactor=1;UpdateZoomStatus();Refresh();}void DrawBoard::UpdateZoomStatus(){if(mainFrame)mainFrame->SetStatusText(wxString::Format(L"缩放 %.0f%%",zoomFactor*100));}
void DrawBoard::LoadExample(){ClearCanvas();components={{{100,100},GateType::START_NODE,"1"},{{100,200},GateType::START_NODE,"1"},{{100,300},GateType::START_NODE,"0"},{{360,160},GateType::ADDER,""},{{620,150},GateType::END_NODE,L"和 S"},{{620,270},GateType::END_NODE,L"进位 Cout"}};
    Route({160,120},{260,120});Route({260,120},{260,170});Route({260,170},{340,170});Route({160,220},{240,220});Route({240,220},{240,190});Route({240,190},{340,190});Route({160,320},{280,320});Route({280,320},{280,210});Route({280,210},{340,210});Route({440,170},{600,170});Route({440,210},{520,210});Route({520,210},{520,290});Route({520,290},{600,290});texts.push_back({L"全加器：双击左侧输入切换电平，按 F5 计算",{90,400}});ResetZoom();Changed();}

namespace {
std::string U8(const wxString&s){auto b=s.ToUTF8();return std::string(b.data(),b.length());}
bool AtomicWrite(const wxString&path,const std::string&body){wxTempFile f(path);return f.IsOpened()&&f.Write(body.data(),body.size())&&f.Commit();}
}
bool DrawBoard::WriteDocument(const wxString&path,bool net){std::ostringstream s;s<<(net?"NETLIST 2\n":"CIRCUIT 2\n");s<<"COLOR "<<(int)lineColour.Red()<<' '<<(int)lineColour.Green()<<' '<<(int)lineColour.Blue()<<'\n';
    for(auto&c:components)s<<"GATE "<<(int)c.type<<' '<<c.position.x<<' '<<c.position.y<<' '<<std::quoted(U8(c.value))<<'\n';
    for(auto w:wires)s<<"WIRE "<<w.first.x<<' '<<w.first.y<<' '<<w.second.x<<' '<<w.second.y<<'\n';
    for(auto&t:texts)s<<"TEXT "<<t.position.x<<' '<<t.position.y<<' '<<std::setprecision(17)<<t.angle<<' '<<std::quoted(U8(t.content))<<'\n';s<<"END\n";return AtomicWrite(path,s.str());}
bool DrawBoard::ReadDocument(const wxString&path,bool net){wxFile f(path);if(!f.IsOpened()||f.Length()>4*1024*1024||f.Length()<8)return false;std::string bytes((size_t)f.Length(),'\0');if(f.Read(bytes.data(),bytes.size())!=(wxFileOffset)bytes.size())return false;
    State next;wxColour color=lineColour;std::istringstream s(bytes);std::string head;int version=0;
    auto valid=[](int v){return v>=-100000&&v<=100000;};
    if(bytes.substr(0,4)=="CIRC"&&bytes[4]==1){ // Read legacy v1 without modifying the board on failure.
        size_t pos=4;auto read=[&](auto&v){if(pos+sizeof(v)>bytes.size())return false;memcpy(&v,bytes.data()+pos,sizeof(v));pos+=sizeof(v);return true;};
        int v,width;unsigned char r,g,b;if(!read(v)||v!=1||!read(r)||!read(g)||!read(b)||!read(width)||width<1||width>20)return false;color=wxColour(r,g,b);
        auto count=[&](int&n){return read(n)&&n>=0&&n<=10000;};auto str=[&](wxString&out){int n;if(!read(n)||n<0||n>65536||pos+n>bytes.size())return false;out=wxString::FromUTF8(bytes.data()+pos,n);if(out.empty()&&n)return false;pos+=n;return true;};int n;
        if(!count(n))return false;while(n--){wxPoint a,b;if(!read(a.x)||!read(a.y)||!read(b.x)||!read(b.y)||!valid(a.x)||!valid(a.y)||!valid(b.x)||!valid(b.y))return false;if(a!=b)next.wires.emplace_back(a,b);}
        if(!count(n))return false;while(n--){CircuitComponent c;int type;if(!read(c.position.x)||!read(c.position.y)||!read(type)||type<0||type>9||!str(c.value)||!valid(c.position.x)||!valid(c.position.y))return false;c.type=(GateType)type;if(c.type==GateType::START_NODE)c.value="1";next.components.push_back(c);}
        if(!count(n))return false;while(n--){TextElement t;if(!read(t.position.x)||!read(t.position.y)||!read(t.angle)||!std::isfinite(t.angle)||!str(t.content)||!valid(t.position.x)||!valid(t.position.y))return false;next.texts.push_back(t);}if(pos!=bytes.size())return false;
    }else if((s>>head>>version)&&version==2&&head==(net?"NETLIST":"CIRCUIT")){
        std::string tag;bool ended=false;int items=0;while(s>>tag){if(++items>10000)return false;if(tag=="END"){ended=true;break;}
            if(tag=="COLOR"){int r,g,b;if(!(s>>r>>g>>b)||r<0||g<0||b<0||r>255||g>255||b>255)return false;color=wxColour(r,g,b);}
            else if(tag=="GATE"){int type;CircuitComponent c;std::string v;if(!(s>>type>>c.position.x>>c.position.y>>std::quoted(v))||type<0||type>9||!valid(c.position.x)||!valid(c.position.y)||v.size()>65536)return false;c.type=(GateType)type;c.value=wxString::FromUTF8(v);if(!v.empty()&&c.value.empty())return false;if(c.type==GateType::START_NODE&&c.value!="0"&&c.value!="1")return false;next.components.push_back(c);}
            else if(tag=="WIRE"){wxPoint a,b;if(!(s>>a.x>>a.y>>b.x>>b.y)||!valid(a.x)||!valid(a.y)||!valid(b.x)||!valid(b.y))return false;if(a!=b)next.wires.emplace_back(a,b);}
            else if(tag=="TEXT"){TextElement t;std::string v;if(!(s>>t.position.x>>t.position.y>>t.angle>>std::quoted(v))||!valid(t.position.x)||!valid(t.position.y)||!std::isfinite(t.angle)||v.size()>65536)return false;t.content=wxString::FromUTF8(v);if(!v.empty()&&t.content.empty())return false;next.texts.push_back(t);}else return false;
        }std::string extra;if(!ended||(s>>extra))return false;
    }else {
        // Legacy coordinate netlists: preserve geometry; CONNECT comments were advisory only.
        if(!net)return false;std::istringstream lines(bytes);std::string line;bool any=false;while(std::getline(lines,line)){std::istringstream l(line);std::string tag,name,type;if(!(l>>tag)||tag.substr(0,2)=="//")continue;if(tag=="CONNECT")continue;
            if(tag=="WIRE"){wxPoint a,b;if(!(l>>name>>a.x>>a.y>>b.x>>b.y)||!valid(a.x)||!valid(a.y)||!valid(b.x)||!valid(b.y))return false;next.wires.emplace_back(a,b);}
            else if(tag=="GATE"||tag=="NODE"||tag=="COMP"){CircuitComponent c;if(!(l>>name>>type>>c.position.x>>c.position.y)||!valid(c.position.x)||!valid(c.position.y))return false;const std::vector<std::string> types={"AND","OR","NOT","NAND","NOR","XNOR","XOR","INPUT","OUTPUT","ADDER"};auto it=std::find(types.begin(),types.end(),type);if(it==types.end())return false;c.type=(GateType)(it-types.begin());c.value=c.type==GateType::START_NODE?"1":"";next.components.push_back(c);}else return false;any=true;if(next.components.size()+next.wires.size()>10000)return false;}if(!any)return false;
    }
    Cancel();SaveState();Restore(next);lineColour=color;dirty=net;ResetZoom();if(mainFrame)mainFrame->BoardChanged();return true;
}
bool DrawBoard::SaveToFile(const wxString&p){if(!WriteDocument(p,false))return false;dirty=false;return true;}bool DrawBoard::LoadFromFile(const wxString&p){return ReadDocument(p,false);}bool DrawBoard::SaveNetlistToFile(const wxString&p){return WriteDocument(p,true);}bool DrawBoard::LoadNetlistFromFile(const wxString&p){return ReadDocument(p,true);}
bool DrawBoard::ExportToSpice(const wxString&path){CircuitSimulator check;if(!check.RunSimulation(this)){if(mainFrame)mainFrame->SetStatusText(check.GetSimulationResult());return false;}NetModel nets(this);std::ostringstream s;s<<"* Digital combinational circuit, behavioral sources, 0/1 V\n";auto node=[&](int id){return std::string("n")+std::to_string(nets.Root(id)+1);};
    for(int i=0;i<(int)components.size();++i){auto&c=components[i];if(c.type==GateType::START_NODE){s<<"V"<<i+1<<' '<<node(nets.outputs[i][0])<<" 0 DC "<<U8(c.value)<<'\n';continue;}if(c.type==GateType::END_NODE){s<<"* Output U"<<i+1<<" = "<<node(nets.inputs[i][0])<<'\n';continue;}
        std::vector<std::string> in;for(int pin:nets.inputs[i])in.push_back("(V("+node(pin)+")>0.5)");std::string sum="(";for(size_t j=0;j<in.size();++j){if(j)sum+="+";sum+=in[j];}sum+=")";
        for(int j=0;j<(int)nets.outputs[i].size();++j){std::string expr;switch(c.type){case GateType::AND_GATE:expr=sum+"==2";break;case GateType::NAND_GATE:expr=sum+"!=2";break;case GateType::OR_GATE:expr=sum+">0";break;case GateType::NOR_GATE:expr=sum+"==0";break;case GateType::NOT_GATE:expr=sum+"==0";break;case GateType::XOR_GATE:expr=sum+"==1";break;case GateType::XNOR_GATE:expr=sum+"!=1";break;case GateType::ADDER:expr=j?sum+">=2":"("+sum+"==1)+("+sum+"==3)";break;default:expr="0";}
            s<<"B"<<i+1<<'_'<<j<<' '<<node(nets.outputs[i][j])<<" 0 V={"<<expr<<"}\n";}
    }s<<".op\n.end\n";return AtomicWrite(path,s.str());}
bool DrawBoard::RunSelfTests(const wxString&report){int passed=0;std::ostringstream log;auto require=[&](bool ok,const char*name){log<<(ok?"PASS ":"FAIL ")<<name<<'\n';AtomicWrite(report,log.str());if(!ok)throw std::runtime_error(name);++passed;};auto original=Snapshot();
    try{for(int a=0;a<2;++a)for(int b=0;b<2;++b){std::vector<int> in={a,b};require(CircuitSimulator::Evaluate(GateType::AND_GATE,in)==(a&b),"AND truth table");require(CircuitSimulator::Evaluate(GateType::NAND_GATE,in)==!(a&b),"NAND truth table");require(CircuitSimulator::Evaluate(GateType::OR_GATE,in)==(a|b),"OR truth table");require(CircuitSimulator::Evaluate(GateType::NOR_GATE,in)==!(a|b),"NOR truth table");require(CircuitSimulator::Evaluate(GateType::XOR_GATE,in)==(a^b),"XOR truth table");require(CircuitSimulator::Evaluate(GateType::XNOR_GATE,in)==!(a^b),"XNOR truth table");for(int c=0;c<2;++c){require(CircuitSimulator::Evaluate(GateType::ADDER,{a,b,c})==((a+b+c)%2),"full adder sum");require(CircuitSimulator::Evaluate(GateType::ADDER,{a,b,c},1)==(a+b+c>=2),"full adder carry");}}for(int a=0;a<2;++a)require(CircuitSimulator::Evaluate(GateType::NOT_GATE,{a})==!a,"NOT truth table");
        LoadExample();CircuitSimulator sim;for(int a=0;a<2;++a)for(int b=0;b<2;++b)for(int c=0;c<2;++c){components[0].value=wxString::Format("%d",a);components[1].value=wxString::Format("%d",b);components[2].value=wxString::Format("%d",c);require(sim.RunSimulation(this)&&sim.GetComponentState(4)==((a+b+c)%2)&&sim.GetComponentState(5)==(a+b+c>=2),"routed full adder end-to-end");}
        selected=3;dragState=Snapshot();dragSaved=false;MoveSelected({400,200});dragSaved=false;require(components[3].position==wxPoint(400,200)&&sim.RunSimulation(this),"drag preserves routed connectivity");UndoLastAction();require(components[3].position==wxPoint(360,160)&&sim.RunSimulation(this),"undo drag");Redo();require(components[3].position==wxPoint(400,200)&&sim.RunSimulation(this),"redo drag");
        components[3].value=L"中文名称";texts.push_back({L"中文注释与换行\n第二行",{10,10},0.25});auto filename=report+".circuit";require(SaveToFile(filename),"save UTF-8 document");auto saved=Snapshot();components.clear();require(LoadFromFile(filename)&&components[3].value==L"中文名称"&&texts.back().content==saved.texts.back().content,"reload Chinese document");require(SaveNetlistToFile(report+".net")&&LoadNetlistFromFile(report+".net")&&sim.RunSimulation(this),"netlist roundtrip");require(ExportToSpice(report+".cir"),"SPICE model export");
        AtomicWrite(report+".bad","CIRCUIT 2\nGATE 90 0 0 \"bad\"\nEND\n");auto before=components.size();require(!LoadFromFile(report+".bad")&&components.size()==before,"malformed file leaves board intact");
        components.push_back({{800,500},GateType::END_NODE,""});require(!sim.RunSimulation(this),"floating input rejected");components.pop_back();
        wires.emplace_back(components[0].position+wxPoint(60,20),components[1].position+wxPoint(60,20));require(!sim.RunSimulation(this),"multiple drivers rejected");
        components={{{100,100},GateType::NOT_GATE,""}};wires.clear();Route({180,120},{80,120});require(!sim.RunSimulation(this),"feedback rejected");
        components={{{100,100},GateType::START_NODE,"1"},{{400,200},GateType::END_NODE,""},{{300,100},GateType::END_NODE,""}};wires={{{160,120},{250,120}},{{250,120},{250,220}},{{250,220},{380,220}},{{250,120},{280,120}}};require(sim.RunSimulation(this)&&sim.GetComponentState(1)==1&&sim.GetComponentState(2)==1,"junction fan-out");
        components={{{100,100},GateType::START_NODE,"1"},{{400,200},GateType::END_NODE,""}};wires={{{160,120},{350,120}},{{250,50},{250,220}},{{250,220},{380,220}}};require(!sim.RunSimulation(this),"interior crossing stays disconnected");
        log<<passed<<" checks passed\n";
    }catch(const std::exception&e){log<<"ERROR "<<e.what()<<'\n';AtomicWrite(report,log.str());return false;}Restore(original);dirty=false;return AtomicWrite(report,log.str());}
