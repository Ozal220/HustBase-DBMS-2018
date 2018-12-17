#pragma once

/***************
*该类用于管理一段位图信息
*by mtf
**/
class bitmanager
{
    public:
        bitmanager(int length, char *data);
        virtual ~bitmanager();
        bool atPos(int n);   //取出第n位的值，n从0开始计数
        int firstBit(int n, bool val);   //返回第一个值为0的位的位置
        bool anyZero();  //返回值标识位图中是否有值为0的位
        bool setBitmap(int pos, bool value);   //设置位图某位的值
        //int setEffectiveLength(int length);
		void redirectBitmap(int length, char *data);

    protected:

    private:
        int bmLength;    //位图信息最大长度，该长度以字节为单位
        //char *bitmap;
        char *realData;   //实际的位图位置指针
        //int effectiveLength; //有效长度，该长度以位为单位
};