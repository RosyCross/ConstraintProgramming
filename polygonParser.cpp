#include "polygonParser.h"
#include <stdlib.h>
#define _WITH_GETLINE
#include <stdio.h>
#include <memory.h>

//STL
#include <iterator>

#ifdef DBG
#define DBG_PRINT(format, args...) printf("[%s:%d] "format, __FILE__, __LINE__, ##args)
#else
#define DBG_PRINT(format, args...)
#endif

/*
int main(int argc, char** argv)
{
    int a = 1;
    DBG_PRINT("HAHA:%d\n",a);
    return EXIT_SUCCESS;
}
*/
//===================================================
PolygonParser::PolygonParser(const char* fileName):fp_(NULL),buffer_(NULL)
{
    if( NULL == fileName || '\0' == *fileName ) return;

    fileName_ = fileName;
}
//===================================================
PolygonParser::~PolygonParser()
{
    std::vector<Point>().swap(pointVec_);
    close();
    fp_ = NULL;
    if (buffer_)
    {
        free(buffer_);
        buffer_ = NULL;
    }
}
//===================================================
bool PolygonParser::open()
{
    if (fileName_.empty()) return false;
 
    if ( NULL==( fp_=fopen(fileName_.c_str(), "r") ) )
        return false;

    return true; 
}
//===================================================
bool PolygonParser::close()
{
    if (NULL==fp_) return true;

    if (0 == fclose(fp_) )
        return false;

    return true;
}
//===================================================
bool PolygonParser::init()
{
    //clear point buffer any way
    pointVec_.clear();

    if ( fileName_.empty() ) return false;
    if ( NULL == fp_ && !open() ) return false;

    if ( NULL == buffer_ )
    {
        buffer_ = (char*)malloc(sizeof(char)*1024);
    }

    //should be in the status of: the file is opened.
    //see if the file read pointer is at the beginning
    bool rslt = false;
    long pos = -1L;
    if ( 0 < (pos = ftell(fp_)) )
    {
        if ( 0 != pos && 0 == fseek(fp_,0,SEEK_SET) )
        {
            rslt = true;
        }
    }
    else if ( 0 == pos )
        rslt = true;

    return rslt && (NULL != buffer_);
}
//===================================================
bool PolygonParser::parseOneLine(FILE *fp, std::vector<Point>& pointVec)
{
    if ( NULL == fp ) return false;

    pointVec.clear();

    const char brace[2] = {'(', ')'}; 
    size_t bufSize  = 1024;
    ssize_t readCnt = 0;
    bool found = false;
    while(!found)
    {
        readCnt = getline(&buffer_, &bufSize, fp_);
        if ( -1 == readCnt || bufSize > 1024 ) break;
    
        char tmpBuf[1024];
        Point pt;
        char* posStart = buffer_;
        char* posEnd   = buffer_;
        while(NULL != posStart && '\0' != *posStart )
        {
            //skip space to left brace
            while (NULL!=posStart && '\0'!=*posStart && brace[0] != *posStart)
                ++posStart;
            if (NULL==posStart || '\0' == *posStart ) break;
    
            posEnd = ++posStart;
            while (NULL!=posEnd && '\0'!=*posEnd && brace[1] != *posEnd)
                ++posEnd;
            if (NULL==posEnd || '\0' == *posEnd) break;
    
            memcpy(tmpBuf, posStart, posEnd - posStart);
            tmpBuf[posEnd - posStart] = '\0';
            sscanf(tmpBuf, "%d , %d", &pt.x_, &pt.y_);
            DBG_PRINT("Check: %s | %d %d\n", tmpBuf, pt.x_,pt.y_);
    
            pointVec.push_back(pt);
            posStart = ++posEnd;
            found = true;
        } 
    }
    return (found && -1 != readCnt && bufSize <= 1024);
}
