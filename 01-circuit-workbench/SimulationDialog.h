#pragma once
#include <wx/wx.h>
#include <wx/notebook.h>
#include <wx/listctrl.h>
#include "CircuitSimulator.h"
#include "DrawBoard.h"

class SimulationDialog : public wxDialog
{
public:
    SimulationDialog(wxWindow* parent, CircuitSimulator* simulator, DrawBoard* drawBoard);

private:
    CircuitSimulator* circuitSimulator;
    DrawBoard* drawBoard;

    wxTextCtrl* resultText;
    wxListCtrl* signalList;

    void OnRunSimulation(wxCommandEvent& event);
    void OnClose(wxCommandEvent& event);

    void UpdateResults();

    wxDECLARE_EVENT_TABLE();
};