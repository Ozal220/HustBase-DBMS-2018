#ifndef RM_MANAGER_H_H
#define RM_MANAGER_H_H

#include "PF_Manager.h"
#include "str.h"
#include "bitmanager.h"

typedef int SlotNum;

typedef struct {	
	PageNum pageNum;	//记录所在页的页号
	SlotNum slotNum;		//记录的插槽号
	bool bValid; 			//true表示为一个有效记录的标识符
}RID;

typedef struct{
	bool bValid;		 // False表示还未被读入记录
	RID  rid; 		 // 记录的标识符 
	char *pData; 		 //记录所存储的数据 
}RM_Record;

//定义记录信息结构，参考指导书的设计
typedef struct{
	int recNum;
	int recSize;
	int recPerPage;
	int recordOffset; //首条记录偏移量(由于位图的存在及其大小不变)
	//int fileNum; //该表使用的分页文件数
}RM_recControl;

typedef struct
{
	int bLhsIsAttr,bRhsIsAttr;//左、右是属性（1）还是值（0）
	AttrType attrType;
	int LattrLength,RattrLength;
	int LattrOffset,RattrOffset;
	CompOp compOp;
	void *Lvalue,*Rvalue;
}Con;

typedef struct{//文件句柄
	bool bOpen;//句柄是否打开（是否正在被使用）
	//需要自定义其内部结构
	
	/*********************
	根据本实验中分页文件的定义方法，一个分页文件大小至多为127MB。
	现考虑存在一个巨大的表，其大小大于分页文件总大小，此时需要多个
	分页文件存储
	*********************/
	/*先不管多个文件的情况了
	char *fileName;  //文件名（表名）
	int fileNum;  //使用的分页文件数量
	PF_FileHandle *file[5];  //最多5个分页文件的大小
	*/

	//存储记录的基本信息，与记录信息结构类似，使得打开文件后马上能获得记录管理信息，以后不需要多次访问记录管理页面
	//但该信息需要及时更新
	int recNum;
	int recSize;
	int recPerPage;
	int recOffset;
	int bitmapLength;  //控制页的位图大小（按字节计）
	bitmanager *pageCtlBitmap;   //页面信息的位图管理对象
	bitmanager *recCtlBitmap;    //记录信息的位图管理对象
	PF_FileHandle *file;
}RM_FileHandle;

typedef struct{
	bool  bOpen;		//扫描是否打开 
	RM_FileHandle  *pRMFileHandle;		//扫描的记录文件句柄
	int  conNum;		//扫描涉及的条件数量 
	Con  *conditions;	//扫描涉及的条件数组指针
    PF_PageHandle  PageHandle; //处理中的页面句柄
	PageNum  pn; 	//扫描即将处理的页面号
	SlotNum  sn;		//扫描即将处理的插槽号
}RM_FileScan;



RC GetNextRec(RM_FileScan *rmFileScan,RM_Record *rec);

RC OpenScan(RM_FileScan *rmFileScan,RM_FileHandle *fileHandle,int conNum,Con *conditions);

RC CloseScan(RM_FileScan *rmFileScan);

RC UpdateRec (RM_FileHandle *fileHandle,const RM_Record *rec);

RC DeleteRec (RM_FileHandle *fileHandle,const RID *rid);

RC InsertRec (RM_FileHandle *fileHandle, char *pData, RID *rid); 

RC GetRec (RM_FileHandle *fileHandle, RID *rid, RM_Record *rec); 

RC RM_CloseFile (RM_FileHandle *fileHandle);

RC RM_OpenFile (char *fileName, RM_FileHandle *fileHandle);

RC RM_CreateFile (char *fileName, int recordSize);

#endif