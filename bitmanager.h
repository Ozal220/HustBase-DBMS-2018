#pragma once

/***************
*�������ڹ���һ��λͼ��Ϣ
*by mtf
**/
class bitmanager
{
    public:
        bitmanager(int length, char *data, int eLength);
        virtual ~bitmanager();
        bool atPos(int n);   //ȡ����nλ��ֵ��n��0��ʼ����
        int firstZero(int n);   //���ص�һ��ֵΪ0��λ��λ��
        bool anyZero();  //����ֵ��ʶλͼ���Ƿ���ֵΪ0��λ
        bool setBitmap(int pos, bool value);   //����λͼĳλ��ֵ
        int setEffectiveLength(int length);

    protected:

    private:
        int bmLength;    //λͼ��Ϣ��󳤶ȣ��ó������ֽ�Ϊ��λ
        //char *bitmap;
        char *realData;   //ʵ�ʵ�λͼλ��ָ��
        int effectiveLength; //��Ч���ȣ��ó�����λΪ��λ
};

