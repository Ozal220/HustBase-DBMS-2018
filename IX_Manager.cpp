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

//12/24
RC InsertEntry(IX_IndexHandle *indexHandle,void *pData,const RID * rid)
{
	//�����ҵ�����ڵ�









	//�ݹ���ýڵ���봦��������B+���ṹ



	return FAIL;
}

RC DeleteEntry(IX_IndexHandle *indexHandle,void *pData,const RID * rid)
{

	return FAIL;
}

RC CreateIndex(const char * fileName,AttrType attrType,int attrLength){
	if(CreateFile(fileName))
		return FAIL;  
	//����ɹ�
	PF_FileHandle *file=NULL;
	if(openFile((char *)fileName,file))
		return FAIL;
	//������ҳ�����ڴ��������ҳ�����ڵ㣩
	PF_PageHandle *firstPage=NULL;
	if(AllocatePage(file,firstPage))
		return FAIL;
	// ҳ�������<����������Ϣ>������rootPage��first_leafĬ����Ϊ1ҳ��������ڸ�
	IX_FileHeader *fileHeader = (IX_FileHeader *)firstPage->pFrame->page.pData;
	fileHeader->attrLength = attrLength;
	fileHeader->attrType = attrType;
	fileHeader->first_leaf = 1;
	fileHeader->keyLength = attrLength+sizeof(RID);
	//why 2*sizeof(RID)
	//��һ��Ϊ������һ��λ��ʹ��ÿ���ڵ�洢�Ĺؼ�����������ʱ��������1��
	fileHeader->order = (PF_PAGE_SIZE-sizeof(IX_FileHeader)-sizeof(IX_Node))/(2*sizeof(RID)+attrLength)-1;
	fileHeader->rootPage = 1;				
	// ��<����������Ϣ>�����<�ڵ������Ϣ>
	IX_Node *ixNode = (IX_Node *)(firstPage->pFrame->page.pData+sizeof(IX_FileHeader));
	ixNode->is_leaf = 1;		// Ĭ��Ϊ��Ҷ�ӽ��
	ixNode->keynum = 0;
	ixNode->parent = 1;
	ixNode->brother = 1;
	ixNode->keys = (char *)(firstPage->pFrame->page.pData+sizeof(IX_FileHeader)+sizeof(IX_Node));
	ixNode->rids = (RID *)(ixNode->keys+fileHeader->order*fileHeader->keyLength);
	/*
	// ����IX_Node�ṹ֮�󣬴�pData[sizeof(IX_FileHeader)+ sizeof(IX_Node)]��ʼ�����B+���ڵ���Ϣ
	Tree *bTree = (Tree *)(IX_FileHeader *)ctrPage->pFrame->page.pData[sizeof(IX_FileHeader)+ sizeof(IX_Node)];
	bTree->attrLength = attrLength;
	bTree->attrType = attrType;
	bTree->order = (PF_PAGE_SIZE-sizeof(IX_FileHeader))/(2*sizeof(RID)+attrLength);
	//bTree->root = null;   �������Ŀ�ʼ
	*/
	//�رմ򿪵��ļ�
	CloseFile(file);
	return SUCCESS;
}

RC OpenIndex(const char *fileName,IX_IndexHandle *indexHandle) {
	//�ж��ļ��Ƿ��Ѵ�
	if(indexHandle->bOpen)  //��ʹ�õľ���Ѿ���Ӧһ���򿪵��ļ�
		return RM_FHOPENNED;
	if(openFile((char*)fileName,indexHandle->fileHandle))
		return FAIL;
	indexHandle->bOpen=TRUE;
	//��ȡ��¼���������Ϣ
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
	//���Ѿ��ر�
	if(!indexHandle->bOpen)
		return IX_ISCLOSED;
	if(CloseFile(indexHandle->fileHandle))	// ��filename�ر��ļ�? �ر��ļ�û�ж�Ӧ�����ݽṹ
		return FAIL;
	indexHandle->bOpen=FALSE;
	return SUCCESS;
}

//attrLength ����RID�ĳ���
int insertKey(char *key, RID *val, int *effectiveLength, char *keyInsert, RID valInsert, AttrType type, int attrLength)
{
	int keyOffset;
	//��������key���ҵ�����λ��
	switch(type)
	{
	case 0://�ַ����ıȽ�
		for (keyOffset=0;keyOffset<(*effectiveLength);keyOffset++)
		{
			int rtn=strcmp(keyInsert+sizeof(RID),key+keyOffset*attrLength+sizeof(RID));
			if(rtn<=0)
			{
				if(rtn==0)
				{
					//��һ���Ƚ�RID
					if(((RID *)keyInsert)->pageNum==((RID *)key+keyOffset*attrLength)->pageNum)
					{
						if(((RID *)keyInsert)->slotNum==((RID *)key+keyOffset*attrLength)->slotNum)
						{
							//�������key�Ѵ��ڣ�����ֵ��RID)
							*((RID *)(val+keyOffset*sizeof(RID)))=valInsert;
							return *effectiveLength;
						}
						else if(((RID *)keyInsert)->slotNum>((RID *)key+keyOffset*attrLength)->slotNum)
							continue;
					}
					else if(((RID *)keyInsert)->pageNum>((RID *)key+keyOffset*attrLength)->pageNum)
						continue;
				}
				return KeyShift(keyOffset,key,val,effectiveLength,keyInsert,valInsert,attrLength);
			}
			//������ȵ�ǰ�Աȼ���
			else
				continue;
		}
		//�Ƚ�keyOffset��ֵ
		break;
	case 1:
	case 2: //int�Լ�float�ıȽ�
		for (keyOffset=0;keyOffset<(*effectiveLength);keyOffset++)
		{
			if(*((float *)keyInsert+sizeof(RID))<=*((float *)(key+keyOffset*attrLength+sizeof(RID))))
			{
				if(*((float *)keyInsert+sizeof(RID))==*((float *)(key+keyOffset*attrLength+sizeof(RID))))
				{
					//��һ���Ƚ�RID
					if(((RID *)keyInsert)->pageNum==((RID *)key+keyOffset*attrLength)->pageNum)
					{
						if(((RID *)keyInsert)->slotNum==((RID *)key+keyOffset*attrLength)->slotNum)
						{
							//�������key�Ѵ��ڣ�����ֵ��RID)
							*((RID *)(val+keyOffset*sizeof(RID)))=valInsert;
							return *effectiveLength;
						}
						else if(((RID *)keyInsert)->slotNum>((RID *)key+keyOffset*attrLength)->slotNum)
							continue;
					}
					else if(((RID *)keyInsert)->pageNum>((RID *)key+keyOffset*attrLength)->pageNum)
							continue;
				}
				return KeyShift(keyOffset,key,val,effectiveLength,keyInsert,valInsert,attrLength);
			}
			//������ȵ�ǰ�Աȼ���
			else
				continue;
		}
		break;
	default:
		break;
	}
}

int KeyShift(int keyOffset,char *key, RID *val, int *effectiveLength, char *keyInsert, RID valInsert, int attrLength)
{
	//�ؼ���������λ������ÿ���ڵ��Ѿ�����һ����λ�����赣�Ľڵ��������
	char *buffer=(char *)malloc((*effectiveLength-keyOffset-1)*attrLength);
	memcpy(buffer,key+keyOffset*attrLength,(*effectiveLength-keyOffset-1)*attrLength);
	memset(key+keyOffset*attrLength,0,(*effectiveLength-keyOffset-1)*attrLength);
	memcpy(key+(keyOffset+1)*attrLength,buffer,(*effectiveLength-keyOffset-1)*attrLength);
	//�ؼ�����������µ�����
	strcpy(key+keyOffset*attrLength,keyInsert);
	free(buffer);
	//ֵ����λ
	RID *valBuffer=(RID *)malloc((*effectiveLength-keyOffset-1)*sizeof(RID));
	memcpy(buffer,val+keyOffset*sizeof(RID),(*effectiveLength-keyOffset-1)*sizeof(RID));
	memset(val+keyOffset*sizeof(RID),0,(*effectiveLength-keyOffset-1)*sizeof(RID));
	memcpy(val+(keyOffset+1)*sizeof(RID),buffer,(*effectiveLength-keyOffset-1)*sizeof(RID));
	//ֵ������������
	*((RID *)(val+keyOffset*sizeof(RID)))=valInsert;
	free(valBuffer);
	//��ɼ�ֵ�ԵĲ��룬�����µĽڵ���Ч���ݴ�С
	return ++(*effectiveLength);
}

//����/ɾ���ж�λ��Ҫ���в����Ľڵ㣬����Ŀ��ڵ��ҳ����ָ��
PF_PageHandle *FindNode(IX_IndexHandle *indexHandle,char *targetKey)
{
	//��λ���ڵ�
	int rootPage=indexHandle->fileHeader->rootPage;
	PF_PageHandle *currentPage;
	int rtn;
	float targetVal,indexVal;
	GetThisPage(indexHandle->fileHandle,rootPage,currentPage);
	IX_Node *nodeInfo;
	nodeInfo=(IX_Node *)(currentPage->pFrame->page.pData[sizeof(IX_FileHeader)]);
	int isLeaf=nodeInfo->is_leaf;
	//һֱ����ֱ������һ��Ҷ�ӽڵ�
	int offset;
	while(isLeaf==0)
	{
		for(offset=0; offset<nodeInfo->keynum;offset++)
		{
			switch(indexHandle->fileHeader->attrType)
			{
			case 0:
				rtn=strcmp(targetKey+sizeof(RID),
					nodeInfo->keys+offset*indexHandle->fileHeader->keyLength+sizeof(RID));
			case 1:
			case 2:
				targetVal=*(float *)(targetKey+sizeof(RID));
				indexVal=*(float *)(nodeInfo->keys+offset*indexHandle->fileHeader->keyLength+sizeof(RID));
				rtn=(targetVal<indexVal)?-1:((targetVal==indexVal)?0:1);
				break;
			default:
				break;
			}
			if(rtn>0)
				continue; //������һ����
			else if(rtn==0)
			{
				//��һ���Ƚ�rid
				int currentPageNum=((RID *)(nodeInfo->keys+offset*indexHandle->fileHeader->keyLength))->pageNum;
				if(((RID *)targetKey)->pageNum>currentPageNum)
					continue;
				else if(((RID *)targetKey)->pageNum==currentPageNum)
				{
					int currentSlotNum=((RID *)(nodeInfo->keys+offset*indexHandle->fileHeader->keyLength))->slotNum;
					if(((RID *)targetKey)->slotNum>currentSlotNum)
						continue;
					if(((RID *)targetKey)->slotNum)
						offset++;   //��Ϊ����offsetҪ��һ��������ļ����ڽڵ㣨�����ڵ㣩�иպô���ʱ��
					//ʹ�����ڽڵ��ڵ���һ������Ӧ��ָ����Ϊ��һ�����ǵ��ӽڵ�ָ��
				}
			}
			//������Ӧ���ӽڵ�
			RID child=(RID)nodeInfo->rids[offset==0?0:offset-1];
			GetThisPage(indexHandle->fileHandle,child.pageNum,currentPage);
			nodeInfo=(IX_Node *)(currentPage->pFrame->page.pData[sizeof(IX_FileHeader)]);
			int isLeaf=nodeInfo->is_leaf;
			break;
		}
	}
	return currentPage;
}