#pragma once

/***************
*�������ڹ���һ��λͼ��Ϣ
*by mtf
**/
class bitmanager
{
    public:
        bitmanager(int length, char *data);
        virtual ~bitmanager();
        bool atPos(int n);   //ȡ����nλ��ֵ��n��0��ʼ����
        int firstBit(int n, bool val);   //���ص�һ��ֵΪ0��λ��λ��
        bool anyZero();  //����ֵ��ʶλͼ���Ƿ���ֵΪ0��λ
        bool setBitmap(int pos, bool value);   //����λͼĳλ��ֵ
        //int setEffectiveLength(int length);
		void redirectBitmap(int length, char *data);

    protected:

    private:
        int bmLength;    //λͼ��Ϣ��󳤶ȣ��ó������ֽ�Ϊ��λ
        //char *bitmap;
        char *realData;   //ʵ�ʵ�λͼλ��ָ��
        //int effectiveLength; //��Ч���ȣ��ó�����λΪ��λ
};