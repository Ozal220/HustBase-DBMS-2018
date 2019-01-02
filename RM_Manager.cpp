#include "stdafx.h"
#include "RM_Manager.h"
#include "str.h"
#include <cstring>

#include <iostream>

RC CloseScan(RM_FileScan *rmFileScan)
{
	rmFileScan->bOpen=false;
	CloseFile(&rmFileScan->pRMFileHandle->file);
	UnpinPage(&rmFileScan->PageHandle);
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
	if(GetThisPage(&fileHandle->file,2,&rmFileScan->PageHandle))
		return FAIL;
	rmFileScan->pn=2;
	rmFileScan->sn=0; //��۴�0��ʼ���
	return SUCCESS;
}

//���Գ�����ʲô�ã�
RC GetNextRec(RM_FileScan *rmFileScan,RM_Record *rec)
{
	if(!rmFileScan->bOpen)
		return RM_FSCLOSED;
	bitmanager recBitmap(1,NULL);
	while(1)
	{
		GetThisPage(&rmFileScan->pRMFileHandle->file,rmFileScan->pn,&rmFileScan->PageHandle);
		//recPerPage��Ϊ8�ı���
		recBitmap.redirectBitmap(rmFileScan->pRMFileHandle->recPerPage/8,rmFileScan->PageHandle.pFrame->page.pData);
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
		UnpinPage(&rmFileScan->PageHandle);
		if(rmFileScan->pRMFileHandle->pageCtlBitmap->firstBit(rmFileScan->pn+1,1)==-1)
			return RM_NOMORERECINMEM;
		rmFileScan->pn=rmFileScan->pRMFileHandle->pageCtlBitmap->firstBit(rmFileScan->pn+1,1);
		GetThisPage(&rmFileScan->pRMFileHandle->file,rmFileScan->pn,&rmFileScan->PageHandle);
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
	if(!fileHandle->pageCtlBitmap->atPos(rid->pageNum)||(rid->pageNum>fileHandle->file.pFileSubHeader->pageCount)||(rid->slotNum>fileHandle->recPerPage-1)||(rid->pageNum<2))
		return RM_INVALIDRID;
	PF_PageHandle *targetPage=new PF_PageHandle;
	if(GetThisPage(&fileHandle->file,rid->pageNum,targetPage))
	{
		free(targetPage);
		return FAIL;
	}
	char bitmapRec=*(targetPage->pFrame->page.pData+rid->slotNum/8);
	if((bitmapRec&(0x01<<(rid->slotNum%8)))==0)
	{
		UnpinPage(targetPage);
		free(targetPage);
		return RM_INVALIDRID;
	}
	//RID��Ч
	rec->rid.bValid=TRUE;
	rec->rid.pageNum=rid->pageNum;
	rec->rid.slotNum=rid->slotNum;
	rec->bValid=TRUE;
	rec->pData=targetPage->pFrame->page.pData+fileHandle->recOffset+rid->slotNum*fileHandle->recSize;
	UnpinPage(targetPage);
	free(targetPage);
	return SUCCESS;
}

RC InsertRec (RM_FileHandle *fileHandle,char *pData, RID *rid)
{
	int unfillPage=fileHandle->recCtlBitmap->firstBit(2,0);
	//��û��δ��ҳ����ȫ��ҳ���Ѿ�����
	if(unfillPage==-1)
		return FAIL;
	//���Ȳ����Ƿ�����ѷ����δ��ҳ
	while((!fileHandle->pageCtlBitmap->atPos(unfillPage))&&unfillPage<=fileHandle->file.pFileSubHeader->pageCount)
		unfillPage=fileHandle->recCtlBitmap->firstBit(unfillPage+1,0);
	if(unfillPage>fileHandle->file.pFileSubHeader->pageCount)    //��û���Ѿ������δ��ҳ��������ҳ
	{
		if(!fileHandle->pageCtlBitmap->anyZero())   //�Ѿ�û�п���ҳ��
		{
			rid->bValid=false;
			return FAIL;
		}
		//���п���ҳ�棨��δ����ҳ�棩
		PF_PageHandle *newPage=new PF_PageHandle;
		if(AllocatePage(&fileHandle->file,newPage))
			return FAIL;
		bitmanager bmpNewPage(fileHandle->recOffset,newPage->pFrame->page.pData);
		memcpy(newPage->pFrame->page.pData+fileHandle->recOffset,pData,fileHandle->recSize);
		rid->bValid=true;
		rid->pageNum=newPage->pFrame->page.pageNum;
		rid->slotNum=0;
		bmpNewPage.setBitmap(0,1);
		MarkDirty(newPage);
		UnpinPage(newPage);
		free(newPage);
	}
	else    //�����Ѿ������δ��ҳ��ҳ��ΪunfillPage
	{
		PF_PageHandle *page=new PF_PageHandle;
		if(GetThisPage(&fileHandle->file,unfillPage,page))
		{
			free(page);
			return FAIL;
		}
		//ͨ��λͼ�ҵ��ղ�
		bitmanager bmp(fileHandle->recOffset,page->pFrame->page.pData);
		int emptySlot=bmp.firstBit(0,0);
		//�����¼
		memcpy(page->pFrame->page.pData+fileHandle->recOffset+emptySlot*fileHandle->recSize,pData,fileHandle->recSize);
		//���²�λͼ
		bmp.setBitmap(emptySlot,1);
		//��ҳ�����������¼�¼����λͼ
		if(!bmp.anyZero())
		{
			PF_PageHandle *ctrPage=new PF_PageHandle;
			GetThisPage(&fileHandle->file,1,ctrPage);
			fileHandle->recCtlBitmap->setBitmap(unfillPage,1);
			MarkDirty(ctrPage);
			UnpinPage(ctrPage);
			free(ctrPage);
		}
		rid->bValid=true;
		rid->pageNum=unfillPage;
		rid->slotNum=emptySlot;
		MarkDirty(page);
		UnpinPage(page);
		free(page);
	}
	//���¼�¼��
	*(fileHandle->recNum)++;
	return SUCCESS;
}

RC DeleteRec (RM_FileHandle *fileHandle,const RID *rid)
{
	//���RID��Ч��
	if(rid->pageNum>fileHandle->file.pFileSubHeader->pageCount||(!fileHandle->pageCtlBitmap->atPos(rid->pageNum))||(rid->pageNum<2))
		return RM_INVALIDRID;
	//���Ŀ��ҳ�ľ��
	PF_PageHandle *target=new PF_PageHandle;
	if(GetThisPage(&fileHandle->file,rid->pageNum,target))
	{
		free(target);
		return FAIL;
	}
	//����¼�Ƿ���Ч
	bitmanager bitmap(fileHandle->recOffset,target->pFrame->page.pData);
	if(!bitmap.atPos(rid->slotNum))
	{
		UnpinPage(target);
		free(target);
		return RM_INVALIDRID;
	}
	bitmap.setBitmap(rid->slotNum,0);
	//���Ѿ�û���κ���Ч��¼
	if(bitmap.firstBit(0,1)==-1)
	{
		PF_PageHandle *ctrPage=new PF_PageHandle;
		GetThisPage(&fileHandle->file,1,ctrPage);
		fileHandle->recCtlBitmap->setBitmap(rid->pageNum,0);
		MarkDirty(ctrPage);
		UnpinPage(ctrPage);
		free(ctrPage);
		DisposePage(&fileHandle->file,target->pFrame->page.pageNum);
	}
	//�޸�ʣ���¼��
	(*fileHandle->recNum)--;
	MarkDirty(target);
	UnpinPage(target);
	free(target);
	return SUCCESS;
}

RC UpdateRec (RM_FileHandle *fileHandle,const RM_Record *rec)
{
	//���RID��Ч��
	if(rec->rid.pageNum>fileHandle->file.pFileSubHeader->pageCount||(!fileHandle->pageCtlBitmap->atPos(rec->rid.pageNum))||(rec->rid.pageNum<2))
		return RM_INVALIDRID;
	//���Ŀ��ҳ�ľ��
	PF_PageHandle *target=new PF_PageHandle;
	if(GetThisPage(&fileHandle->file,rec->rid.pageNum,target))
		return FAIL;
	//����¼�Ƿ���Ч
	bitmanager bitmap(fileHandle->recOffset,target->pFrame->page.pData);
	if(!bitmap.atPos(rec->rid.slotNum))
	{
		UnpinPage(target);
		free(target);
		return RM_INVALIDRID;
	}
	//RID��Ч�������¼
	memcpy(target->pFrame->page.pData+fileHandle->recOffset+rec->rid.slotNum *fileHandle->recSize,rec->pData,fileHandle->recSize);
	MarkDirty(target);
	UnpinPage(target);
	free(target);
	return SUCCESS;
}

RC RM_CreateFile (char *fileName, int recordSize)
{
	if(CreateFile(fileName))
		return FAIL;  //����ֵ��Ҫ����һ��
	//����ɹ������ɼ�¼��Ϣ����ҳ�����Ȼ�ô����ļ��ľ��
	PF_FileHandle *file=new PF_FileHandle;
	if(openFile(fileName,file))
		return FAIL;  //����ֵ��Ҫ����һ��
	//������ҳ��
	PF_PageHandle *ctrPage=new PF_PageHandle;
	if(AllocatePage(file,ctrPage))
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
	MarkDirty(ctrPage);
	UnpinPage(ctrPage);
	CloseFile(file);
	return SUCCESS;
}

RC RM_OpenFile(char *fileName, RM_FileHandle *fileHandle)
{
	if(fileHandle->bOpen)  //��ʹ�õľ���Ѿ���Ӧһ���򿪵��ļ�
		return RM_FHOPENNED;
	if(openFile(fileName,&fileHandle->file))
		return FAIL;
	fileHandle->bOpen=TRUE;
	//fileHandle->fileName=fileName;
	//fileHandle->file[0]=filePF;
	//��ȡ��¼���������Ϣ
	PF_PageHandle *ctrPage=new PF_PageHandle;
	if(GetThisPage(&fileHandle->file,1,ctrPage))
	{
		CloseFile(&fileHandle->file);
		return FAIL;
	}
	RM_recControl *recCtl=NULL;
	recCtl=(RM_recControl *)ctrPage->pFrame->page.pData;
	fileHandle->recNum=&recCtl->recNum;
	fileHandle->recOffset=recCtl->recordOffset;
	fileHandle->recPerPage=recCtl->recPerPage;
	fileHandle->recSize=recCtl->recSize;
	fileHandle->bitmapLength=PF_PAGE_SIZE-sizeof(RM_recControl); //���Է�������ҳ��/8
	//��ȡ��¼����λͼ
	fileHandle->recCtlBitmap=new bitmanager(fileHandle->bitmapLength,ctrPage->pFrame->page.pData+sizeof(RM_recControl));
	//��ȡҳ�����λͼ
	fileHandle->pageCtlBitmap=new bitmanager(fileHandle->bitmapLength,fileHandle->file.pBitmap);
	UnpinPage(ctrPage);
	return SUCCESS;
}


RC RM_CloseFile(RM_FileHandle *fileHandle)
{
	//���Ѿ��ر�
	if(!fileHandle->bOpen)
		return RM_FHCLOSED;
	if(CloseFile(&fileHandle->file))
			return FAIL;
	fileHandle->bOpen=FALSE;
	return SUCCESS;
}
