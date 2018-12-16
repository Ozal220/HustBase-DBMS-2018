#include "stdafx.h"
#include "bitmanager.h"
#include <cstdlib>
#include <cstring>

bitmanager::bitmanager(int length, char *data, int eLength)    //��data��ʼ��λͼ��û�м�鳤�����⣬����Խ�磩
{
    if(length<=0||eLength<=0) //ǿ��������С����Ϊ8��������ֵС��0ʱ��
    {
        length=1;
        eLength=8;
    }
    //realData=(char *)malloc(length);
    //memcpy(realData,data,length);
    realData=data;
    bmLength=length;
    effectiveLength=(eLength<=length*8)?eLength:length*8;  //��Ч���Ȳ��ܴ�����󳤶�
}

bitmanager::~bitmanager()
{
    //free(realData);
}

bool bitmanager::atPos(int n)   //ȡ����nλ��ֵ��n��0��ʼ����
{
    if((n>effectiveLength-1)||n<0)
        return false;
    int atChar=n/8;
    char target=*(realData+atChar);
    return (target&(0x01<<(n%8)))==0?false:true;
}

//���شӵ�nλ���һ��ֵΪ0��λ��λ�ã���û����򳬳���Ч��Χ�򷵻�-1
int bitmanager::firstZero(int n)
{
    if(n<0||n>effectiveLength-1)
        return -1;
    int offset=n/8;
    int bitOffset=n%8;
    char test=*(realData+offset);
    while((test==0xff)&&(offset<bmLength))
    {
        test=*(realData+offset);
        offset++;
        bitOffset=0;
    }
    for(;((test&(0x01<<bitOffset))!=0)&&bitOffset<8;bitOffset++);
    if(bitOffset>=8)  //����
        return -1;
    return (offset*8+bitOffset)>(effectiveLength-1)?-1:(offset*8+bitOffset);
}

bool bitmanager::anyZero()  //����ֵ��ʶλͼ���Ƿ���ֵΪ0��λ
{
    int offset;
    for(offset=0;(offset+1)*8<effectiveLength;offset++)
        if(*(realData+offset)!=0xff)
            return true;
    int lastFewBitsOffset=effectiveLength%8-1;
    char mask=0x80; //ʹ��������λ
    if(((*(realData+offset))&(~(mask>>(8-lastFewBitsOffset))))!=(~(mask>>(8-lastFewBitsOffset))))
        return true;
    return false;
}

bool bitmanager::setBitmap(int pos, bool value)   //����λͼĳλ��ֵ
{
    if(pos>effectiveLength-1||pos<0)
        return false;
    int charOffset=pos/8;
    int bitOffset=pos%8;
    char mask=0x01<<bitOffset;
    *(realData+charOffset)=value?(*(realData+charOffset)|mask):(*(realData+charOffset)&(~mask));
    //*(realData+charOffset)=value?(*(realData+charOffset)|mask):(*(realData+charOffset)&(~mask));
    return false;
}

int bitmanager::setEffectiveLength(int length)
{
    if(length<0)
        return -1;
    else if(length>bmLength*8)
        return effectiveLength=bmLength*8;
    return effectiveLength=length;
}