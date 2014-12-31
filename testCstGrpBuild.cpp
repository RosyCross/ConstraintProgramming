#include "segmentTree.hpp"
#include "polygonParser.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <vector>
#include <list>
#include <algorithm>
#include <limits>

//for memset
#include <string.h>

#include "graphLib.hpp"

//==========================================
//OVERVIWE:
// This file is for the proof-of-concept of the following:
// 1. keep a constraint graph and use it to fulfill the needs to
//    place polygons in certain manner. 
// 2. when there are several polygons, build up a constraint graph. it is
//    built according to if one polygon may affect another polygon while 
//    alter the first polygon.
// 3. To have a feeling that when there is a new polygon placed into
//    the existing polygons, how do we do minimal changes to organize
//    the polygons in certain manner(avoid overlap).
//    The basic idea is first find out the polygons overlapped with
//    the newly add polygon. The second step is to find the possible affected
//    other polygons when adjust positions of the polygons found in the first
//    step. The final step is to build a constraint graph for these polygons
//    and solve it.
//
//DATA STRUCTURE:
// constraint graph: node==> a pointer to the record in the polygon list. 
//                   edge==> indicate constraint info
// segment tree    : it is used to find the possible affected polygons   
// polygon record list : contains polygon info and info to the corresponding 
//                       graph node(this is bi-directional)
// Constraint Graph
//
//         O ------------>O                         [___] BST tree 
//                \
//                 V                            [___]   [___]
//  O ---> O -----> O ------> O                   |     
//   \               ^    /^  |                   |        [___]
//    \               \ /     |                   |
//     \--> O -------> O -----|------o------------------[___]  [___]
//          |                 |      |            |              |
//          o---|        o ---|      |            |   o----------o
//          V   o------- V --------- V ---------- o   V
//        nodeIter     nodeIter      nodeIter      nodeIter
//      PolygonRec -> PolygonRec -> PolygonRec -> PolygonRec -> .... 
//        Polygon      Polygon       Polygon       Polygon 
//                      -------
//                      |     |
//         ---          |  D  |
//         | |          |     |
//         | |  |--|    ------- 
//         | |  |  |         |--|
//         |B|  |  o-------  | E| 
//         | |  | C       |  |--|
//         | |  |         |     -----
//         | |  |---------|     | F |
//         | |                  ----- 
// |   |-| | |                  
// |   |A| | |
// |   |-| ---
// |
// +------
//
//
namespace STD = std;

namespace {
struct Point 
{
    int x_;
    int y_;
};
typedef std::vector<Point> Polygon;
typedef std::list<Polygon> PolygonList;
class DataTrans
{
public:
DataTrans(Polygon& input):plygn_(input){};

template<typename T>
void operator()(const T& data)
{
   tmpPoint.x_ = data.x_;
   tmpPoint.y_ = data.y_;
   plygn_.push_back(tmpPoint);
}

Polygon& getData(){return plygn_;}  

private:
   Polygon& plygn_;
   Point tmpPoint;
};

//this is a temporary container for sorting all the vertical segments of polygons
struct Segment
{
    Point head_;
    Point tail_;
};


//this is the storage of polygon. And it provides bi-directional accessing
//info to polygon info and Node identification.
typedef STD::pair<void*,GraphLib::IdType> PolygonLinkRec;
typedef STD::list<PolygonLinkRec> LinkRecList;
struct VerSegCntnr
{
    Segment seg_;
    PolygonLinkRec* linkRecPtr_;
};
bool sortVerSegByRevX(const VerSegCntnr& a, const VerSegCntnr& b)
{
    return a.seg_.head_.x_ > b.seg_.head_.x_;
}
typedef STD::vector<SegmentTreeNode*> StNodePtrVec;

class Collector
{
    public:
        Collector(StNodePtrVec& nodePtrVec,PolygonLinkRec* linkRecPtr,SegCoord refCoord):nodePtrVec_(nodePtrVec), linkRecPtr_(linkRecPtr),refCoord_(refCoord){}

   void operator()(SegmentTreeNode& treeNode)
        {   //on left, with the extreme case of having same coordinate
            SegmentTreeNode*& nodePtr =
            refCoord_ >= treeNode.getRefCoord() ? nodePtrVec_[0] : nodePtrVec_[1];
            if (treeNode.getId() == linkRecPtr_ ) return;

            if ( NULL == nodePtr || 
                 ((refCoord_>=treeNode.getRefCoord()? 
                   treeNode.getRefCoord()>nodePtr->getRefCoord(): 
                   treeNode.getRefCoord()<nodePtr->getRefCoord())
                 )
               ) 
            {
                nodePtr = &treeNode;
            }
        }
    private:
        StNodePtrVec& nodePtrVec_;       
        PolygonLinkRec* linkRecPtr_;
        SegCoord refCoord_;
};

//======== Graph Definition =============//
class Gnode
{
    public:
        Gnode(PolygonLinkRec* linkRecPtr):linkRecPtr_(linkRecPtr){} 
    PolygonLinkRec* getLinkRecPtr() { return linkRecPtr_; }
        void setLinkRecPtr(PolygonLinkRec* linkRecPtr) { linkRecPtr_=linkRecPtr;}

    private:
        PolygonLinkRec *linkRecPtr_;
};
typedef GraphLib::Graph<Gnode,int> CnstrntGraph;

void printPolygon(const Polygon& plygn)
{
   for (STD::vector<Point>::const_iterator it=plygn.begin();
        it != plygn.end();
        ++it
       )
   {
       printf(" (%d,%d)-", (*it).x_, (*it).y_);
   }
   printf("end");
}

void dfsPrint(CnstrntGraph& graph,GraphLib::IdType& currId, int* dist)
{
    if ( NULL == dist ) return;

printf("node id:%d\n", currId.val());
    CnstrntGraph::IdIter idIter = graph.beginNodeIter(currId); 
    CnstrntGraph::IdIter end    = graph.endNodeIter(currId); 
    while( idIter != end)
    {
        //mainly for preventing printing dummy nodes bcuz it contains NULL polygon record
        if (NULL == (Polygon*)(graph.getNodeData(currId).userData_.getLinkRecPtr()) )
            printf("Dummy Node ");
        else
            printPolygon( *(Polygon*)(graph.getNodeData(currId).userData_.getLinkRecPtr()->first) ); 
        printf(" <=>%d=%d=%d<=> ",dist[currId], dist[*idIter]-dist[currId],dist[*idIter]);
        if( NULL == (Polygon*)(graph.getNodeData(*idIter).userData_.getLinkRecPtr()) )
            printf("Dummy Node "); 
        else
            printPolygon( *(Polygon*)(graph.getNodeData(*idIter).userData_.getLinkRecPtr()->first) );
        printf("\n");

        dfsPrint(graph, (*idIter), dist);
        ++idIter;
    }

}

} //anonymous namespace

// test template property
void myF() { printf("function\n"); }
struct mySF
{
    void operator()() const
    {
        printf("structure\n");
    }
};
/*
//test
#include <typeinfo>
#include <string>
#include <cxxabi.h>
//std::string demangle(const char* p)
template<typename T>
void test(T& functor)
{
    int status = -1;
    printf("TYPE:%s %s\n",typeid(T).name(),abi::__cxa_demangle(typeid(T).name(),0,0,&status) );
    functor();
}
*/

int main(int argc, char* argv[])
{
/*
mySF gg;
test(&myF);
test(gg);
*/
    PolygonList plygnList;
    PolygonParser plygnPrsr("polygon.txt");
    if (plygnPrsr.init())
    {
        Polygon tmpPlygn;
        DataTrans tmp(tmpPlygn);
        while (plygnPrsr.hasNext())
        {
            //tmpPlygn.clear();
            tmp.getData().clear();
            plygnPrsr.getPointVec(tmp,tmp);
            plygnList.push_back(tmp.getData());
        }
    }

    VerSegCntnr tmpCntnr;
    STD::vector<VerSegCntnr> segInfoVec;
    GraphLib::IdType invalidId(STD::numeric_limits<int>::max());
    PolygonLinkRec linkRec;
    LinkRecList linkRecList;
    size_t verSegCnt = 0;
    size_t totalVerSegCnt = 0;
    for (STD::list<Polygon>::iterator it=plygnList.begin();
         it != plygnList.end(); ++it )
    {
        assert(0 == (*it).size() % 1 && 4 <= (*it).size() );
        if (!(0==(*it).size() %1 && 4<= (*it).size()) ) continue;
        linkRecList.push_back( STD::make_pair(&(*it), invalidId) );

        verSegCnt = (*it).size() >> 2;
        totalVerSegCnt += verSegCnt;
        tmpCntnr.linkRecPtr_ = &(linkRecList.back());

        //assume the first point is the most lowerleft point
        //O(n) complexity
        //the trap is: itA+=2 could be an action of 2,not as simple as adding.
        Polygon::const_iterator itA = (*it).begin()+1;
        Polygon::const_iterator itB = itA+1; 
        for (;itA != (*it).end() && itB != (*it).end();
              itA+=2,itB+=2)
        {
            tmpCntnr.seg_.head_ = ((*itA).y_<=(*itB).y_) ? (*itA) : (*itB);
            tmpCntnr.seg_.tail_ = ((*itA).y_<=(*itB).y_) ? (*itB) : (*itA);
            printf("PUSH (%d,%d)-(%d,%d)\n", tmpCntnr.seg_.head_.x_,tmpCntnr.seg_.head_.y_,tmpCntnr.seg_.tail_.x_,tmpCntnr.seg_.tail_.y_);
            segInfoVec.push_back(tmpCntnr);
        }
        itB = (*it).begin();
        tmpCntnr.seg_.head_ = ((*itA).y_<=(*itB).y_) ? (*itA) : (*itB);
        tmpCntnr.seg_.tail_ = ((*itA).y_<=(*itB).y_) ? (*itB) : (*itA);
        printf("PUSH (%d,%d)-(%d,%d)\n", tmpCntnr.seg_.head_.x_,tmpCntnr.seg_.head_.y_,tmpCntnr.seg_.tail_.x_,tmpCntnr.seg_.tail_.y_);
        segInfoVec.push_back(tmpCntnr);
    }

    //sort reversely
    STD::sort(segInfoVec.begin(), segInfoVec.end(),sortVerSegByRevX);

    StNodePtrVec treeNodePtrVec(2,static_cast<StNodePtrVec::value_type>(NULL));
    //Collector collector(treeNodePtrVec);
    SegmentTree st; 
    CnstrntGraph graph;
    PolygonLinkRec *leftRecPtr = NULL, *rightRecPtr = NULL, *middleRecPtr = NULL;
    for(size_t idx = 0; idx < segInfoVec.size(); ++idx )
    {
        //collect overlapped datas
        treeNodePtrVec.clear();
        Collector collector(treeNodePtrVec,segInfoVec[idx].linkRecPtr_,segInfoVec[idx].seg_.head_.x_);
        st.getOverlap(segInfoVec[idx].seg_.head_.y_,segInfoVec[idx].seg_.tail_.y_, collector);
        //use the collected datas to build contraints
        if ( treeNodePtrVec[0] || treeNodePtrVec[1] )
        {
            middleRecPtr = segInfoVec[idx].linkRecPtr_;
            if (invalidId == middleRecPtr->second )
                middleRecPtr->second = graph.addNode(Gnode(middleRecPtr));

            printf("Ref:%d %d %d \n", segInfoVec[idx].seg_.head_.y_, segInfoVec[idx].seg_.tail_.y_,segInfoVec[idx].seg_.tail_.x_);
            if (treeNodePtrVec[0])
            {
                printf("left:%d,%d,%d\n", treeNodePtrVec[0]->getStartCoord(), treeNodePtrVec[0]->getEndCoord(), treeNodePtrVec[0]->getRefCoord());         
                leftRecPtr = static_cast<PolygonLinkRec*>(treeNodePtrVec[0]->getId());
                if ( invalidId == leftRecPtr->second )
                    leftRecPtr->second = graph.addNode(Gnode(leftRecPtr));

                SegCoord diff =  segInfoVec[idx].seg_.head_.x_ - treeNodePtrVec[idx]->getRefCoord();
                GraphLib::IdType edgeId;
                //My personal constraint: only one constraint
                if (graph.beginNodeIter(leftRecPtr->second) == graph.endNodeIter(leftRecPtr->second))
                    edgeId = graph.addEdge(leftRecPtr->second, middleRecPtr->second, diff);
                else
                    edgeId = graph.getEdgeId(leftRecPtr->second,middleRecPtr->second);

                if (graph.getEdgeData(edgeId).userData_ > diff) 
                {
                    graph.getEdgeData(edgeId).userData_ = diff;
                }
            }
            if (treeNodePtrVec[1])
            {
                printf("right:%d,%d,%d\n", treeNodePtrVec[1]->getStartCoord(), treeNodePtrVec[1]->getEndCoord(), treeNodePtrVec[1]->getRefCoord());
                rightRecPtr = static_cast<PolygonLinkRec*>(treeNodePtrVec[1]->getId()); 
                if ( invalidId == rightRecPtr->second )
                    rightRecPtr->second = graph.addNode(Gnode(rightRecPtr)); 

                SegCoord diff =  treeNodePtrVec[1]->getRefCoord() - segInfoVec[idx].seg_.head_.x_;
printf("diff:%d\n",diff);
                GraphLib::IdType edgeId;
                //My personal constraint: only one edge
                if (graph.beginNodeIter(middleRecPtr->second) == graph.endNodeIter(middleRecPtr->second))
                    edgeId = graph.addEdge(middleRecPtr->second, rightRecPtr->second, diff);
                else
                    edgeId = graph.getEdgeId(middleRecPtr->second, rightRecPtr->second);

                if (graph.getEdgeData(edgeId).userData_ > diff) 
                {
                    graph.getEdgeData(edgeId).userData_ = diff;
                    graph.getEdgeData(edgeId).toId_ = rightRecPtr->second;
                }
            }
        }

        st.insert_equal( SegmentTreeNode(segInfoVec[idx].seg_.head_.y_, 
                                         segInfoVec[idx].seg_.tail_.y_,
                                         segInfoVec[idx].seg_.head_.x_,
                                         (void*)(segInfoVec[idx].linkRecPtr_)));

        treeNodePtrVec.assign(2,static_cast<StNodePtrVec::value_type>(NULL));
    } 

    printf("NodeCount:%d EdgeCount:%d\n", graph.nodeCount(),graph.edgeCount());
    CnstrntGraph::EdgeIdGen edgeIdGenObj(graph);
    while(edgeIdGenObj.hasNext()) 
    {
        CnstrntGraph::EdgeType& edge = graph.getEdgeData(edgeIdGenObj.getNext());
        CnstrntGraph::NodeType& from = graph.getNodeData(edge.fromId_);
        CnstrntGraph::NodeType& to = graph.getNodeData(edge.toId_);
       
        printf("edge Value:%d shape1:size:%d x:%d, shape2:size:%d x:%d\n",
                edge.userData_, 
                ((Polygon*)(from.userData_.getLinkRecPtr()->first))->size(),
                ((Polygon*)(from.userData_.getLinkRecPtr()->first))->front().x_,
                ((Polygon*)(to.userData_.getLinkRecPtr()->first))->size(),
                ((Polygon*)(to.userData_.getLinkRecPtr()->first))->front().x_
              );
    }

#if 0
// this code snippet is testing that if all the posotion of polygons are all
// altered after the constraint grah is built, this solving system can get
// all the polygons back to where it is such that the constraints are 
// satisfied.
    int shift = 10;
    for (PolygonList::iterator iter = ++plygnList.begin(); iter != plygnList.end(); ++iter)
    {
        for ( Polygon::iterator iter2 = (*iter).begin(); iter2 != (*iter).end(); ++iter2)
        {
            (*iter2).x_ = (*iter2).x_ + shift;
            (*iter2).y_ = (*iter2).y_ + shift;
        }
        shift+=10;
    }
#endif
    //add a fake source
    GraphLib::IdType source = graph.addNode(Gnode(NULL));
    graph.addEdge(source, linkRecList.front().second, 0);

    //longest path
    edgeIdGenObj.init();
    int dist[ graph.nodeCount() ];
    memset(dist,-1,sizeof(dist));
    int parent[graph.nodeCount()];
    dist[source] = 0;
    parent[source] = source;
    for (int count = 0; count < graph.nodeCount(); ++count )
    {
        while (edgeIdGenObj.hasNext())
        {
            CnstrntGraph::EdgeType& edge = graph.getEdgeData(edgeIdGenObj.getNext());
            if ( dist[edge.toId_] < dist[edge.fromId_] + edge.userData_ )
            {
                dist[edge.toId_] = dist[edge.fromId_] + edge.userData_;
                parent[edge.toId_] = edge.fromId_;
            }

        }
        edgeIdGenObj.init();
    }

    edgeIdGenObj.init();
    while (edgeIdGenObj.hasNext())
    {
        CnstrntGraph::EdgeType& edge = graph.getEdgeData(edgeIdGenObj.getNext());
        printf("from:%d dist:%d to:%d dist:%d\n",
                edge.fromId_.val(), dist[edge.fromId_],
                edge.toId_.val(), dist[edge.toId_]
              );
    }

    //for verification
    printf("Start printing result for verification\n");
    dfsPrint(graph,source,dist);    
    printf("End printing result for verification\n");
     
#if 0 
    for (STD::list<Polygon>::const_iterator it=plygnList.begin();
         it != plygnList.end();
         ++it
        )
    {
        for (STD::vector<Point>::const_iterator it2=(*it).begin();
             it2 != (*it).end();
             ++it2
            )
        {
            printf(" (%d %d)-", (*it2).x_, (*it2).y_);
        }
        printf("\n");
    }
#endif
    return EXIT_SUCCESS;
}
