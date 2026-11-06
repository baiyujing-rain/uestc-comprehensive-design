#pragma once
#include "DrawBoard.h"
#include "CircuitSimulator.h"
#include <wx/tglbtn.h>
class cMain:public wxFrame {
public:
    cMain();DrawBoard* GetBoard(){return board;}CircuitSimulator* GetCircuitSimulator(){return &simulator;}
    bool IsSimulationModeActive()const{return board->IsSimulationMode();}
    void BoardChanged();void UpdateSelection();void SetMode(DrawBoard::Tool);void Run();
    wxStaticText* GetCoordLabel(){return coords;}
private:
    DrawBoard* board;CircuitSimulator simulator;wxStaticText *coords,*selection,*counts,*modeLabel;wxTextCtrl* results;
    wxToggleButton *selectBtn,*wireBtn,*textBtn;wxString currentFile;
    void Save(bool as=false);void Open();bool ConfirmDiscard();void Title();
};
