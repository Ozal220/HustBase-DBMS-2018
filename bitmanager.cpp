#include "stdafx.h"
#include "bitmanager.h"
#include <cstdlib>
#include <cstring>

bitmanager::bitmanager(int length, char *data)    //��data��ʼ��λͼ��û�м�鳤�����⣬����Խ�磩
{
    if(length<=0) //ǿ��������С����Ϊ8��������ֵС��0ʱ��
        length=1;
    //realData=(char *)malloc(length);
    //memcpy(realData,data,length);
    realData=data;
    bmLength=length;
    //effectiveLength=(eLength<=length*8)?eLength:length*8;  //��Ч���Ȳ��ܴ�����󳤶�
}

bitmanager::~bitmanager()
{
    ;//free(realData);
}

bool bitmanager::atPos(int n)   //ȡ����nλ��ֵ��n��0��ʼ����
{
    if((n>bmLength*8-1)||n<0)
        return false;
    int atChar=n/8;
    unsigned char target=*(realData+atChar);
    return (target&(0x01<<(n%8)))==0?false:true;
}

//���شӵ�nλ���һ��ֵΪ0/1��λ��λ�ã���û����򳬳���Ч��Χ�򷵻�-1
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
    if(bitOffset>=8)  //����
        return -1;
    return (offset*8+bitOffset);
}

bool bitmanager::anyZero()  //����ֵ��ʶλͼ���Ƿ���ֵΪ0��λ
{
    int offset;
    for(offset=0;offset<bmLength;offset++)
        if((unsigned char)(*(realData+offset))!=0xff)
            return true;
    //int lastFewBitsOffset=effectiveLength%8-1;
    //char mask=0x80; //ʹ��������λ
    //if(((*(realData+offset))&(~(mask>>(8-lastFewBitsOffset))))!=(~(mask>>(8-lastFewBitsOffset))))
    //    return true;
    return false;
}

bool bitmanager::setBitmap(int pos, bool value)   //����λͼĳλ��ֵ
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
    if(length<=0) //ǿ��������С����Ϊ8��������ֵС��0ʱ��
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