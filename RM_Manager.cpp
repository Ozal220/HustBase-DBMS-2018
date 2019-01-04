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

RC OpenScan(RM_FileScan *rmFileScan,RM_FileHandle *fileHandle,int conNum,Con *conditions)//初始化扫描
{
	//若扫描已经打开
	if(rmFileScan->bOpen)
		return RM_FSOPEN;
	//初始化扫描结构
	rmFileScan->bOpen=TRUE;
	rmFileScan->conNum=conNum;
	rmFileScan->conditions=conditions;
	rmFileScan->pRMFileHandle=fileHandle;
	//取得待扫描的首页面
	if(GetThisPage(&fileHandle->file,2,&rmFileScan->PageHandle))
		return FAIL;
	rmFileScan->pn=2;
	rmFileScan->sn=0; //插槽从0开始编号
	return SUCCESS;
}

//属性长度有什么用？
RC GetNextRec(RM_FileScan *rmFileScan,RM_Record *rec)
{
	if(!rmFileScan->bOpen)
		return RM_FSCLOSED;
	bitmanager recBitmap(1,NULL);
	while(1)
	{
		GetThisPage(&rmFileScan->pRMFileHandle->file,rmFileScan->pn,&rmFileScan->PageHandle);
		//recPerPage必为8的倍数
		recBitmap.redirectBitmap(rmFileScan->pRMFileHandle->recPerPage/8,rmFileScan->PageHandle.pFrame->page.pData);
		rmFileScan->sn=recBitmap.firstBit(rmFileScan->sn,1);
		while(((rmFileScan->sn=recBitmap.firstBit(rmFileScan->sn,1))!=-1))
		{
			Con *condition;
			bool correct=true;
			//与条件进行比较
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
					//比较两个字符串
					correct=CmpString(leftStr,rightStr,condition->compOp);
					break;
				}
				if(!correct)
					break;
			}
			//if 满足比较条件
			if(conNumber==rmFileScan->conNum)
			{
				rec->bValid=true;
				rec->pData=recAddr;
				rec->rid.bValid=true;
				rec->rid.pageNum=rmFileScan->pn;
				rec->rid.slotNum=rmFileScan->sn;
				rmFileScan->sn++;  //很重要
				return SUCCESS;
			}
			//if 不满足条件
			else
				rmFileScan->sn++;
		}
		//跳到下一页
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
	//首先判断rid是否有效（页是否被分配，记录是否有效）
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
	//RID有效
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
	//若没有未满页面且全部页面已经分配
	if(unfillPage==-1)
		return FAIL;
	//首先查找是否存在已分配的未满页
	while((!fileHandle->pageCtlBitmap->atPos(unfillPage))&&unfillPage<=fileHandle->file.pFileSubHeader->pageCount)
		unfillPage=fileHandle->recCtlBitmap->firstBit(unfillPage+1,0);
	if(unfillPage>fileHandle->file.pFileSubHeader->pageCount)    //若没有已经分配的未满页，分配新页
	{
		if(!fileHandle->pageCtlBitmap->anyZero())   //已经没有空闲页面
		{
			rid->bValid=false;
			return FAIL;
		}
		//还有空闲页面（或未分配页面）
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
	else    //存在已经分配的未满页，页号为unfillPage
	{
		PF_PageHandle *page=new PF_PageHandle;
		if(GetThisPage(&fileHandle->file,unfillPage,page))
		{
			free(page);
			return FAIL;
		}
		//通过位图找到空槽
		bitmanager bmp(fileHandle->recOffset,page->pFrame->page.pData);
		int emptySlot=bmp.firstBit(0,0);
		//存入记录
		memcpy(page->pFrame->page.pData+fileHandle->recOffset+emptySlot*fileHandle->recSize,pData,fileHandle->recSize);
		//更新槽位图
		bmp.setBitmap(emptySlot,1);
		//若页面已满，更新记录控制位图
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
	//更新记录数
	*(fileHandle->recNum)++;
	return SUCCESS;
}

RC DeleteRec (RM_FileHandle *fileHandle,const RID *rid)
{
	//检测RID有效性
	if(rid->pageNum>fileHandle->file.pFileSubHeader->pageCount||(!fileHandle->pageCtlBitmap->atPos(rid->pageNum))||(rid->pageNum<2))
		return RM_INVALIDRID;
	//获得目标页的句柄
	PF_PageHandle *target=new PF_PageHandle;
	if(GetThisPage(&fileHandle->file,rid->pageNum,target))
	{
		free(target);
		return FAIL;
	}
	//检查记录是否有效
	bitmanager bitmap(fileHandle->recOffset,target->pFrame->page.pData);
	if(!bitmap.atPos(rid->slotNum))
	{
		UnpinPage(target);
		free(target);
		return RM_INVALIDRID;
	}
	bitmap.setBitmap(rid->slotNum,0);
	//若已经没有任何有效记录
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
	//修改剩余记录数
	(*fileHandle->recNum)--;
	MarkDirty(target);
	UnpinPage(target);
	free(target);
	return SUCCESS;
}

RC UpdateRec (RM_FileHandle *fileHandle,const RM_Record *rec)
{
	//检测RID有效性
	if(rec->rid.pageNum>fileHandle->file.pFileSubHeader->pageCount||(!fileHandle->pageCtlBitmap->atPos(rec->rid.pageNum))||(rec->rid.pageNum<2))
		return RM_INVALIDRID;
	//获得目标页的句柄
	PF_PageHandle *target=new PF_PageHandle;
	if(GetThisPage(&fileHandle->file,rec->rid.pageNum,target))
		return FAIL;
	//检查记录是否有效
	bitmanager bitmap(fileHandle->recOffset,target->pFrame->page.pData);
	if(!bitmap.atPos(rec->rid.slotNum))
	{
		UnpinPage(target);
		free(target);
		return RM_INVALIDRID;
	}
	//RID有效，存入记录
	memcpy(target->pFrame->page.pData+fileHandle->recOffset+rec->rid.slotNum *fileHandle->recSize,rec->pData,fileHandle->recSize);
	MarkDirty(target);
	UnpinPage(target);
	free(target);
	return SUCCESS;
}

RC RM_CreateFile (char *fileName, int recordSize)
{
	if(CreateFile(fileName))
		return FAIL;  //返回值需要考虑一下
	//如果成功，生成记录信息控制页，首先获得创建文件的句柄
	PF_FileHandle *file=new PF_FileHandle;
	if(openFile(fileName,file))
		return FAIL;  //返回值需要考虑一下
	//申请新页面
	PF_PageHandle *ctrPage=new PF_PageHandle;
	if(AllocatePage(file,ctrPage))
		return FAIL;
	//在记录控制页存放记录信息数据结构
	RM_recControl *recCtl;
	recCtl=(RM_recControl *)ctrPage->pFrame->page.pData;
	//初始化
	recCtl->recNum=0;
	recCtl->recSize=recordSize;
	recCtl->recPerPage=PF_PAGE_SIZE/(recordSize+0.125); //计算可以存放的记录数量，每项记录占用recordSize另加0.125个位图大小字节,减1是为了确保位图大小能够为8的倍数
	//为了方便位图的设计，此处将记录数向下取8的倍数
	recCtl->recPerPage=(int(recCtl->recPerPage/8))*8;
	recCtl->recordOffset=recCtl->recPerPage/8; //位图的大小，必为8的整数倍
	//recCtl->fileNum=1; //刚创建时，只使用了一个分页文件
	//关闭刚刚打开的文件
	MarkDirty(ctrPage);
	UnpinPage(ctrPage);
	CloseFile(file);
	return SUCCESS;
}

RC RM_OpenFile(char *fileName, RM_FileHandle *fileHandle)
{
	if(fileHandle->bOpen)  //若使用的句柄已经对应一个打开的文件
		return RM_FHOPENNED;
	if(openFile(fileName,&fileHandle->file))
		return FAIL;
	fileHandle->bOpen=TRUE;
	//fileHandle->fileName=fileName;
	//fileHandle->file[0]=filePF;
	//获取记录管理基本信息
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
	fileHandle->bitmapLength=PF_PAGE_SIZE-sizeof(RM_recControl); //可以分配的最大页数/8
	//获取记录管理位图
	fileHandle->recCtlBitmap=new bitmanager(fileHandle->bitmapLength,ctrPage->pFrame->page.pData+sizeof(RM_recControl));
	//获取页面管理位图
	fileHandle->pageCtlBitmap=new bitmanager(fileHandle->bitmapLength,fileHandle->file.pBitmap);
	UnpinPage(ctrPage);
	return SUCCESS;
}


RC RM_CloseFile(RM_FileHandle *fileHandle)
{
	//若已经关闭
	if(!fileHandle->bOpen)
		return RM_FHCLOSED;
	if(CloseFile(&fileHandle->file))
			return FAIL;
	fileHandle->bOpen=FALSE;
	return SUCCESS;
}
