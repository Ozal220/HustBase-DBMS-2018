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
	//����ɹ������ɼ�¼��Ϣ����ҳ�����Ȼ�ô����ļ��ľ��
	PF_FileHandle *file=NULL;
	rtn=openFile((char *)fileName,file);
	if(rtn)
		return FAIL; 

	//������ҳ��
	PF_PageHandle *ctrPage=NULL;
	rtn=AllocatePage(file,ctrPage);
	if(rtn)
		return FAIL;
	// ҳ�������<����������Ϣ>������rootPage��first_leafĬ����Ϊ1ҳ��������ڸ�
	IX_FileHeader *fileHeader = (IX_FileHeader *)ctrPage->pFrame->page.pData;
	fileHeader->attrLength = attrLength;
	fileHeader->attrType = attrType;
	fileHeader->first_leaf = 1;
	//fileHeader->keyLength = attrLength;		// �����ؼ��ֵĳ���keyLengthΪ������ֵ�ĳ���+��¼ID�ĳ���
	fileHeader->order = (PF_PAGE_SIZE-sizeof(IX_FileHeader))/(2*sizeof(RID)+attrLength);// m=(PF_PAGE_SIZE-������Ϣ����)/(2*sizeof(RID)+atrrLength) 
	fileHeader->rootPage = 1;				

	// ��<����������Ϣ>�����<�ڵ������Ϣ>
	IX_Node *ixNode = (IX_Node *)ctrPage->pFrame->page.pData[sizeof(IX_FileHeader)];
	ixNode->is_leaf = 1;		// Ĭ��Ϊ��Ҷ�ӽ��
	ixNode->keynum = 0;
	//ixNode->parent = 0;
	//ixNode->brother = 0;
	//ixNode->keys = ass;   �ؼ������Ǵ��Ŀ�ʼ����
	//ixNode->rids = shit;	ָ�����ִ��Ŀ�ʼ��

	// ����IX_Node�ṹ֮�󣬴�pData[sizeof(IX_FileHeader)+ sizeof(IX_Node)]��ʼ�����B+���ڵ���Ϣ
	Tree *bTree = (Tree *)(IX_FileHeader *)ctrPage->pFrame->page.pData[sizeof(IX_FileHeader)+ sizeof(IX_Node)];
	bTree->attrLength = attrLength;
	bTree->attrType = attrType;
	bTree->order = (PF_PAGE_SIZE-sizeof(IX_FileHeader))/(2*sizeof(RID)+attrLength);
	//bTree->root = null;   �������Ŀ�ʼ

	//�رմ򿪵��ļ�
	CloseFile(file);
	return SUCCESS;
}

RC OpenIndex(const char *fileName,IX_IndexHandle *indexHandle) {
	//�ж��ļ��Ƿ��Ѵ�
	if(indexHandle->bOpen)  //��ʹ�õľ���Ѿ���Ӧһ���򿪵��ļ�
		return RM_FHOPENNED;
	PF_FileHandle *filePF=NULL;
	if(openFile((char*)fileName,filePF))
		return FAIL;
	indexHandle->bOpen=TRUE;
	
	indexHandle->fileHandle = *filePF;	//��һ�в���ȷ��

	//��ȡ��¼���������Ϣ
	PF_PageHandle *ctrPage=NULL;
	if(GetThisPage(filePF,1,ctrPage))
	{
		CloseFile(filePF);
		return FAIL;
	}

	return SUCCESS;
}

RC CloseIndex(IX_IndexHandle *indexHandle){
	//���Ѿ��ر�
	if(!indexHandle->bOpen)
		return IX_ISCLOSED;
	//if(CloseFile(indexHandle.fileHandle))	// ��filename�ر��ļ�? �ر��ļ�û�ж�Ӧ�����ݽṹ
		//	return FAIL;
	/*
	//�������йصķ�ҳ�ļ�ȫ���ر�
	for(int close=0;close<indexHandle->fileHandle.fileNum;close++)
	{
		if(CloseFile(indexHandle->fileHandle.file[close]))
			return FAIL;
	}
	*/
	indexHandle->bOpen=FALSE;
	return SUCCESS;
}