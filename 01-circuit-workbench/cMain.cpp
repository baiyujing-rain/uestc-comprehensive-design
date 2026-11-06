#include "cMain.h"
#include <wx/scrolwin.h>
#include <wx/filename.h>
namespace {enum {ID_SELECT=wxID_HIGHEST+101,ID_WIRE,ID_TEXT,ID_RUN,ID_SAMPLE,ID_NET_SAVE,ID_NET_OPEN,ID_SPICE};
wxStaticText* Label(wxWindow* p,wxString text,int size=10,bool bold=false,wxColour c=wxColour(55,72,94)) {auto l=new wxStaticText(p,wxID_ANY,text);l->SetFont(wxFontInfo(size).FaceName("Microsoft YaHei UI").Weight(bold?wxFONTWEIGHT_BOLD:wxFONTWEIGHT_NORMAL));l->SetForegroundColour(c);return l;}
wxButton* Button(wxWindow*p,wxString text){auto b=new wxButton(p,wxID_ANY,text,wxDefaultPosition,wxSize(-1,34));return b;}
}
cMain::cMain():wxFrame(nullptr,wxID_ANY,L"电路工作台",wxDefaultPosition,wxSize(1380,860)) {
    SetMinSize(wxSize(1080,680));SetFont(wxFontInfo(10).FaceName("Microsoft YaHei UI"));SetBackgroundColour(wxColour(237,242,248));
    auto mb=new wxMenuBar;auto f=new wxMenu;f->Append(wxID_NEW,L"新建\tCtrl+N");f->Append(wxID_OPEN,L"打开…\tCtrl+O");f->Append(wxID_SAVE,L"保存\tCtrl+S");f->Append(wxID_SAVEAS,L"另存为…\tCtrl+Shift+S");f->AppendSeparator();f->Append(ID_NET_SAVE,L"导出可编辑网表…");f->Append(ID_NET_OPEN,L"导入网表…");f->Append(ID_SPICE,L"导出 SPICE 行为模型…");f->AppendSeparator();f->Append(wxID_EXIT,L"退出");mb->Append(f,L"文件");
    auto e=new wxMenu;e->Append(wxID_UNDO,L"撤销\tCtrl+Z");e->Append(wxID_REDO,L"重做\tCtrl+Y");e->Append(wxID_DELETE,L"删除选中\tDel");e->Append(wxID_PROPERTIES,L"编辑属性\tCtrl+E");mb->Append(e,L"编辑");
    auto v=new wxMenu;v->Append(wxID_ZOOM_IN,L"放大\tCtrl+=");v->Append(wxID_ZOOM_OUT,L"缩小\tCtrl+-");v->Append(wxID_ZOOM_FIT,L"恢复视图\tCtrl+0");v->Append(ID_RUN,L"运行逻辑仿真\tF5");mb->Append(v,L"视图与仿真");SetMenuBar(mb);
    auto root=new wxBoxSizer(wxVERTICAL);auto header=new wxPanel(this);header->SetBackgroundColour(wxColour(30,49,73));auto hs=new wxBoxSizer(wxHORIZONTAL);
    hs->Add(Label(header,L"电路工作台",19,true,*wxWHITE),0,wxALL|wxALIGN_CENTER_VERTICAL,18);hs->Add(Label(header,L"数字逻辑设计 / 组合电路仿真",10,false,wxColour(181,202,225)),1,wxALIGN_CENTER_VERTICAL|wxLEFT,12);
    auto run=Button(header,L"▶  运行仿真   F5");run->SetBackgroundColour(wxColour(47,107,218));run->SetForegroundColour(*wxWHITE);hs->Add(run,0,wxALL|wxALIGN_CENTER_VERTICAL,14);header->SetSizer(hs);root->Add(header,0,wxEXPAND);
    auto tools=new wxPanel(this);tools->SetBackgroundColour(*wxWHITE);auto ts=new wxBoxSizer(wxHORIZONTAL);
    selectBtn=new wxToggleButton(tools,ID_SELECT,L"选择 / 移动",wxDefaultPosition,wxSize(112,34));wireBtn=new wxToggleButton(tools,ID_WIRE,L"连线",wxDefaultPosition,wxSize(84,34));textBtn=new wxToggleButton(tools,ID_TEXT,L"文字",wxDefaultPosition,wxSize(84,34));
    for(auto b:{selectBtn,wireBtn,textBtn})ts->Add(b,0,wxALL,6);
    auto addTool=[&](wxString s,auto action){auto b=Button(tools,s);ts->Add(b,0,wxTOP|wxBOTTOM|wxRIGHT,6);b->Bind(wxEVT_BUTTON,action);};
    addTool(L"撤销",[this](wxCommandEvent&){board->UndoLastAction();});addTool(L"重做",[this](wxCommandEvent&){board->Redo();});addTool(L"删除",[this](wxCommandEvent&){board->DeleteSelectedComponent();});ts->AddStretchSpacer();
    addTool(L"打开",[this](wxCommandEvent&){Open();});addTool(L"保存",[this](wxCommandEvent&){Save();});addTool(L"−",[this](wxCommandEvent&){board->ZoomOut();});addTool("100%",[this](wxCommandEvent&){board->ResetZoom();});addTool(L"＋",[this](wxCommandEvent&){board->ZoomIn();});tools->SetSizer(ts);root->Add(tools,0,wxEXPAND|wxBOTTOM,1);
    auto body=new wxBoxSizer(wxHORIZONTAL);auto left=new wxScrolledWindow(this,wxID_ANY,wxDefaultPosition,wxSize(208,-1),wxVSCROLL);left->SetScrollRate(0,12);left->SetBackgroundColour(*wxWHITE);auto ls=new wxBoxSizer(wxVERTICAL);
    ls->Add(Label(left,L"元件库",13,true),0,wxALL,16);ls->Add(Label(left,L"点击元件，再点击画布放置",9),0,wxLEFT|wxBOTTOM,16);
    for(int i: {7,8,0,1,2,3,4,6,5,9}){auto t=static_cast<GateType>(i);auto b=Button(left,GateName(t));b->SetToolTip(t==GateType::ADDER?L"三输入 A / B / Cin；双输出 S / Cout":L"选择后在画布中点击放置");ls->Add(b,0,wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM,10);b->Bind(wxEVT_BUTTON,[this,t](wxCommandEvent&){SetMode(DrawBoard::Tool::Place);board->SetSelectedGate(t);modeLabel->SetLabel(L"放置："+GateName(t));});}
    ls->AddStretchSpacer();auto sample=Button(left,L"载入全加器示例");ls->Add(sample,0,wxEXPAND|wxALL,12);left->SetSizer(ls);body->Add(left,0,wxEXPAND);
    auto center=new wxBoxSizer(wxVERTICAL);auto strip=new wxPanel(this);auto ss=new wxBoxSizer(wxHORIZONTAL);modeLabel=Label(strip,L"选择 / 移动",10,true);ss->Add(modeLabel,1,wxALL,10);ss->Add(Label(strip,L"滚轮缩放  ·  中键平移  ·  Esc 取消",9),0,wxALL,10);strip->SetSizer(ss);center->Add(strip,0,wxEXPAND);
    board=new DrawBoard(this,this);center->Add(board,1,wxEXPAND);body->Add(center,1,wxEXPAND|wxLEFT|wxRIGHT,1);
    auto right=new wxPanel(this,wxID_ANY,wxDefaultPosition,wxSize(264,-1));right->SetBackgroundColour(*wxWHITE);auto rs=new wxBoxSizer(wxVERTICAL);rs->Add(Label(right,L"设计检查",13,true),0,wxALL,16);counts=Label(right,L"0 个元件 / 0 段导线");rs->Add(counts,0,wxLEFT|wxBOTTOM,16);
    selection=Label(right,L"选择元件查看属性\n双击输入切换 0 / 1");rs->Add(selection,0,wxEXPAND|wxALL,16);auto prop=Button(right,L"编辑选中属性");rs->Add(prop,0,wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM,16);
    rs->Add(Label(right,L"仿真结果",12,true),0,wxALL,16);results=new wxTextCtrl(right,wxID_ANY,L"连接所有输入后，点击运行仿真。\n\n蓝色：选中\n绿色：高电平 1\n灰色：低电平 0\n\n折线端点与导线相接会形成连接；仅交叉不连接。",wxDefaultPosition,wxDefaultSize,wxTE_MULTILINE|wxTE_READONLY|wxBORDER_NONE);results->SetBackgroundColour(wxColour(246,249,253));rs->Add(results,1,wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM,14);right->SetSizer(rs);body->Add(right,0,wxEXPAND);root->Add(body,1,wxEXPAND);
    auto bottom=new wxPanel(this);auto bs=new wxBoxSizer(wxHORIZONTAL);coords=Label(bottom,L"坐标 0, 0",9);bs->Add(coords,1,wxALL,7);bs->Add(Label(bottom,L"双击编辑  ·  Delete 删除  ·  Ctrl+Z 撤销",9),0,wxALL,7);bottom->SetSizer(bs);root->Add(bottom,0,wxEXPAND);SetSizer(root);CreateStatusBar();SetStatusText(L"就绪：从左侧选择元件开始设计");
    run->Bind(wxEVT_BUTTON,[this](wxCommandEvent&){Run();});sample->Bind(wxEVT_BUTTON,[this](wxCommandEvent&){if(ConfirmDiscard()){board->LoadExample();currentFile.clear();Title();}});prop->Bind(wxEVT_BUTTON,[this](wxCommandEvent&){board->EditSelected();});
    Bind(wxEVT_TOGGLEBUTTON,[this](wxCommandEvent&e){SetMode(e.GetId()==ID_WIRE?DrawBoard::Tool::Wire:e.GetId()==ID_TEXT?DrawBoard::Tool::Text:DrawBoard::Tool::Select);});
    Bind(wxEVT_MENU,[this](wxCommandEvent&e){switch(e.GetId()){
        case wxID_NEW:if(ConfirmDiscard()){board->ClearCanvas();currentFile.clear();Title();}break;case wxID_OPEN:Open();break;case wxID_SAVE:Save();break;case wxID_SAVEAS:Save(true);break;case wxID_EXIT:Close();break;
        case wxID_UNDO:board->UndoLastAction();break;case wxID_REDO:board->Redo();break;case wxID_DELETE:board->DeleteSelectedComponent();break;case wxID_PROPERTIES:board->EditSelected();break;
        case wxID_ZOOM_IN:board->ZoomIn();break;case wxID_ZOOM_OUT:board->ZoomOut();break;case wxID_ZOOM_FIT:board->ResetZoom();break;case ID_RUN:Run();break;
        case ID_NET_OPEN:case ID_NET_SAVE:case ID_SPICE:{bool open=e.GetId()==ID_NET_OPEN;if(open&&!ConfirmDiscard())break;wxFileDialog d(this,open?L"导入网表":L"导出", "","",e.GetId()==ID_SPICE?L"SPICE 文件|*.cir":L"网表文件|*.net",open?(wxFD_OPEN|wxFD_FILE_MUST_EXIST):(wxFD_SAVE|wxFD_OVERWRITE_PROMPT));if(d.ShowModal()!=wxID_OK)break;bool ok=open?board->LoadNetlistFromFile(d.GetPath()):e.GetId()==ID_SPICE?board->ExportToSpice(d.GetPath()):board->SaveNetlistToFile(d.GetPath());if(!ok)wxMessageBox(L"文件操作失败，请检查文件格式或写入权限。",L"文件",wxOK|wxICON_ERROR);if(open&&ok){currentFile.clear();Title();}break;}
    }});
    Bind(wxEVT_CLOSE_WINDOW,[this](wxCloseEvent&e){if(e.CanVeto()&&!ConfirmDiscard()){e.Veto();return;}e.Skip();});SetMode(DrawBoard::Tool::Select);Center();
}
void cMain::Title(){SetTitle((board->IsDirty()?"* ":"")+(currentFile.empty()?wxString(L"未命名电路"):wxFileName(currentFile).GetFullName())+L" — 电路工作台");}
void cMain::BoardChanged(){simulator.ResetSimulation();board->SetSimulationMode(false);results->SetValue(L"电路已更新，按 F5 重新计算。");counts->SetLabel(wxString::Format(L"%zu 个元件 / %zu 段导线",board->GetComponents().size(),board->GetWires().size()));UpdateSelection();Title();}
void cMain::UpdateSelection(){selection->SetLabel(board->SelectionDescription());selection->Wrap(225);}
void cMain::SetMode(DrawBoard::Tool t){board->SetTool(t);selectBtn->SetValue(t==DrawBoard::Tool::Select);wireBtn->SetValue(t==DrawBoard::Tool::Wire);textBtn->SetValue(t==DrawBoard::Tool::Text);modeLabel->SetLabel(t==DrawBoard::Tool::Wire?L"连线：按住起点拖到目标引脚":t==DrawBoard::Tool::Text?L"文字：点击画布添加注释":L"选择 / 移动");board->SetFocus();}
void cMain::Run(){SetMode(DrawBoard::Tool::Select);bool ok=simulator.RunSimulation(board);board->SetSimulationMode(ok);results->SetValue(simulator.GetSimulationResult());SetStatusText(ok?L"仿真完成：已更新画布电平":L"仿真未完成，请查看右侧检查结果");}
bool cMain::ConfirmDiscard(){if(!board->IsDirty())return true;int a=wxMessageBox(L"当前电路尚未保存，是否保存？",L"未保存的修改",wxYES_NO|wxCANCEL|wxICON_QUESTION,this);if(a==wxCANCEL)return false;if(a==wxYES){Save();return !board->IsDirty();}return true;}
void cMain::Save(bool as){wxString path=currentFile;if(as||path.empty()){wxFileDialog d(this,L"保存电路","",L"未命名.circuit",L"电路文件|*.circuit",wxFD_SAVE|wxFD_OVERWRITE_PROMPT);if(d.ShowModal()!=wxID_OK)return;path=d.GetPath();if(!path.Lower().EndsWith(".circuit"))path+=".circuit";}if(board->SaveToFile(path)){currentFile=path;Title();SetStatusText(L"已保存："+path);}else wxMessageBox(L"保存失败，请检查路径和写入权限。",L"保存",wxOK|wxICON_ERROR,this);}
void cMain::Open(){if(!ConfirmDiscard())return;wxFileDialog d(this,L"打开电路","","",L"电路文件|*.circuit",wxFD_OPEN|wxFD_FILE_MUST_EXIST);if(d.ShowModal()!=wxID_OK)return;if(board->LoadFromFile(d.GetPath())){currentFile=d.GetPath();SetMode(DrawBoard::Tool::Select);Title();}else wxMessageBox(L"文件格式错误或内容不完整，当前电路已保留。",L"打开失败",wxOK|wxICON_ERROR,this);}
