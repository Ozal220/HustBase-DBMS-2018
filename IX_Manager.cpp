#include "stdafx.h"
#include "IX_Manager.h"
// Ozel writing
RC OpenIndexScan(IX_IndexScan *indexScan,IX_IndexHandle *indexHandle,CompOp compOp,char *value){
	return SUCCESS;
}

RC IX_GetNextEntry(IX_IndexScan *indexScan,RID * rid){
	return SUCCESS;
}

RC CloseIndexScan(IX_IndexScan *indexScan){	
	return SUCCESS;
}

RC GetIndexTree(char *fileName, Tree *index){
		return SUCCESS;
}

RC CreateIndex(const char * fileName,AttrType attrType,int attrLength){
	int rtn=CreateFile(fileName);
	if(rtn)
		return FAIL;  
	//如果成功，生成记录信息控制页，首先获得创建文件的句柄
	PF_FileHandle *file=NULL;
	rtn=openFile((char *)fileName,file);
	if(rtn)
		return FAIL; 

	//申请新页面
	PF_PageHandle *ctrPage=NULL;
	rtn=AllocatePage(file,ctrPage);
	if(rtn)
		return FAIL;
	// 页面上添加<索引控制信息>，其中rootPage和first_leaf默认设为1页，有误后期改
	IX_FileHeader *fileHeader = (IX_FileHeader *)ctrPage->pFrame->page.pData;
	fileHeader->attrLength = attrLength;
	fileHeader->attrType = attrType;
	fileHeader->first_leaf = 1;
	//fileHeader->keyLength = attrLength;		// 索引关键字的长度keyLength为索引列值的长度+记录ID的长度
	fileHeader->order = (PF_PAGE_SIZE-sizeof(IX_FileHeader))/(2*sizeof(RID)+attrLength);// m=(PF_PAGE_SIZE-控制信息长度)/(2*sizeof(RID)+atrrLength) 
	fileHeader->rootPage = 1;				

	// 在<索引控制信息>后添加<节点控制信息>
	IX_Node *ixNode = (IX_Node *)ctrPage->pFrame->page.pData[sizeof(IX_FileHeader)];
	ixNode->is_leaf = 1;		// 默认为是叶子结点
	ixNode->keynum = 0;
	//ixNode->parent = 0;
	//ixNode->brother = 0;
	//ixNode->keys = ass;   关键字区是从哪开始啊？
	//ixNode->rids = shit;	指针区又从哪开始？

	// 紧接IX_Node结构之后，从pData[sizeof(IX_FileHeader)+ sizeof(IX_Node)]开始，存放B+树节点信息
	Tree *bTree = (Tree *)(IX_FileHeader *)ctrPage->pFrame->page.pData[sizeof(IX_FileHeader)+ sizeof(IX_Node)];
	bTree->attrLength = attrLength;
	bTree->attrType = attrType;
	bTree->order = (PF_PAGE_SIZE-sizeof(IX_FileHeader))/(2*sizeof(RID)+attrLength);
	//bTree->root = null;   根结点从哪开始

	//关闭打开的文件
	CloseFile(file);
	return SUCCESS;
}

RC OpenIndex(const char *fileName,IX_IndexHandle *indexHandle) {
	//判断文件是否已打开
	if(indexHandle->bOpen)  //若使用的句柄已经对应一个打开的文件
		return RM_FHOPENNED;
	PF_FileHandle *filePF=NULL;
	if(openFile((char*)fileName,filePF))
		return FAIL;
	indexHandle->bOpen=TRUE;
	
	indexHandle->fileHandle = *filePF;	//这一行不敢确定

	//获取记录管理基本信息
	PF_PageHandle *ctrPage=NULL;
	if(GetThisPage(filePF,1,ctrPage))
	{
		CloseFile(filePF);
		return FAIL;
	}

	return SUCCESS;
}

RC CloseIndex(IX_IndexHandle *indexHandle){
	//若已经关闭
	if(!indexHandle->bOpen)
		return IX_ISCLOSED;
	//if(CloseFile(indexHandle.fileHandle))	// 用filename关闭文件? 关闭文件没有对应的数据结构
		//	return FAIL;
	/*
	//将所有有关的分页文件全部关闭
	for(int close=0;close<indexHandle->fileHandle.fileNum;close++)
	{
		if(CloseFile(indexHandle->fileHandle.file[close]))
			return FAIL;
	}
	*/
	indexHandle->bOpen=FALSE;
	return SUCCESS;
}