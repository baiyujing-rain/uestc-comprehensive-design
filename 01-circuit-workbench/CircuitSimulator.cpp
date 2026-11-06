#include "CircuitSimulator.h"
#include "NetModel.h"
#include <queue>
#include <set>
int CircuitSimulator::Evaluate(GateType t,const std::vector<int>& in,int output){
    int ones=0;for(int x:in)ones+=x;
    switch(t){
    case GateType::AND_GATE:return ones==(int)in.size();case GateType::NAND_GATE:return ones!=(int)in.size();
    case GateType::OR_GATE:return ones>0;case GateType::NOR_GATE:return ones==0;
    case GateType::NOT_GATE:return !in.at(0);case GateType::XOR_GATE:return ones%2;case GateType::XNOR_GATE:return !(ones%2);
    case GateType::ADDER:return output?ones>=2:ones%2;case GateType::END_NODE:return in.at(0);default:return 0;}
}
bool CircuitSimulator::RunSimulation(const DrawBoard* b){
    ResetSimulation();if(!b||b->GetComponents().empty()){result=L"请先放置元件并连接引脚。";return false;}
    auto& cs=b->GetComponents();NetModel net(b);int n=(int)cs.size();states.assign(n,-1);
    std::map<int,std::pair<int,int>> drivers;std::vector<std::vector<std::pair<int,int>>> src(n);
    for(int i=0;i<n;++i)for(int j=0;j<(int)net.outputs[i].size();++j){int r=net.Root(net.outputs[i][j]);if(drivers.count(r)){result=L"连接冲突：同一网络存在多个输出驱动，请检查接线。";return false;}drivers[r]={i,j};}
    std::vector<std::set<int>> graph(n);std::vector<int> degree(n,0);
    for(int i=0;i<n;++i)for(int j=0;j<(int)net.inputs[i].size();++j){int r=net.Root(net.inputs[i][j]);auto it=drivers.find(r);if(it==drivers.end()){result=wxString::Format(L"U%d（%s）的输入 %d 悬空，请连接输入信号。",i+1,GateName(cs[i].type),j+1);return false;}src[i].push_back(it->second);if(graph[it->second.first].insert(i).second)++degree[i];}
    std::queue<int> q;for(int i=0;i<n;++i)if(!degree[i])q.push(i);int count=0;
    std::vector<std::vector<int>> values(n);
    while(!q.empty()){int i=q.front();q.pop();++count;std::vector<int> in;for(auto s:src[i])in.push_back(values[s.first][s.second]);
        if(cs[i].type==GateType::START_NODE){if(cs[i].value!="0"&&cs[i].value!="1"){result=L"输入电平必须为 0 或 1。双击输入元件可修改。";return false;}values[i]={cs[i].value=="1"?1:0};}
        else {values[i].push_back(Evaluate(cs[i].type,in));if(cs[i].type==GateType::ADDER)values[i].push_back(Evaluate(cs[i].type,in,1));}
        states[i]=values[i][0];for(int j:graph[i])if(--degree[j]==0)q.push(j);
    }
    if(count!=n){states.assign(n,-1);result=L"检测到反馈回路：当前支持组合逻辑，请移除反馈连接后运行。";return false;}
    result=L"组合逻辑计算完成\n高电平 1 / 低电平 0\n\n";
    for(int i=0;i<n;++i){auto name=wxString::Format("U%d  %s",i+1,GateName(cs[i].type));data[name]=values[i];result+=name+wxString::Format("   %d",states[i]);if(cs[i].type==GateType::ADDER)result+=wxString::Format(L"  进位 %d",values[i][1]);result+="\n";}
    return true;
}
