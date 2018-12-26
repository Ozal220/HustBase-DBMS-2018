#include "stdafx.h"
#include "IX_Manager.h"

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
//注意处理返回值的问题
RC InsertEntry(IX_IndexHandle *indexHandle,void *pData,const RID *rid)
{
	PF_PageHandle *pageInsert=FindNode(indexHandle,pData); //根据输入的数据找到即将操作的节点
	//调用递归函数
	RecursionInsert(indexHandle,pData,rid,pageInsert);
	return FAIL;
}

//索引插入的递归调用
void RecursionInsert(IX_IndexHandle *indexHandle,void *pData,const RID *rid,PF_PageHandle *pageInsert)
{
	IX_Node *pageControl=(IX_Node *)(pageInsert->pFrame->page.pData+sizeof(IX_FileHeader));  //获得当前页的索引记录信息
	int posInsert=insertKey(pageControl->keys,pageControl->rids,&pageControl->keynum,(char *)pData,  //强势插入一个索引项
		rid,indexHandle->fileHeader->attrType,indexHandle->fileHeader->keyLength);
	if(pageControl->keynum<indexHandle->fileHeader->order)
		return;  //索引项数没有超，皆大欢喜
	else
	{
		//索引项数达到了最大，节点分裂
		int splitOffset=int(pageControl->keynum/2+0.5);          //节点索引记录取半，向上取整
		PF_PageHandle *brotherNode;                              //为当前节点分配一个兄弟节点
		AllocatePage(indexHandle->fileHandle,brotherNode);
		pageControl->brother=brotherNode->pFrame->page.pageNum;  //标记兄弟节点的页号
		IX_Node *broPageControl=(IX_Node *)(brotherNode->pFrame->page.pData+sizeof(IX_FileHeader)); //兄弟节点的控制信息
		broPageControl->keys=brotherNode->pFrame->page.pData+sizeof(IX_FileHeader)+sizeof(IX_Node); //计算兄弟节点的索引区与数据区
		broPageControl->rids=(RID *)(broPageControl->keys+
			(indexHandle->fileHeader->order+1)*indexHandle->fileHeader->keyLength);
		//先向兄弟节点搬移分裂出去的索引数据
		broPageControl->keynum=(pageControl->keynum-splitOffset);   //设置兄弟节点的实际索引数
		memcpy(broPageControl->keys,
			pageControl->keys+splitOffset,
			broPageControl->keynum*indexHandle->fileHeader->keyLength); //搬移索引区数据
		pageControl->keynum=splitOffset;         //设置当前节点的实际索引数
		memcpy(broPageControl->rids,
			pageControl->rids+splitOffset,
			broPageControl->keynum*sizeof(RID)); //搬移指针区（值区）数据
		broPageControl->is_leaf=pageControl->is_leaf;  //标记兄弟节点叶子节点属性
		broPageControl->brother=-1;    //兄弟节点暂时没有右兄弟节点
		//检查是否是当前的根节点在分裂（是否有父结点）
		if(pageControl->parent==0)    //当前节点是根节点
		{
			PF_PageHandle *parentNode;
			AllocatePage(indexHandle->fileHandle,parentNode);
			IX_Node *parentPageControl=(IX_Node *)(parentNode->pFrame->page.pData+sizeof(IX_FileHeader));;  //父结点控制信息
			//初始化父结点信息
			parentPageControl->keynum=2; //只有两个索引项目
			parentPageControl->is_leaf=0;
			parentPageControl->parent=-1;
			parentPageControl->brother=-1;
			parentPageControl->keys=parentNode->pFrame->page.pData+sizeof(IX_FileHeader)+sizeof(IX_Node); //计算父节点的索引区与数据区
			parentPageControl->rids=(RID *)(parentPageControl->keys+
				(indexHandle->fileHeader->order+1)*indexHandle->fileHeader->keyLength);
			indexHandle->fileHeader->rootPage=parentNode->pFrame->page.pageNum;  //设置当前的根节点位置
			memcpy(parentPageControl->keys,pageControl->keys,indexHandle->fileHeader->keyLength);  //当前节点的第一个索引值
			parentPageControl->rids->bValid=true;
			parentPageControl->rids->pageNum=pageInsert->pFrame->page.pageNum;  //一个指针指向当前节点
			parentPageControl->rids->slotNum=0;   //内节点的指针的槽值都为0
			memcpy(parentPageControl->keys,broPageControl->keys,indexHandle->fileHeader->keyLength);  //兄弟节点的第一个索引值
			parentPageControl->rids->bValid=true;
			parentPageControl->rids->pageNum=brotherNode->pFrame->page.pageNum;  //一个指针指向兄弟节点
			parentPageControl->rids->slotNum=0;   //内节点的指针的槽值都为0
			pageControl->parent=parentNode->pFrame->page.pageNum;  //当前节点指向父结点
			broPageControl->parent=parentNode->pFrame->page.pageNum;  //兄弟节点指向父结点
			return;
		}
		else
		{
			broPageControl->parent=pageControl->parent;
			//递归调用插入父结点
			RID *broPointer;
			broPointer->bValid=true;
			broPointer->pageNum=brotherNode->pFrame->page.pageNum;
			broPointer->slotNum=0;
			PF_PageHandle *parentPage;
			GetThisPage(indexHandle->fileHandle,pageControl->parent,parentPage);
			if(posInsert!=0)  //前面插入的时候插在了当前节点的最左侧，需要更新父节点的索引值
				memcpy(parentPage->pFrame->page.pData,pageControl->keys,indexHandle->fileHeader->keyLength);
			RecursionInsert(indexHandle,broPageControl->keys,broPointer,parentPage);
		}
	}
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
	//减一是为了留出一个位置使得每个节点存储的关键字数可以暂时超过限制1个
	fileHeader->order = (PF_PAGE_SIZE-sizeof(IX_FileHeader)-sizeof(IX_Node))/(2*sizeof(RID)+attrLength)-1;
	fileHeader->rootPage = 1;				
	// 在<索引控制信息>后添加<节点控制信息>
	IX_Node *ixNode = (IX_Node *)(firstPage->pFrame->page.pData+sizeof(IX_FileHeader));
	ixNode->is_leaf = 1;		// 默认为是叶子结点
	ixNode->keynum = 0;
	ixNode->parent = 0;
	ixNode->brother = -1;
	ixNode->keys = (char *)(firstPage->pFrame->page.pData+sizeof(IX_FileHeader)+sizeof(IX_Node));
	ixNode->rids = (RID *)(ixNode->keys+(fileHeader->order+1)*fileHeader->keyLength);  //+1很重要，因为留出了一个单位的空间用于平衡节点的调度
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

//attrLength 包括RID的长度
int insertKey(char *key, RID *val, int *effectiveLength, char *keyInsert,const RID *valInsert, AttrType type, int attrLength)
{
	int keyOffset;
	//遍历已有key，找到插入位置
	switch(type)
	{
	case 0://字符串的比较
		for (keyOffset=0;keyOffset<(*effectiveLength);keyOffset++)
		{
			int rtn=strcmp(keyInsert+sizeof(RID),key+keyOffset*attrLength+sizeof(RID));
			if(rtn<=0)
			{
				if(rtn==0)
				{
					//进一步比较RID
					if(((RID *)keyInsert)->pageNum==((RID *)key+keyOffset*attrLength)->pageNum)
					{
						if(((RID *)keyInsert)->slotNum==((RID *)key+keyOffset*attrLength)->slotNum)
						{
							//若插入的key已存在，更新值（RID)
							*((RID *)(val+keyOffset*sizeof(RID)))=*valInsert;
							return keyOffset;
						}
						else if(((RID *)keyInsert)->slotNum>((RID *)key+keyOffset*attrLength)->slotNum)
							continue;
					}
					else if(((RID *)keyInsert)->pageNum>((RID *)key+keyOffset*attrLength)->pageNum)
						continue;
				}
				*effectiveLength=insertKeyShift(keyOffset,key,val,effectiveLength,keyInsert,valInsert,attrLength);
				return keyOffset;
			}
			//插入键比当前对比键大，则继续下一个循环
		}
		//比较keyOffset的值
		break;
	case 1:
	case 2: //int以及float的比较
		for (keyOffset=0;keyOffset<(*effectiveLength);keyOffset++)
		{
			if(*((float *)keyInsert+sizeof(RID))<=*((float *)(key+keyOffset*attrLength+sizeof(RID))))
			{
				if(*((float *)keyInsert+sizeof(RID))==*((float *)(key+keyOffset*attrLength+sizeof(RID))))
				{
					//进一步比较RID
					if(((RID *)keyInsert)->pageNum==((RID *)key+keyOffset*attrLength)->pageNum)
					{
						if(((RID *)keyInsert)->slotNum==((RID *)key+keyOffset*attrLength)->slotNum)
						{
							//若插入的key已存在，更新值（RID)
							*((RID *)(val+keyOffset*sizeof(RID)))=*valInsert;
							return keyOffset;
						}
						else if(((RID *)keyInsert)->slotNum>((RID *)key+keyOffset*attrLength)->slotNum)
							continue;
					}
					else if(((RID *)keyInsert)->pageNum>((RID *)key+keyOffset*attrLength)->pageNum)
							continue;
				}
				*effectiveLength=insertKeyShift(keyOffset,key,val,effectiveLength,keyInsert,valInsert,attrLength);
				return keyOffset;
			}
			//插入键比当前对比键大，则继续下一个循环
		}
		break;
	default:
		break;
	}
}

int deleteKey(char *key, RID *val, int *eLength, char *keyDelete, AttrType type, int attrLength){
	int keyOffset;
	switch (type)
	{
		
		case 0: //字符串比较
			for(keyOffset = 0; keyOffset < (*eLength); keyOffset++)
			{
				int rtn = strcmp(keyDelete + sizeof(RID), key + keyOffset*attrLength + sizeof(RID));
				if(rtn < 0) // 如果要删除的keyDelete小于目前key则跳出循环
					break;
				else if(rtn == 0) // 找到对应的key
				{
					//进一步比较RID
					if(((RID *)keyDelete)->pageNum == ((RID *)key + keyOffset * attrLength)->pageNum)	//页号
					{
						if(((RID *)keyDelete)->slotNum == ((RID *)key + keyOffset * attrLength)->slotNum) //槽号
						{
							//存在删除的key
							return deleteKeyShift(keyOffset,key,val,eLength,attrLength);
							}
						// 如果keyDelete槽号小于目前key的槽号则跳出循环
						else if(((RID *)keyDelete)->slotNum < ((RID *)key+keyOffset*attrLength)->slotNum)
							break;
						// 如果keyDelete槽号大于目前key的槽号则继续下一个循环
					}
					// 如果keyDelete页号小于目前key的页号则跳出循环
					else if(((RID *)keyDelete)->pageNum < ((RID *)key + keyOffset * attrLength)->pageNum)
						break;
					// 如果keyDelete页号大于目前key的页号则继续下一个循环
				}
				// 如果要删除的keyDelete大于目前查找的key则进行下一个循环
			}
			break;
		case 1:	//int
		case 2:	//float
			for(keyOffset = 0; keyOffset < (*eLength); keyOffset++)
			{
				int sub = *((float *)keyDelete + sizeof(RID)) - *((float *)(key + keyOffset*attrLength + sizeof(RID)));
				if(sub < 0) // 如果要删除的keyDelete小于目前key则跳出循环
					break;
				else if(sub == 0) // 找到对应的key
				{
					//进一步比较RID
					if(((RID *)keyDelete)->pageNum == ((RID *)key + keyOffset * attrLength)->pageNum)	//页号
					{
						if(((RID *)keyDelete)->slotNum == ((RID *)key + keyOffset * attrLength)->slotNum) //槽号
						{
							//存在删除的key
							return deleteKeyShift(keyOffset,key,val,eLength,attrLength);
						}
						// 如果keyDelete槽号小于目前key的槽号则跳出循环
						else if(((RID *)keyDelete)->slotNum < ((RID *)key+keyOffset*attrLength)->slotNum)
							break;
						// 如果keyDelete槽号大于目前key的槽号则继续下一个循环
					}
					// 如果keyDelete页号小于目前key的页号则跳出循环
					else if(((RID *)keyDelete)->pageNum < ((RID *)key + keyOffset * attrLength)->pageNum)
						break;
					// 如果keyDelete页号大于目前key的页号则继续下一个循环
				}
				// 如果要删除的keyDelete大于目前查找的key则进行下一个循环
			}
			break;
		default:
			break;
	}
}

// 对keyShift函数已更名,更名为insertKeyShift
int insertKeyShift(int keyOffset, char *key, RID *val, int *effectiveLength, char *keyInsert,const RID *valInsert, int attrLength)
{
	//关键字区域移位，由于每个节点已经多留一个空位，不需担心节点满的情况
	char *buffer=(char *)malloc((*effectiveLength-keyOffset-1)*attrLength);
	memcpy(buffer,key+keyOffset*attrLength,(*effectiveLength-keyOffset-1)*attrLength);
	memset(key+keyOffset*attrLength,0,(*effectiveLength-keyOffset-1)*attrLength);
	memcpy(key+(keyOffset+1)*attrLength,buffer,(*effectiveLength-keyOffset-1)*attrLength);
	//关键字区域插入新的数据
	strcpy(key+keyOffset*attrLength,keyInsert);
	free(buffer);
	//值区移位
	RID *valBuffer=(RID *)malloc((*effectiveLength-keyOffset-1)*sizeof(RID));
	memcpy(buffer,val+keyOffset*sizeof(RID),(*effectiveLength-keyOffset-1)*sizeof(RID));
	memset(val+keyOffset*sizeof(RID),0,(*effectiveLength-keyOffset-1)*sizeof(RID));
	memcpy(val+(keyOffset+1)*sizeof(RID),buffer,(*effectiveLength-keyOffset-1)*sizeof(RID));
	//值区插入新数据
	*((RID *)(val+keyOffset*sizeof(RID)))=*valInsert;
	free(valBuffer);
	//完成键值对的插入，返回新的节点有效数据大小
	return ++(*effectiveLength);
}

//����/ɾ���ж�λ��Ҫ���в����Ľڵ㣬����Ŀ��ڵ��ҳ����ָ��
PF_PageHandle *FindNode(IX_IndexHandle *indexHandle,void *targetKey)
{
	//定位根节点
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
				rtn=strcmp((char *)targetKey+sizeof(RID),
					nodeInfo->keys+offset*indexHandle->fileHeader->keyLength+sizeof(RID));
			case 1:
			case 2:
				targetVal=*((float *)targetKey+sizeof(RID));
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

int deleteKeyShift(int keyOffset, char *key, RID *val, int *eLength, int attrLength){
	// 关键字区域移动
	char *buffer = (char *)malloc((*eLength - keyOffset - 1) * attrLength);
	memcpy(buffer, key + (keyOffset + 1) * attrLength, (*eLength - keyOffset - 1) * attrLength); // +1 
	memcpy(key + keyOffset * attrLength, buffer, (*eLength - keyOffset - 1) * attrLength);
	free(buffer);

	// 值区移动
	RID *valBuffer=(RID *)malloc((*eLength - keyOffset - 1) * sizeof(RID));
	memcpy(buffer, val + (keyOffset + 1) * sizeof(RID), (*eLength - keyOffset - 1) * sizeof(RID)); // +1
	memcpy(val + keyOffset * sizeof(RID), buffer, (*eLength - keyOffset - 1) * sizeof(RID));
	free(valBuffer);

	//完成键值对的删除，返回新的节点有效数据大小
	return --(*eLength);

}