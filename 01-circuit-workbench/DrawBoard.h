#pragma once
#include "DrawGates.h"
#include <deque>
class cMain;
class DrawBoard:public wxPanel {
public:
    struct CircuitComponent {wxPoint position;GateType type;wxString value;bool isSelected=false;};
    struct TextElement {wxString content;wxPoint position;double angle=0;bool isSelected=false;};
    enum class Tool {Select,Wire,Text,Place};
    DrawBoard(wxWindow*,cMain*);
    void SetTool(Tool);void SetSelectedGate(GateType);void SetLineColour(const wxColour& c){lineColour=c;Refresh();}
    void UndoLastAction();void Redo();void ClearCanvas();void DeleteSelectedComponent();void EditSelected();void LoadExample();
    bool SaveToFile(const wxString&);bool LoadFromFile(const wxString&);
    bool SaveNetlistToFile(const wxString&);bool LoadNetlistFromFile(const wxString&);bool ExportToSpice(const wxString&);
    void ZoomIn();void ZoomOut();void ResetZoom();void UpdateZoomStatus();
    const std::vector<CircuitComponent>& GetComponents()const{return components;}
    const std::vector<std::pair<wxPoint,wxPoint>>& GetWires()const{return wires;}
    void SetSimulationMode(bool v){simulationMode=v;Refresh();}bool IsSimulationMode()const{return simulationMode;}
    bool IsDirty()const{return dirty;}bool Empty()const{return components.empty()&&wires.empty()&&texts.empty();}
    wxString SelectionDescription()const;bool RunSelfTests(const wxString& report);
private:
    cMain* mainFrame;std::vector<CircuitComponent> components;std::vector<std::pair<wxPoint,wxPoint>> wires;std::vector<TextElement> texts;
    struct State {decltype(components) components;decltype(wires) wires;decltype(texts) texts;};
    std::deque<State> history,future;
    State Snapshot()const{return {components,wires,texts};}void Restore(const State&);void SaveState();void Changed();
    Tool tool=Tool::Select;GateType selectedGate=GateType::AND_GATE;wxColour lineColour=wxColour(45,65,88);
    double zoomFactor=1;wxPoint pan,mousePos,start,dragOffset;State dragState;int selected=-1,selectedWire=-1,selectedText=-1;
    bool drawing=false,dragging=false,panning=false,dirty=false,simulationMode=false,dragSaved=false;wxPoint panStart;
    wxPoint World(const wxPoint&)const;wxPoint Snap(wxPoint)const;wxPoint Nearest(wxPoint)const;
    void Route(wxPoint,wxPoint);void Select(wxPoint);void Cancel();void MoveSelected(wxPoint);void Paint(wxPaintEvent&);
    void Down(wxMouseEvent&);void Up(wxMouseEvent&);void Motion(wxMouseEvent&);void Right(wxMouseEvent&);void Key(wxKeyEvent&);
    bool WriteDocument(const wxString&,bool);bool ReadDocument(const wxString&,bool);
};
