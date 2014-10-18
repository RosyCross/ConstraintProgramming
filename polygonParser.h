#ifndef POLYGONPARSER_H_
#define POLYGONPARSER_H_

#include <string>
#include <vector>

class PolygonParser
{
    public:
              PolygonParser(const char* fileName);
             ~PolygonParser();
        bool  init();
        bool  hasNext() { return parseOneLine(fp_, pointVec_); };
        template<typename T, typename Inserter>
        bool  getPointVec(T& data,Inserter inserter)
        {
            for(size_t idx = 0; idx < pointVec_.size(); ++idx)
            {
                insertHelper(inserter, pointVec_[idx]);
            }

            return true;
        }
        template<typename Functor,typename DataType>
        void insertHelper(Functor fun, const DataType& data)
        {
            fun(data);
        }
        
        template<typename Container, typename DataType>
        void insertHelper(std::back_insert_iterator<Container> iter, const DataType& data)
        {
            *iter = data;
        }

        template<typename Container, typename DataType>
        void insertHelper(std::insert_iterator<Container> iter, const DataType& data)
        {
            *iter = data;
        }


    private:
        struct Point 
        {
            int x_;
            int y_;
        };
        PolygonParser(const PolygonParser& rhs) {}      
        bool  open();
        bool  close();

        bool  parseOneLine(FILE *fp, std::vector<Point>& pointVec);
 
        FILE *fp_; 
        char *buffer_;
 std::string  fileName_;
 std::vector<Point> pointVec_;
};
#endif
