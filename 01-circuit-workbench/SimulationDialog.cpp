#include "SimulationDialog.h"

wxBEGIN_EVENT_TABLE(SimulationDialog, wxDialog)
EVT_BUTTON(wxID_OK, SimulationDialog::OnRunSimulation)
EVT_BUTTON(wxID_CANCEL, SimulationDialog::OnClose)
wxEND_EVENT_TABLE()

SimulationDialog::SimulationDialog(wxWindow* parent, CircuitSimulator* simulator, DrawBoard* db)
    : wxDialog(parent, wxID_ANY, "电路仿真", wxDefaultPosition, wxSize(600, 400)),
    circuitSimulator(simulator), drawBoard(db)
{
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

    // 创建选项卡
    wxNotebook* notebook = new wxNotebook(this, wxID_ANY);

    // 结果页面
    wxPanel* resultPanel = new wxPanel(notebook);
    wxBoxSizer* resultSizer = new wxBoxSizer(wxVERTICAL);

    resultText = new wxTextCtrl(resultPanel, wxID_ANY, "",
        wxDefaultPosition, wxDefaultSize,
        wxTE_MULTILINE | wxTE_READONLY);
    resultSizer->Add(resultText, 1, wxEXPAND | wxALL, 5);
    resultPanel->SetSizer(resultSizer);

    // 信号页面
    wxPanel* signalPanel = new wxPanel(notebook);
    wxBoxSizer* signalSizer = new wxBoxSizer(wxVERTICAL);

    signalList = new wxListCtrl(signalPanel, wxID_ANY,
        wxDefaultPosition, wxDefaultSize,
        wxLC_REPORT);
    signalList->InsertColumn(0, "节点", wxLIST_FORMAT_LEFT, 100);
    signalList->InsertColumn(1, "信号值", wxLIST_FORMAT_LEFT, 200);
    signalSizer->Add(signalList, 1, wxEXPAND | wxALL, 5);
    signalPanel->SetSizer(signalSizer);

    notebook->AddPage(resultPanel, "仿真结果");
    notebook->AddPage(signalPanel, "信号波形");

    mainSizer->Add(notebook, 1, wxEXPAND | wxALL, 5);

    // 按钮区域
    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    buttonSizer->Add(new wxButton(this, wxID_OK, "运行仿真"), 0, wxALL, 5);
    buttonSizer->Add(new wxButton(this, wxID_CANCEL, "关闭"), 0, wxALL, 5);

    mainSizer->Add(buttonSizer, 0, wxALIGN_CENTER | wxALL, 5);

    SetSizer(mainSizer);
    Center();
}

void SimulationDialog::OnRunSimulation(wxCommandEvent& event)
{
    if (circuitSimulator && drawBoard) {
        if (circuitSimulator->RunSimulation(drawBoard)) {
            UpdateResults();
            wxMessageBox("仿真运行成功！", "成功", wxOK | wxICON_INFORMATION);
        }
    }
}

void SimulationDialog::OnClose(wxCommandEvent& event)
{
    EndModal(wxID_CANCEL);
}

void SimulationDialog::UpdateResults()
{
    // 更新结果显示
    resultText->SetValue(circuitSimulator->GetSimulationResult());

    // 更新信号列表
    signalList->DeleteAllItems();
    auto signalData = circuitSimulator->GetSignalData();

    int index = 0;
    for (const auto& signal : signalData) {
        long itemIndex = signalList->InsertItem(index, signal.first);
        wxString values;
        for (size_t i = 0; i < std::min(signal.second.size(), size_t(10)); i++) {
            values += wxString::Format("%d ", signal.second[i]);
        }
        if (signal.second.size() > 10) values += "...";
        signalList->SetItem(itemIndex, 1, values);
        index++;
    }
}