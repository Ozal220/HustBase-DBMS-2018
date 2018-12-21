#include "stdafx.h"
#include "IX_Manager.h"
// 测试一下吾孜第二次连接git
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

RC InsertEntry(IX_IndexHandle *indexHandle,void *pData,const RID * rid)
{
	return FAIL;
}

RC DeleteEntry(IX_IndexHandle *indexHandle,void *pData,const RID * rid)
{
	return FAIL;
}

RC CreateIndex(const char * fileName,AttrType attrType,int attrLength){
	if(CreateFile(fileName))
		return FAIL;  
	//如果成功
	PF_FileHandle *file=NULL;
	if(openFile((char *)fileName,file))
		return FAIL;
	//申请新页面用于存放索引首页（根节点）
	PF_PageHandle *firstPage=NULL;
	if(AllocatePage(file,firstPage))
		return FAIL;
	// 页面上添加<索引控制信息>，其中rootPage和first_leaf默认设为1页，有误后期改
	IX_FileHeader *fileHeader = (IX_FileHeader *)firstPage->pFrame->page.pData;
	fileHeader->attrLength = attrLength;
	fileHeader->attrType = attrType;
	fileHeader->first_leaf = 1;
	fileHeader->keyLength = attrLength+sizeof(RID);
	//why 2*sizeof(RID)
	fileHeader->order = (PF_PAGE_SIZE-sizeof(IX_FileHeader)-sizeof(IX_Node))/(2*sizeof(RID)+attrLength);
	fileHeader->rootPage = 1;				

	// 在<索引控制信息>后添加<节点控制信息>
	IX_Node *ixNode = (IX_Node *)(firstPage->pFrame->page.pData+sizeof(IX_FileHeader));
	ixNode->is_leaf = 1;		// 默认为是叶子结点
	ixNode->keynum = 0;
	ixNode->parent = 1;
	ixNode->brother = 1;
	ixNode->keys = (char *)(firstPage->pFrame->page.pData+sizeof(IX_FileHeader)+sizeof(IX_Node));
	ixNode->rids = (RID *)ixNode->keys+fileHeader->order*fileHeader->attrLength;
	/*
	// 紧接IX_Node结构之后，从pData[sizeof(IX_FileHeader)+ sizeof(IX_Node)]开始，存放B+树节点信息
	Tree *bTree = (Tree *)(IX_FileHeader *)ctrPage->pFrame->page.pData[sizeof(IX_FileHeader)+ sizeof(IX_Node)];
	bTree->attrLength = attrLength;
	bTree->attrType = attrType;
	bTree->order = (PF_PAGE_SIZE-sizeof(IX_FileHeader))/(2*sizeof(RID)+attrLength);
	//bTree->root = null;   根结点从哪开始
	*/
	//关闭打开的文件
	CloseFile(file);
	return SUCCESS;
}

RC OpenIndex(const char *fileName,IX_IndexHandle *indexHandle) {
	//判断文件是否已打开
	if(indexHandle->bOpen)  //若使用的句柄已经对应一个打开的文件
		return RM_FHOPENNED;
	if(openFile((char*)fileName,indexHandle->fileHandle))
		return FAIL;
	indexHandle->bOpen=TRUE;
	//获取记录管理基本信息
	PF_PageHandle *ctrPage=NULL;
	if(GetThisPage(indexHandle->fileHandle,1,ctrPage))
	{
		CloseFile(indexHandle->fileHandle);
		return FAIL;
	}
	indexHandle->fileHeader=(IX_FileHeader *)ctrPage->pFrame->page.pData;
	return SUCCESS;
}

RC CloseIndex(IX_IndexHandle *indexHandle){
	//若已经关闭
	if(!indexHandle->bOpen)
		return IX_ISCLOSED;
	if(CloseFile(indexHandle->fileHandle))	// 用filename关闭文件? 关闭文件没有对应的数据结构
		return FAIL;
	indexHandle->bOpen=FALSE;
	return SUCCESS;
}