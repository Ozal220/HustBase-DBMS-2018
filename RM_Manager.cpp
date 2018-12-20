#include "stdafx.h"
#include "RM_Manager.h"
#include "str.h"
#include <cstring>
#include <bitset>

RC CloseScan(RM_FileScan *rmFileScan)
{
	rmFileScan->bOpen=false;
	CloseFile(rmFileScan->pRMFileHandle->file);
	return SUCCESS;
}

RC OpenScan(RM_FileScan *rmFileScan,RM_FileHandle *fileHandle,int conNum,Con *conditions)//��ʼ��ɨ��
{
	//��ɨ���Ѿ���
	if(rmFileScan->bOpen)
		return RM_FSOPEN;
	//��ʼ��ɨ��ṹ
	rmFileScan->bOpen=TRUE;
	rmFileScan->conNum=conNum;
	rmFileScan->conditions=conditions;
	rmFileScan->pRMFileHandle=fileHandle;
	//ȡ�ô�ɨ�����ҳ��
	if(GetThisPage(fileHandle->file,2,&rmFileScan->PageHandle))
		return FAIL;
	rmFileScan->pn=3;
	rmFileScan->sn=0; //��۴�0��ʼ���
	return SUCCESS;
}

//���Գ�����ʲô�ã�
RC GetNextRec(RM_FileScan *rmFileScan,RM_Record *rec)
{
	if(!rmFileScan->bOpen)
		return RM_FSCLOSED;
	/*
	//�ҵ���һ����Ч�ļ�¼
	int nextPage=rmFileScan->pRMFileHandle->pageCtlBitmap->firstBit(rmFileScan->pn,1);
	if(nextPage<0)
		return RM_NOMORERECINMEM;
	if(rmFileScan->pn!=nextPage)
	{
		rmFileScan->pn=nextPage;
		GetThisPage(rmFileScan->pRMFileHandle->file,nextPage,&rmFileScan->PageHandle);
		rmFileScan->sn=0;
	}
	*/
	bitmanager recBitmap(1,NULL);
	while(1)
	{
		GetThisPage(rmFileScan->pRMFileHandle->file,rmFileScan->pn,&rmFileScan->PageHandle);
		recBitmap.redirectBitmap(rmFileScan->pRMFileHandle->recPerPage,rmFileScan->PageHandle.pFrame->page.pData);
		rmFileScan->sn=recBitmap.firstBit(rmFileScan->sn,1);
		while(((rmFileScan->sn=recBitmap.firstBit(rmFileScan->sn,1))!=-1))
		{
			Con *condition;
			bool correct=true;
			//���������бȽ�
			int recOffset=rmFileScan->pRMFileHandle->recOffset+rmFileScan->sn*rmFileScan->pRMFileHandle->recSize;
			char *recAddr=rmFileScan->PageHandle.pFrame->page.pData+recOffset;
			int conNumber;
			int leftVal,rightVal;
			float leftF,rightF;
			char *leftStr,*rightStr;
			for(conNumber=0; conNumber<rmFileScan->conNum; conNumber++)
			{
				condition=(Con *)(rmFileScan->conditions+conNumber);
				switch (condition->attrType)
				{
				case ints:
					leftVal=(condition->bLhsIsAttr==1)?*((int *)(recAddr+condition->LattrOffset)):*((int *)condition->Lvalue);
					if(condition->compOp==NO_OP)
					{
						if(leftVal==0)
						{
							correct=false;
							break;
						}
						else
							break;
					}
					rightVal=(condition->bRhsIsAttr==1)?*((int *)(recAddr+condition->RattrOffset)):*((int *)condition->Rvalue);
					correct=CmpValue(leftVal,rightVal,condition->compOp);
					break;
				case floats:
					leftF=(condition->bLhsIsAttr==1)?*((float *)(recAddr+condition->LattrOffset)):*((float *)condition->Lvalue);
					if(condition->compOp==NO_OP)
					{
						if(leftVal==0)
						{
							correct=false;
							break;
						}
						else
							break;
					}
					rightF=(condition->bRhsIsAttr==1)?*((float *)(recAddr+condition->RattrOffset)):*((float *)condition->Rvalue);
					correct=CmpValue(leftF,rightF,condition->compOp);
					break;
				case chars:
					if(condition->compOp==NO_OP)
					{
						correct=false;
						break;
					}
					leftStr=(condition->bLhsIsAttr==1)?(recAddr+condition->LattrOffset):(char *)condition->Lvalue;
					rightStr=(condition->bRhsIsAttr==1)?(recAddr+recOffset+condition->RattrOffset):(char *)condition->Rvalue;
					//�Ƚ������ַ���
					correct=CmpString(leftStr,rightStr,condition->compOp);
					break;
				}
				if(!correct)
					break;
			}
			//if ����Ƚ�����
			if(conNumber==rmFileScan->conNum)
			{
				rec->bValid=true;
				rec->pData=recAddr;
				rec->rid.bValid=true;
				rec->rid.pageNum=rmFileScan->pn;
				rec->rid.slotNum=rmFileScan->sn;
				rmFileScan->sn++;  //����Ҫ
				return SUCCESS;
			}
			//if ����������
			else
				rmFileScan->sn++;
		}
		//������һҳ
		rmFileScan->pn=rmFileScan->pRMFileHandle->pageCtlBitmap->firstBit(rmFileScan->pn+1,1);
		if(rmFileScan->pn==-1)
			return RM_NOMORERECINMEM;
		rmFileScan->sn=0;
	}
}

bool CmpString(char *left, char *right, CompOp oper)
{
	int cmpResult=strcmp(left,right);
	switch(oper)
	{
	case EQual:
		return (cmpResult==0)?true:false;
	case LessT:
		return (cmpResult<0)?true:false;
	case GreatT:
		return (cmpResult>0)?true:false;
	case NEqual:
		return (cmpResult==0)?false:true;
	case LEqual:
		return (cmpResult==0||cmpResult<0)?true:false;
	case GEqual:
		return (cmpResult==0||cmpResult>0)?true:false;
	default:
		return false;
	}
}

bool CmpValue(float left, float right, CompOp oper)
{
	switch(oper)
	{
	case EQual:
		return (left==right);
	case LessT:
		return (left<right);
	case GreatT:
		return (left>right);
	case NEqual:
		return (left!=right);
	case LEqual:
		return (left<=right);
	case GEqual:
		return (left>=right);
	default:
		return false;
	}
}

RC GetRec (RM_FileHandle *fileHandle,RID *rid, RM_Record *rec) 
{
	//�����ж�rid�Ƿ���Ч��ҳ�Ƿ񱻷��䣬��¼�Ƿ���Ч��
	rec->bValid=false;
	//char bitmapPage=*(fileHandle->file->pBitmap+rid->pageNum/8);
	if(!fileHandle->pageCtlBitmap->atPos(rid->pageNum)||(rid->pageNum>fileHandle->file->pFileSubHeader->pageCount)||(rid->slotNum>fileHandle->recPerPage-1))
		return RM_INVALIDRID;
	PF_PageHandle *targetPage=NULL;
	if(GetThisPage(fileHandle->file,rid->pageNum,targetPage))
		return FAIL;
	char bitmapRec=*(targetPage->pFrame->page.pData+rid->slotNum/8);
	if((bitmapRec&(0x01<<(rid->slotNum%8)))==0)
		return RM_INVALIDRID;
	//RID��Ч
	rid->bValid=TRUE;
	rec->bValid=TRUE;
	rec->pData=targetPage->pFrame->page.pData+fileHandle->recOffset+rid->slotNum*fileHandle->recSize;
	return SUCCESS;
}

RC InsertRec (RM_FileHandle *fileHandle,char *pData, RID *rid)
{
	/*************
	�����¼ʱ���������ļ����ҵ�һ������ҳ����û������Ҫ����һ���µ�ҳ�棩��
	�ڸ�ҳ���ҵ�һ���ղ�۲������¼����¼�����Ҫ���¿���ҳ��
	�����ļ������ļ�¼����1��ͬʱ�����λͼ�е���Ӧλ��1��
	�����¼����󣬸�ҳ���Ѿ�û�пղ�ۣ��򽫸�ҳ����Ϊ��ҳ��
	���ڼ�¼�ļ��Ŀ���ҳ��λͼ�Ͻ���ҳ������Ӧ��λ��1��
	*************/
	//��û��δ��ҳ����ȫ��ҳ���Ѿ�����
	if(!fileHandle->recCtlBitmap->anyZero())
		return FAIL;
	//���Ȳ����Ƿ�����ѷ����δ��ҳ
	int unfillPage=fileHandle->recCtlBitmap->firstBit(2,0);
	while((!fileHandle->pageCtlBitmap->atPos(unfillPage))&&unfillPage<fileHandle->bitmapLength*8)
		unfillPage=fileHandle->recCtlBitmap->firstBit(unfillPage+1,0);
	if(unfillPage>=fileHandle->bitmapLength*8)    //��û���Ѿ������δ��ҳ��������ҳ
	{
		if(!fileHandle->pageCtlBitmap->anyZero())   //�Ѿ�û�п���ҳ��
		{
			rid->bValid=false;
			return FAIL;
		}
		//���п���ҳ�棨��δ����ҳ�棩
		PF_PageHandle *newPage=NULL;
		if(AllocatePage(fileHandle->file,newPage))
			return FAIL;
		bitmanager bmpNewPage(fileHandle->bitmapLength,newPage->pFrame->page.pData);
		memcpy(newPage->pFrame->page.pData+fileHandle->recOffset,pData,fileHandle->recSize);
		rid->bValid=true;
		rid->pageNum=newPage->pFrame->page.pageNum;
		rid->slotNum=0;
		bmpNewPage.setBitmap(2,1);
	}
	else    //�����Ѿ������δ��ҳ��ҳ��ΪunfillPage
	{
		PF_PageHandle *page=NULL;
		if(GetThisPage(fileHandle->file,unfillPage,page))
			return FAIL;
		//ͨ��λͼ�ҵ��ղ�
		bitmanager bmp(fileHandle->bitmapLength,page->pFrame->page.pData);
		int emptySlot=bmp.firstBit(0,0);
		//�����¼
		memcpy(page->pFrame->page.pData+fileHandle->recOffset+emptySlot*fileHandle->recSize,pData,fileHandle->recSize);
		//���²�λͼ
		bmp.setBitmap(emptySlot,1);
		//��ҳ�����������¼�¼����λͼ
		if(!bmp.anyZero())
			fileHandle->recCtlBitmap->setBitmap(unfillPage,1);
		rid->bValid=true;
		rid->pageNum=unfillPage;
		rid->slotNum=emptySlot;
	}
	//���¼�¼��
	*(fileHandle->recNum)++;
	return SUCCESS;
}

RC DeleteRec (RM_FileHandle *fileHandle,const RID *rid)
{
	//���RID��Ч��
	if(rid->pageNum>fileHandle->file->pFileSubHeader->pageCount||(!fileHandle->pageCtlBitmap->atPos(rid->pageNum)))
		return RM_INVALIDRID;
	//���Ŀ��ҳ�ľ��
	PF_PageHandle *target=NULL;
	if(GetThisPage(fileHandle->file,rid->pageNum,target))
		return FAIL;
	//����¼�Ƿ���Ч
	bitmanager bitmap(fileHandle->recPerPage,target->pFrame->page.pData);
	if(!bitmap.atPos(rid->slotNum))
		return RM_INVALIDRID;
	bitmap.setBitmap(rid->slotNum,0);
	//�޸�ʣ���¼��
	(*fileHandle->recNum)--;
	return SUCCESS;
}

RC UpdateRec (RM_FileHandle *fileHandle,const RM_Record *rec)
{
	//���RID��Ч��
	if(rec->rid.pageNum>fileHandle->file->pFileSubHeader->pageCount||(!fileHandle->pageCtlBitmap->atPos(rec->rid.pageNum)))
		return RM_INVALIDRID;
	//���Ŀ��ҳ�ľ��
	PF_PageHandle *target=NULL;
	if(GetThisPage(fileHandle->file,rec->rid.pageNum,target))
		return FAIL;
	//����¼�Ƿ���Ч
	bitmanager bitmap(fileHandle->recPerPage,target->pFrame->page.pData);
	if(!bitmap.atPos(rec->rid.slotNum))
		return RM_INVALIDRID;
	//RID��Ч�������¼
	memcpy(target->pFrame->page.pData+fileHandle->recOffset+rec->rid.slotNum *fileHandle->recSize,rec->pData,fileHandle->recSize);
	return SUCCESS;
}

RC RM_CreateFile (char *fileName, int recordSize)
{
	int rtn=CreateFile(fileName);
	if(rtn)
		return FAIL;  //����ֵ��Ҫ����һ��
	//����ɹ������ɼ�¼��Ϣ����ҳ�����Ȼ�ô����ļ��ľ��
	PF_FileHandle *file=NULL;
	rtn=openFile(fileName,file);
	if(rtn)
		return FAIL;  //����ֵ��Ҫ����һ��
	//������ҳ��
	PF_PageHandle *ctrPage=NULL;
	rtn=AllocatePage(file,ctrPage);
	if(rtn)
		return FAIL;
	//�ڼ�¼����ҳ��ż�¼��Ϣ���ݽṹ
	RM_recControl *recCtl;
	recCtl=(RM_recControl *)ctrPage->pFrame->page.pData;
	//��ʼ��
	recCtl->recNum=0;
	recCtl->recSize=recordSize;
	recCtl->recPerPage=PF_PAGE_SIZE/(recordSize+0.125); //������Դ�ŵļ�¼������ÿ���¼ռ��recordSize���0.125��λͼ��С�ֽ�,��1��Ϊ��ȷ��λͼ��С�ܹ�Ϊ8�ı���
	//Ϊ�˷���λͼ����ƣ��˴�����¼������ȡ8�ı���
	recCtl->recPerPage=(int(recCtl->recPerPage/8))*8;
	recCtl->recordOffset=recCtl->recPerPage/8; //λͼ�Ĵ�С����Ϊ8��������
	//recCtl->fileNum=1; //�մ���ʱ��ֻʹ����һ����ҳ�ļ�
	//�رոոմ򿪵��ļ�
	CloseFile(file);
	return SUCCESS;
}

RC RM_OpenFile(char *fileName, RM_FileHandle *fileHandle)
{
	if(fileHandle->bOpen)  //��ʹ�õľ���Ѿ���Ӧһ���򿪵��ļ�
		return RM_FHOPENNED;
	if(openFile(fileName,fileHandle->file))
		return FAIL;
	fileHandle->bOpen=TRUE;
	//fileHandle->fileName=fileName;
	//fileHandle->file[0]=filePF;
	//��ȡ��¼���������Ϣ
	PF_PageHandle *ctrPage=NULL;
	if(GetThisPage(filePF,1,ctrPage))
	{
		CloseFile(filePF);
		return FAIL;
	}
	RM_recControl *recCtl;
	recCtl=(RM_recControl *)ctrPage->pFrame->page.pData;
	fileHandle->recNum=&recCtl->recNum;
	fileHandle->recOffset=recCtl->recordOffset;
	fileHandle->recPerPage=recCtl->recPerPage;
	fileHandle->recSize=recCtl->recSize;
	fileHandle->bitmapLength=PF_PAGE_SIZE-sizeof(RM_recControl); //���Է�������ҳ��/8
	//��ȡ��¼����λͼ
	fileHandle->recCtlBitmap=new bitmanager(fileHandle->bitmapLength,ctrPage->pFrame->page.pData+sizeof(RM_recControl));
	//��ȡҳ�����λͼ
	fileHandle->pageCtlBitmap=new bitmanager(fileHandle->bitmapLength,filePF->pBitmap);
	/*
	//ȷ���ܹ����ٸ���ҳ�ļ�����Ҫ��ȡ
	PF_PageHandle *ctrPage;
	if(GetThisPage(filePF,1,ctrPage))
		return FAIL;
	RM_recControl *control;
	control=(RM_recControl *)ctrPage->pFrame->page.pData;
	fileHandle->fileNum=control->fileNum;
	//��ʹ���˶����ҳ�ļ��洢�������
	if(fileHandle->fileNum>1)
	{
		char name[sizeof(fileName)+2];  //����������ļ���
		strcpy(name,fileName);
		name[sizeof(fileName)+1]='\0';  //ȷ���ַ�����β
		for(int i=1;i<fileHandle->fileNum;i++)
		{
			name[sizeof(fileName)]=(char)(i+48);
			if(openFile(name,fileHandle->file[i]))  //�޸�
			{
				for(int close=0;close<i;close++)       //����һ���ļ���ʧ����ʧ�ܣ����Ѿ��򿪵Ĺر�
					CloseFile(fileHandle->file[close]);
				return FAIL;
			}
		}
	}
	*/
	return SUCCESS;
}

RC RM_CloseFile(RM_FileHandle *fileHandle)
{
	//���Ѿ��ر�
	if(!fileHandle->bOpen)
		return RM_FHCLOSED;
	if(CloseFile(fileHandle->file))
			return FAIL;
	/*
	//�������йصķ�ҳ�ļ�ȫ���ر�
	for(int close=0;close<fileHandle->fileNum;close++)
	{
		if(CloseFile(fileHandle->file[close]))
			return FAIL;
	}
	*/
	fileHandle->bOpen=FALSE;
	return SUCCESS;
}

/*

RC��RM���֣�
RM_FH CLOSED,
RM_FH OPENNED,
RM_INVALID RECSIZE,
RM_INVALID RID,
RM_FS CLOSED,
RM_NO MORE REC IN MEM,
RM_FS OPEN,
RM_NO MORE IDX IN MEM,
*/