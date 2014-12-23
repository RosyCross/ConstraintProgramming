#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <vector>
#include <map>

#if __clang__
   #if __cplusplus >= 201103L
   //cannot proceed because there is no unorderd_map!!
   //it exists in tr1/ directory. so even if you use -std=c++11
   //along with __cpluscplus(version for checking if it is c++11),
   //the include directive still cannot find the include file.
   //    #include <unorderd_map>
   #else
       #include <tr1/unordered_map>
       namespace STDTR1 = std::tr1;
   #endif
#else
   #if __cplusplus >= 201103L
   //    #include <unorderd_map>
   #else
       #include <tr1/unordered_map>
       namespace STDTR1 = std::tr1;
   #endif
#endif

#include "geomObj.h"
#include "graphLib.hpp"
#include "graphUtil.hpp"

namespace 
{
    class Transistor; // forward declaration for typedefs
    typedef std::vector<size_t> TxIdVec;
    typedef std::vector<Transistor> TxVec;

    class Transistor
    {
        public:
            Transistor(IntType x1, IntType y1,IntType x2, IntType y2)
            { 
                lowerLeft_.x_  = x1;  
                lowerLeft_.y_  = y1;
                upperRight_.x_ = x2;
                upperRight_.y_ = y2;
            };

            Transistor(Point lowerLeft, Point upperRight):
            lowerLeft_(lowerLeft), upperRight_(upperRight)
            {}
           
            const Point& getLowerLeft()  { return lowerLeft_;};
            const Point& getUpperRight() { return upperRight_;}
            
        private:
        Point lowerLeft_;
        Point upperRight_;
    };
 
    class TxNode
    {
        public:
            explicit TxNode(long index):index_(index) {};
            long getIndex() { return index_; };

        private:
            long index_;
    };

    class CstEdge 
    {
        public:
            CstEdge(Point refPtFrom, Point refPtTo): 
            refPtFrom_(refPtFrom), refPtTo_(refPtTo) 
            { weight_ = refPtTo.x_ - refPtFrom.x_;};

            const Point& getRefPtFrom() { return refPtFrom_; }
            const Point& getRefPtTo()   { return refPtTo_; }
        
            int weight_;
        private:
            Point refPtFrom_;
            Point refPtTo_;    
    };

    //Transistor Alignment Constraint
    class TxAlignCst
    {
        public:
            //some typedefs for enhancing encapsulation
            typedef TxIdVec::iterator memIter;
            typedef TxIdVec::const_iterator memcIter;


        public:
            TxAlignCst() {};
            void addMember(size_t id) { idVec_.push_back(id); } 
            memIter  begin() { return idVec_.begin(); }
            memIter  end()   { return idVec_.end();}
            memcIter cbegin() const { return idVec_.begin(); }
            memcIter cend()   const { return idVec_.end();}

        private:
            TxIdVec idVec_;
    };
}
#include<bits/stl_tree.h>
int main(int argc, char* argv[])
{
    //std::_Rb_tree<int,int,std::_Identity<int>, std::less<int> > aa;
//    |       |
//   =1=======5=
//    |       |
//   =2= =4= =6=
//    |       |
//   =3=     =7=
//    |       | 
//
//   
    //=============================================
    // txVec is a container for transistor data storage
    //=============================================
    TxVec txVec;
    txVec.reserve(8);

    txVec.push_back(Transistor(1,1,2,2));
    txVec.push_back(Transistor(1,6,2,7));
    txVec.push_back(Transistor(1,11,2,12));

    txVec.push_back(Transistor(3,6,4,7));

    txVec.push_back(Transistor(5,1,6,2));
    txVec.push_back(Transistor(5,6,6,7));
    txVec.push_back(Transistor(5,11,6,12));
    //this one is going to be inserted between 4 and 6
    txVec.push_back(Transistor(3,6,4,7));

    //============================================
    // Build up constraint graph.
    //============================================
    std::vector<GraphLib::IdType> gIdVec;
    gIdVec.reserve(txVec.size());

    GraphLib::Graph<TxNode,CstEdge> graph;
    GraphLib::IdType startId = graph.addNode(TxNode(-1));
 
    for(int idx = 0; idx < txVec.size(); ++idx)
    {
        gIdVec.push_back( graph.addNode(TxNode(idx)) );
    }    

    Point dummyZero = {0,0};
    for(int idx = 0; idx < txVec.size(); ++idx)
    {
        graph.addEdge(startId, gIdVec[idx], CstEdge(dummyZero, dummyZero));
    }   

    graph.addEdge(gIdVec[0], gIdVec[1], CstEdge(txVec[0].getLowerLeft(), txVec[1].getLowerLeft()));
    graph.addEdge(gIdVec[1], gIdVec[0], CstEdge(txVec[1].getLowerLeft(), txVec[0].getLowerLeft()));

    graph.addEdge(gIdVec[1], gIdVec[2], CstEdge(txVec[1].getLowerLeft(), txVec[2].getLowerLeft()));
    graph.addEdge(gIdVec[2], gIdVec[1], CstEdge(txVec[2].getLowerLeft(), txVec[1].getLowerLeft()));

//======================Middle
    graph.addEdge(gIdVec[0], gIdVec[3], CstEdge(txVec[0].getLowerLeft(), txVec[3].getLowerLeft()));
    graph.addEdge(gIdVec[3], gIdVec[0], CstEdge(txVec[3].getLowerLeft(), txVec[0].getLowerLeft()));
    graph.addEdge(gIdVec[1], gIdVec[3], CstEdge(txVec[1].getLowerLeft(), txVec[3].getLowerLeft()));
    graph.addEdge(gIdVec[3], gIdVec[1], CstEdge(txVec[3].getLowerLeft(), txVec[1].getLowerLeft()));
    graph.addEdge(gIdVec[2], gIdVec[3], CstEdge(txVec[2].getLowerLeft(), txVec[3].getLowerLeft()));
    graph.addEdge(gIdVec[3], gIdVec[2], CstEdge(txVec[3].getLowerLeft(), txVec[2].getLowerLeft()));

//important
    GraphLib::IdType t1 =
    graph.addEdge(gIdVec[3], gIdVec[7], CstEdge(txVec[3].getLowerLeft(), txVec[7].getLowerLeft()));
    GraphLib::IdType t2 =
    graph.addEdge(gIdVec[7], gIdVec[3], CstEdge(txVec[7].getLowerLeft(), txVec[3].getLowerLeft()));

    graph.getEdgeData(t1).userData_.weight_ = 7;
    graph.getEdgeData(t2).userData_.weight_ = -7;

// connect to the right most alignments
    t1 =
    graph.addEdge(gIdVec[7], gIdVec[4], CstEdge(txVec[7].getLowerLeft(), txVec[4].getLowerLeft())); graph.getEdgeData(t1).userData_.weight_ = 3;
    t1 =
    graph.addEdge(gIdVec[4], gIdVec[7], CstEdge(txVec[4].getLowerLeft(), txVec[7].getLowerLeft())); graph.getEdgeData(t1).userData_.weight_ = -3;
    t1 =
    graph.addEdge(gIdVec[7], gIdVec[5], CstEdge(txVec[7].getLowerLeft(), txVec[5].getLowerLeft())); graph.getEdgeData(t1).userData_.weight_ = 3;
    t1 = 
    graph.addEdge(gIdVec[5], gIdVec[7], CstEdge(txVec[5].getLowerLeft(), txVec[7].getLowerLeft())); graph.getEdgeData(t1).userData_.weight_ = -3;
    t1 =
    graph.addEdge(gIdVec[7], gIdVec[6], CstEdge(txVec[7].getLowerLeft(), txVec[6].getLowerLeft())); graph.getEdgeData(t1).userData_.weight_ = 3;
    t1 = 
    graph.addEdge(gIdVec[6], gIdVec[7], CstEdge(txVec[6].getLowerLeft(), txVec[7].getLowerLeft())); graph.getEdgeData(t1).userData_.weight_ = -3;



//=======================
    graph.addEdge(gIdVec[4], gIdVec[5], CstEdge(txVec[4].getLowerLeft(), txVec[5].getLowerLeft()));
    graph.addEdge(gIdVec[5], gIdVec[4], CstEdge(txVec[5].getLowerLeft(), txVec[4].getLowerLeft()));

    graph.addEdge(gIdVec[5], gIdVec[6], CstEdge(txVec[5].getLowerLeft(), txVec[6].getLowerLeft()));
    graph.addEdge(gIdVec[6], gIdVec[5], CstEdge(txVec[6].getLowerLeft(), txVec[5].getLowerLeft()));

bool updated = false;
typedef GraphLib::Graph<TxNode,CstEdge> CstGraph;
typedef STDTR1::unordered_map<CstGraph::NodeType*, int> NodeDistMap;
NodeDistMap dist;
dist[&(graph.getNodeData(startId))] = 0;
printf("Node Count:%u EdgeCount:%u\n", graph.nodeCount(), graph.edgeCount());
for(int idx = 0; idx < graph.nodeCount() ; ++idx)
{
    updated = false;
    CstGraph::EdgeIdGen edgeIdGen(graph);
    while(!edgeIdGen.isDone())
    {
        CstGraph::EdgeType& e = graph.getEdgeData(edgeIdGen.getCurrent());
        GraphLib::IdType fromId = e.fromId_;
        GraphLib::IdType toId   = e.toId_;
 
        CstGraph::NodeType& from = graph.getNodeData(e.fromId_);
        CstGraph::NodeType& to   = graph.getNodeData(e.toId_);
        printf("edgeId:%d from:%d to:%d w:%d\n",edgeIdGen.getCurrent().val(), e.fromId_.val(), (int)e.toId_, e.userData_.weight_);
        if(dist.end() == dist.find(&from))
            dist[&from] = INT_MAX;
        if(dist.end() == dist.find(&to))
            dist[&to] = INT_MAX;
 
        if( dist[&from] != INT_MAX &&
            dist[&to] > dist[&from] + e.userData_.weight_
          )
        {
            dist[&to] = dist[&from] + e.userData_.weight_;
            updated = true;
        }
        edgeIdGen.goNext();
    }
//        }//for 
//    }//for
}
if(updated)
    printf("Negative Cycle, No solutions\n");

for(NodeDistMap::iterator iter = dist.begin();
    dist.end() != iter;
    ++iter
   )
{
    printf("node:%d  dist:%d\n", (int)(*(*iter).first).id_, (*iter).second);
}
//std::map<GraphLib::IdType,GraphLib::IdType> ans;
//bool k = GraphUtil::longestPath(graph, startId, ans);
//printf("%d\n",k);    
//printf("%d\n",__clang__);
    return EXIT_SUCCESS;
}
