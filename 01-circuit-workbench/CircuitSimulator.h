#pragma once
#include "DrawBoard.h"
#include <map>
class cMain;
class CircuitSimulator {
public:
    bool RunSimulation(const DrawBoard*);void ResetSimulation(){states.clear();data.clear();result.clear();}
    wxString GetSimulationResult()const{return result;}
    std::map<wxString,std::vector<int>> GetSignalData()const{return data;}
    int GetComponentState(int i)const{return i>=0&&i<(int)states.size()?states[i]:-1;}
    void SetSimulationTime(double){}void SetTimeStep(double){}void SetMainFrame(cMain*){}
    static int Evaluate(GateType,const std::vector<int>&,int output=0);
private:std::vector<int> states;std::map<wxString,std::vector<int>> data;wxString result;
};
