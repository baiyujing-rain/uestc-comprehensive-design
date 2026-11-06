#pragma once
#include "DrawBoard.h"
#include <numeric>
#include <map>
// Endpoints touching a segment form a junction. Interior-only crossings do not.
inline bool OnSegment(wxPoint p,wxPoint a,wxPoint b) {
    long long dx=b.x-a.x,dy=b.y-a.y,px=p.x-a.x,py=p.y-a.y;
    return px*dy==py*dx&&p.x>=std::min(a.x,b.x)&&p.x<=std::max(a.x,b.x)&&p.y>=std::min(a.y,b.y)&&p.y<=std::max(a.y,b.y);
}
struct NetModel {
    std::vector<wxPoint> points;std::vector<int> parent;std::vector<std::vector<int>> inputs,outputs;
    int Add(wxPoint p){points.push_back(p);parent.push_back((int)parent.size());return (int)parent.size()-1;}
    int Root(int a){return parent[a]==a?a:parent[a]=Root(parent[a]);}
    void Join(int a,int b){a=Root(a);b=Root(b);if(a!=b)parent[b]=a;}
    explicit NetModel(const DrawBoard* b) {
        for(auto& c:b->GetComponents()){std::vector<wxPoint> in,out;GetGatePinPositions(c.type,c.position,in,out);inputs.emplace_back();outputs.emplace_back();for(auto p:in)inputs.back().push_back(Add(p));for(auto p:out)outputs.back().push_back(Add(p));}
        std::vector<std::pair<int,int>> edges;
        for(auto w:b->GetWires()){int a=Add(w.first),z=Add(w.second);Join(a,z);edges.push_back({a,z});}
        for(int i=0;i<(int)points.size();++i){for(int j=0;j<i;++j)if(points[i]==points[j])Join(i,j);for(auto e:edges)if(OnSegment(points[i],points[e.first],points[e.second]))Join(i,e.first);}
    }
};
