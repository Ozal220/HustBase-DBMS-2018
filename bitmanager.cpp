#include "stdafx.h"
#include "bitmanager.h"
#include <cstdlib>
#include <cstring>

bitmanager::bitmanager(int length, char *data)    //以data初始化位图（没有检查长度问题，可能越界）
{
    if(length<=0) //强行限制最小长度为8（当输入值小于0时）
        length=1;
    //realData=(char *)malloc(length);
    //memcpy(realData,data,length);
    realData=data;
    bmLength=length;
    //effectiveLength=(eLength<=length*8)?eLength:length*8;  //有效长度不能大于最大长度
}

bitmanager::~bitmanager()
{
    ;//free(realData);
}

bool bitmanager::atPos(int n)   //取出第n位的值，n从0开始计数
{
    if((n>bmLength*8-1)||n<0)
        return false;
    int atChar=n/8;
    unsigned char target=*(realData+atChar);
    return (target&(0x01<<(n%8)))==0?false:true;
}

//返回从第n位起第一个值为0/1的位的位置，若没有则或超出有效范围则返回-1
int bitmanager::firstBit(int n, bool val)
{
    if(n<0||n>bmLength*8-1)
        return -1;
    int offset=n/8;
    int bitOffset=n%8;
    unsigned char test=*(realData+offset);
    unsigned char cmp=val?0x00:0xff;
    while(((test>>bitOffset)==(cmp>>bitOffset))&&(offset<bmLength))
    {
		offset++;
        test=*(realData+offset);
        bitOffset=0;
    }
    if(val)
        for(;((test&(0x01<<bitOffset))==0)&&bitOffset<8;bitOffset++);
    else
        for(;((test&(0x01<<bitOffset))!=0)&&bitOffset<8;bitOffset++);
    if(bitOffset>=8)  //出错
        return -1;
    return (offset*8+bitOffset);
}

bool bitmanager::anyZero()  //返回值标识位图中是否有值为0的位
{
    int offset;
    for(offset=0;offset<bmLength;offset++)
        if((unsigned char)(*(realData+offset))!=0xff)
            return true;
    //int lastFewBitsOffset=effectiveLength%8-1;
    //char mask=0x80; //使用算术移位
    //if(((*(realData+offset))&(~(mask>>(8-lastFewBitsOffset))))!=(~(mask>>(8-lastFewBitsOffset))))
    //    return true;
    return false;
}

bool bitmanager::setBitmap(int pos, bool value)   //设置位图某位的值
{
    if(pos>bmLength*8-1||pos<0)
        return false;
    int charOffset=pos/8;
    int bitOffset=pos%8;
    unsigned char mask=0x01<<bitOffset;
    *(realData+charOffset)=value?(*(realData+charOffset)|mask):(*(realData+charOffset)&(~mask));
    //*(realData+charOffset)=value?(*(realData+charOffset)|mask):(*(realData+charOffset)&(~mask));
    return false;
}

void bitmanager::redirectBitmap(int length, char *data)
{
    if(length<=0) //强行限制最小长度为8（当输入值小于0时）
        length=1;
    realData=data;
    bmLength=length;
}


/*
int bitmanager::setEffectiveLength(int length)
{
    if(length<0)
        return -1;
    else if(length>bmLength*8)
        return effectiveLength=bmLength*8;
    return effectiveLength=length;
}
*/