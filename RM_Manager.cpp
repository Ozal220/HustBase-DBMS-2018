#include "stdafx.h"
#include "RM_Manager.h"
#include "str.h"
#include <cstring>
#include <bitset>

RC OpenScan(RM_FileScan *rmFileScan,RM_FileHandle *fileHandle,int conNum,Con *conditions)//初始化扫描
{
	//若扫描已经打开
	if(rmFileScan->bOpen)
		return RM_FSOPEN;
	//初始化扫描结构
	rmFileScan->bOpen=TRUE;
	rmFileScan->conNum=conNum;
	rmFileScan->conditions=conditions;
	//取得带扫描的首页面
	if(GetThisPage(fileHandle->file,2,&rmFileScan->PageHandle))
		return FAIL;
	rmFileScan->pn=3;
	rmFileScan->sn=0; //插槽从0开始编号
	return SUCCESS;
}

//测试正常后考虑使用引用优化一下程序表达式，太长了
RC GetNextRec(RM_FileScan *rmFileScan,RM_Record *rec)
{
	//确认是否检查完毕
	//if(rmFileScan->pn==(rmFileScan->pRMFileHandle->file->pFileSubHeader->pageCount-1))
	//先找到一个已经分配的页
	char bitmap,bitmapRec;
	while(rmFileScan->pn<=(rmFileScan->pRMFileHandle->file->pFileSubHeader->pageCount-1))
	{
		//先判断是否为空页
		bitmap=*(rmFileScan->pRMFileHandle->file->pBitmap+(rmFileScan->pn/8));
		if((bitmap&(0x01<<(rmFileScan->pn%8)))==0) //是空页
		{
			rmFileScan->pn++;
			rmFileScan->sn=0; //从新页的0号插槽开始处理
		}
		else  //若非空页，对逐条记录进行比较
		{
			//首先获得该页句柄
			if(GetThisPage(rmFileScan->pRMFileHandle->file,rmFileScan->pn,&rmFileScan->PageHandle))
				return FAIL;	
			for(;rmFileScan->sn<rmFileScan->pRMFileHandle->recNum;rmFileScan->sn++)
			{
				//先检查位图，记录为空直接跳过
				bitmapRec=*(rmFileScan->PageHandle.pFrame->page.pData+(rmFileScan->sn/8));
				if(bitmapRec&(0x01<<rmFileScan->sn%8)==0)
					continue;
				//开始比较记录与查询条件
				char *recOffset=rmFileScan->PageHandle.pFrame->page.pData+(rmFileScan->pRMFileHandle->recOffset+rmFileScan->sn*rmFileScan->pRMFileHandle->recSize);  //当前使用记录的指针
				for(int con=0;con<rmFileScan->conNum;con++)
				{
					//比较共有00,01,10,11四种
					int type=0;
					type|=rmFileScan->conditions[con].bLhsIsAttr;
					type<<=1;
					type|=rmFileScan->conditions[con].bRhsIsAttr;
					switch(type)
					{
					case 0:
						//shit, damn hard
					}
				}
			}
		}
	}
	/*
	char bitmap=*(rmFileScan->pRMFileHandle->file->pBitmap+((rmFileScan->pn+1)/8));
	while((bitmap&(0x01<<(rmFileScan->pn%8)))==0) //未分配页
	{
		rmFileScan->pn++;
		if(rmFileScan->pn==(rmFileScan->pRMFileHandle->file->pFileSubHeader->pageCount-1))
			return RM_NOMORERECINMEM;
		bitmap=*(rmFileScan->pRMFileHandle->file->pBitmap+((rmFileScan->pn+1)%8));
	}

	return SUCCESS;
	*/
	rec->bValid=FALSE;
	return RM_NOMORERECINMEM;
}

RC GetRec (RM_FileHandle *fileHandle,RID *rid, RM_Record *rec) 
{
	//首先判断rid是否有效（页是否被分配，记录是否有效）
	rec->bValid=FALSE;
	//char bitmapPage=*(fileHandle->file->pBitmap+rid->pageNum/8);
	if(!fileHandle->pageCtlBitmap->atPos(rid->pageNum)/*(bitmapPage&(0x01<<(rid->pageNum%8))==0)*/||(rid->pageNum>fileHandle->file->pFileSubHeader->pageCount)||(rid->slotNum>fileHandle->recPerPage-1))
		return RM_INVALIDRID;
	PF_PageHandle *targetPage;
	if(GetThisPage(fileHandle->file,rid->pageNum,targetPage))
		return FAIL;
	char bitmapRec=*(targetPage->pFrame->page.pData+rid->slotNum/8);
	if(bitmapRec&(0x01<<(rid->slotNum%8))==0)
		return RM_INVALIDRID;
	//RID有效
	rid->bValid=TRUE;
	rec->bValid=TRUE;
	rec->pData=targetPage->pFrame->page.pData+fileHandle->recOffset+rid->slotNum*fileHandle->recSize;
	return SUCCESS;
}

RC InsertRec (RM_FileHandle *fileHandle,char *pData, RID *rid)
{
	/*************
	插入记录时，首先在文件中找到一个非满页（若没有则需要申请一个新的页面），
	在该页中找到一个空插槽并插入记录。记录插入后，要更新控制页，
	即将文件包含的记录数加1，同时将插槽位图中的相应位置1。
	如果记录插入后，该页面已经没有空插槽，则将该页面标记为满页，
	即在记录文件的控制页的位图上将该页面所对应的位置1。
	*************/
	//若没有未满页面且全部页面已经分配
	if(!fileHandle->recCtlBitmap->anyZero())
		return FAIL;
	//首先查找是否存在已分配的未满页
	int unfillPage=fileHandle->recCtlBitmap->firstZero(2);
	while((!fileHandle->pageCtlBitmap->atPos(unfillPage))&&unfillPage<fileHandle->bitmapLength*8)
		unfillPage=fileHandle->recCtlBitmap->firstZero(unfillPage+1);
	if(unfillPage>=fileHandle->bitmapLength*8)    //若没有已经分配的未满页，分配新页
	{
		if(!fileHandle->pageCtlBitmap->anyZero())   //已经没有空闲页面
			return FAIL;
		//还有空闲页面（或未分配页面）
		PF_PageHandle *newPage;
		if(AllocatePage(fileHandle->file,newPage))
			return FAIL;
		bitmanager bmpNewPage(fileHandle->bitmapLength,newPage->pFrame->page.pData,1/*去掉*/);
		memcpy(newPage->pFrame->page.pData+fileHandle->recOffset,pData,fileHandle->recSize);
		bmpNewPage.setBitmap(2,1);
	}
	else    //存在已经分配的未满页，页号为unfillPage
	{
		PF_PageHandle *page;
		if(GetThisPage(fileHandle->file,unfillPage,page))
			return FAIL;
		//通过位图找到空槽
		bitmanager bmp(fileHandle->bitmapLength,page->pFrame->page.pData,fileHandle->bitmapLength*8);
		int emptySlot=bmp.firstZero(0);
		//存入记录
		memcpy(page->pFrame->page.pData+fileHandle->recOffset+emptySlot*fileHandle->recSize,pData,fileHandle->recSize);
		//更新槽位图
		bmp.setBitmap(emptySlot,1);
		//若页面已满，更新记录控制位图
		if(!bmp.anyZero())
			fileHandle->recCtlBitmap->setBitmap(unfillPage,1);
	}
	//更新记录数
	fileHandle->recNum++;
	return SUCCESS;
}

RC DeleteRec (RM_FileHandle *fileHandle,const RID *rid)
{
	//检测RID有效性
	if(rid->pageNum>fileHandle->file->pFileSubHeader->pageCount||(!fileHandle->pageCtlBitmap->atPos(rid->pageNum)))
		return RM_INVALIDRID;
	//获得目标页的句柄
	PF_PageHandle *target;
	if(GetThisPage(fileHandle->file,rid->pageNum,target))
		return FAIL;
	//检查记录是否有效
	if(
	return SUCCESS;
}

RC UpdateRec (RM_FileHandle *fileHandle,const RM_Record *rec)
{

	return SUCCESS;
}

RC RM_CreateFile (char *fileName, int recordSize)
{
	int rtn=CreateFile(fileName);
	if(rtn)
		return FAIL;  //返回值需要考虑一下
	//如果成功，生成记录信息控制页，首先获得创建文件的句柄
	PF_FileHandle *file;
	rtn=openFile(fileName,file);
	if(rtn)
		return FAIL;  //返回值需要考虑一下
	//申请新页面
	PF_PageHandle *ctrPage;
	rtn=AllocatePage(file,ctrPage);
	if(rtn)
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
	CloseFile(file);
	return SUCCESS;
}

RC RM_OpenFile(char *fileName, RM_FileHandle *fileHandle)
{
	if(fileHandle->bOpen)  //若使用的句柄已经对应一个打开的文件
		return RM_FHOPENNED;
	PF_FileHandle *filePF;
	if(openFile(fileName,filePF))
		return FAIL;
	fileHandle->bOpen=TRUE;
	//fileHandle->fileName=fileName;
	//fileHandle->file[0]=filePF;
	fileHandle->file=filePF;
	//获取记录管理基本信息
	PF_PageHandle *ctrPage;
	if(GetThisPage(filePF,1,ctrPage))
	{
		CloseFile(filePF);
		return FAIL;
	}
	RM_recControl *recCtl;
	recCtl=(RM_recControl *)ctrPage->pFrame->page.pData;
	fileHandle->recNum=recCtl->recNum;
	fileHandle->recOffset=recCtl->recordOffset;
	fileHandle->recPerPage=recCtl->recPerPage;
	fileHandle->recSize=recCtl->recSize;
	fileHandle->bitmapLength=PF_PAGE_SIZE-sizeof(RM_recControl); //可以分配的最大页数/8
	//获取记录管理位图
	fileHandle->recCtlBitmap=new bitmanager(fileHandle->bitmapLength,ctrPage->pFrame->page.pData+sizeof(RM_recControl),fileHandle->file->pFileSubHeader->pageCount+1);
	//获取页面管理位图
	fileHandle->pageCtlBitmap=new bitmanager(fileHandle->bitmapLength,filePF->pBitmap,fileHandle->file->pFileSubHeader->pageCount+1);
	/*
	//确定总共多少个分页文件，需要读取
	PF_PageHandle *ctrPage;
	if(GetThisPage(filePF,1,ctrPage))
		return FAIL;
	RM_recControl *control;
	control=(RM_recControl *)ctrPage->pFrame->page.pData;
	fileHandle->fileNum=control->fileNum;
	//若使用了多个分页文件存储，逐个打开
	if(fileHandle->fileNum>1)
	{
		char name[sizeof(fileName)+2];  //用于组合新文件名
		strcpy(name,fileName);
		name[sizeof(fileName)+1]='\0';  //确保字符串结尾
		for(int i=1;i<fileHandle->fileNum;i++)
		{
			name[sizeof(fileName)]=(char)(i+48);
			if(openFile(name,fileHandle->file[i]))  //修改
			{
				for(int close=0;close<i;close++)       //其中一个文件打开失败则失败，将已经打开的关闭
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
	//若已经关闭
	if(!fileHandle->bOpen)
		return RM_FHCLOSED;
	if(CloseFile(fileHandle->file))
			return FAIL;
	/*
	//将所有有关的分页文件全部关闭
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

RC的RM部分：

RM_FH CLOSED,
RM_FH OPENNED,
RM_INVALID RECSIZE,
RM_INVALID RID,
RM_FS CLOSED,
RM_NO MORE REC IN MEM,
RM_FS OPEN,
RM_NO MORE IDX IN MEM,
*/