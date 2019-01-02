#include "stdafx.h"
#include "IX_Manager.h"
#include "RM_Manager.h" //使用比较函数

/****************
*optimization notepad
*1.索引插入的返回值（失败的条件）
*2.索引插入的左兄弟情况
*3.索引插入的旋转操作
*4.索引插入打断指针的情况
****************/

//12/27
RC OpenIndexScan(IX_IndexScan *indexScan,IX_IndexHandle *indexHandle,CompOp compOp,char *value)
{
	//初始化其他属性值
	indexScan->bOpen=true;
	indexScan->compOp=compOp;
	indexScan->pIXIndexHandle=indexHandle;
	indexScan->value=value;
	PF_PageHandle *pageStart;
	//初始化页面号、索引项编号、页面句柄
	switch (compOp) //小于、小于等于与等于从最小的索引项开始查找
	{
	case NO_OP:
	case LEqual:
	case LessT:
		indexScan->pnNext=indexHandle->fileHeader.first_leaf;
		indexScan->ridIx=0;
		GetThisPage(indexHandle->fileHandle,indexScan->pnNext,pageStart);
		indexScan->pfPageHandle=pageStart;
		indexScan->currentPageControl=(IX_Node *)(pageStart->pFrame->page.pData+sizeof(IX_FileHeader));
		return SUCCESS;
	default:
		break;
	}
	pageStart=FindNode(indexHandle,value);  //找到搜索开始的索引值所在节点
	IX_Node *startPageControl=(IX_Node *)(pageStart->pFrame->page.pData+sizeof(IX_FileHeader));  //获得开始页的索引记录信息
	int indexOffset,rtn;
	float targetVal,indexVal;
	for(indexOffset=0;indexOffset<startPageControl->keynum;indexOffset++)
	{
		switch(indexHandle->fileHeader.attrType)
		{
		case 0:
			rtn=strcmp((char *)value,
				startPageControl->keys+indexOffset*indexHandle->fileHeader.keyLength+sizeof(RID));
		case 1:
		case 2:
			targetVal=*(float *)value;
			indexVal=*(float *)(startPageControl->keys+indexOffset*indexHandle->fileHeader.keyLength+sizeof(RID));
			rtn=(targetVal<indexVal)?-1:((targetVal==indexVal)?0:1);
			break;
		default:
			break;
		}
		if(rtn==0)
		{
			indexScan->pnNext=pageStart->pFrame->page.pageNum;
			indexScan->ridIx=(compOp==EQual||compOp==GEqual)?indexOffset:indexOffset+1;
			indexScan->pfPageHandle=pageStart;
			indexScan->currentPageControl=startPageControl;
			return SUCCESS;
		}
		else if(rtn<0)
		{
			if(compOp==EQual)
				return FAIL;
			else
			{
				indexScan->pnNext=pageStart->pFrame->page.pageNum;
				indexScan->ridIx=indexOffset;
				indexScan->pfPageHandle=pageStart;
				indexScan->currentPageControl=startPageControl;
				return SUCCESS;
			}
		}
	}
	if(indexOffset==startPageControl->keynum)  //这是一种情况，当目标值大于某节点的所有值，而小于其右兄弟节点的最小值
	{
		if(compOp==EQual)
			return FAIL;
		else
		{
			indexScan->pnNext=startPageControl->brother;
			indexScan->ridIx=0;
			GetThisPage(indexHandle->fileHandle,startPageControl->brother,indexScan->pfPageHandle);
			indexScan->currentPageControl=(IX_Node *)(indexScan->pfPageHandle->pFrame->page.pData+sizeof(IX_FileHeader));
			return SUCCESS;
		}
	}
	return SUCCESS;
}

//检查比较策略
RC IX_GetNextEntry(IX_IndexScan *indexScan,RID * rid)
{
	if(indexScan->ridIx==indexScan->currentPageControl->keynum)
	{
		if(indexScan->currentPageControl->brother==-1)
			return FAIL;
		else
		{
			indexScan->pnNext=indexScan->currentPageControl->brother;
			GetThisPage(indexScan->pIXIndexHandle->fileHandle,indexScan->pnNext,indexScan->pfPageHandle);
			indexScan->currentPageControl=(IX_Node *)(indexScan->pfPageHandle->pFrame->page.pData+sizeof(IX_FileHeader));
			indexScan->ridIx=0;
		}
	}
	if(indexScan->compOp!=NO_OP)
	{
		switch (indexScan->pIXIndexHandle->fileHeader->attrType)
		{
		case chars:
			if(!CmpString(indexScan->currentPageControl->keys+
				indexScan->ridIx*indexScan->pIXIndexHandle->fileHeader->keyLength+sizeof(RID),
				indexScan->value,
				indexScan->compOp))
				return FAIL;
		case ints:
		case floats:
			if(!CmpValue(*(float *)(indexScan->currentPageControl->keys+
				indexScan->ridIx*indexScan->pIXIndexHandle->fileHeader->keyLength+sizeof(RID)),
				*(float *)indexScan->value,
				indexScan->compOp))
				return FAIL;
			break;
		}
	}
	rid->bValid=true;
	rid->pageNum=indexScan->pnNext;
	rid->slotNum=indexScan->ridIx;
	indexScan->ridIx++;
	return SUCCESS;
}

RC CloseIndexScan(IX_IndexScan *indexScan){	
	indexScan->bOpen=false;
	indexScan->pfPageHandle->pFrame->bDirty=false;
    CloseIndex(indexScan->pIXIndexHandle);
	return SUCCESS;
}

RC GetIndexTree(char *fileName, Tree *index)
{

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
		rid,indexHandle->fileHeader.attrType,indexHandle->fileHeader.keyLength);
	if(pageControl->keynum<indexHandle->fileHeader.order)
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
			(indexHandle->fileHeader.order+1)*indexHandle->fileHeader.keyLength);
		//先向兄弟节点搬移分裂出去的索引数据
		broPageControl->keynum=(pageControl->keynum-splitOffset);   //设置兄弟节点的实际索引数
		memcpy(broPageControl->keys,
			pageControl->keys+splitOffset,
			broPageControl->keynum*indexHandle->fileHeader.keyLength); //搬移索引区数据
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
				(indexHandle->fileHeader.order+1)*indexHandle->fileHeader->keyLength);
			indexHandle->fileHeader.rootPage=parentNode->pFrame->page.pageNum;  //设置当前的根节点位置
			memcpy(parentPageControl->keys,pageControl->keys,indexHandle->fileHeader.keyLength);  //当前节点的第一个索引值
			parentPageControl->rids->bValid=true;
			parentPageControl->rids->pageNum=pageInsert->pFrame->page.pageNum;  //一个指针指向当前节点
			parentPageControl->rids->slotNum=0;   //内节点的指针的槽值都为0
			memcpy(parentPageControl->keys,broPageControl->keys,indexHandle->fileHeader.keyLength);  //兄弟节点的第一个索引值
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
				memcpy(parentPage->pFrame->page.pData,pageControl->keys,indexHandle->fileHeader.keyLength);
			RecursionInsert(indexHandle,broPageControl->keys,broPointer,parentPage);
		}
	}
}


RC DeleteEntry(IX_IndexHandle *indexHandle,void *pData,const RID * rid)
{/*    ↑
	   |
	   |
     I'm so lonely, all my brothers have been implemented,
	 but I still remain in a state of prototype.
	 I feel like I was abandoned by Wuzi, so sad o(╥﹏╥)o
 */
	/* Your never code alone ~ */
	PF_PageHandle *pageDelete = FindNode(indexHandle, pData); //根据输入的数据找到即将操作的节点
	//调用递归函数
	return RecursionDelete(indexHandle, pData, rid, pageDelete);
}

//索引删除的递归调用
RC RecursionDelete(IX_IndexHandle *indexHandle, void *pData, const RID *rid, PF_PageHandle *pageDelete)
{
	PF_FileHandle *fileHandle = indexHandle->fileHandle;
	IX_Node *pageControl = (IX_Node *)(pageDelete->pFrame->page.pData + sizeof(IX_FileHeader));			// 获得当前页的索引记录信息
	int offset = deleteKey(pageControl->keys, pageControl->rids, &pageControl->keynum, (char *)pData,
							 indexHandle->fileHeader->attrType, indexHandle->fileHeader->keyLength);	// 删除对应的索引项
	if (-1 == offset) // 如果该键不存在
		return FAIL;
	int threshold = ceil((float)indexHandle->fileHeader->order / 2);
	// 该key存在，并已在叶子结点删除。进行下一步判断:每个内部节点的分支数范围应为[ceil(m/2),m];
	if(pageControl->keynum >= threshold)							// 索引项数符合规定,没有下溢
	{
		if(offset == 0)	// 删除的是页面的第一个结点,需调整父页面的值
		{
			PageNum nodePageNum;
			PF_PageHandle *parentPageHandle = nullptr;
		
			GetPageNum(pageHandle, &nodePageNum);									//本页面的页号
			GetThisPage(fileHandle, pageControl->parent, parentPageHandle);			//本页面的父亲
			deleteOrAlterParentNode(parentPageHandle, fileHandle, indexHandle->fileHeader.order, indexHandle->fileHeader.attrType,
									indexHandle->fileHeader.attrLength, nodePageNum, pageControl->keynum, pageControl->parentOrder, false); 
		}	
	}  
	else	// 下溢
	{
		/*	if(临近兄弟e处于半满状态) 则不能借节点，要将d合并到兄弟e
			*	1 合并
			*		1.1 e是左兄弟就把目前页中内容都放到e最后的节点之后。
			*		1.2 e是右兄弟在把目前页内容插入到e的开头，将e中原有的节点往下移
			*	2 删掉在父节点中指向目前页d的内容 
			*	3 递归向上走
		 *  else 将从e借一个节点加到目前节点, 如果e是左节点则借最大数，是右节点则借最小数
		 */
		getFromBrother(pageDelete, fileHandle, indexHandle->fileHeader.order, indexHandle->fileHeader.attrType, 
						indexHandle->fileHeader.attrLength, threshold);   //对兄弟节点进行处理(函数内部会先后找左右兄弟)
	}
	return SUCCESS;		// 返回成功,不用调整父页面的值
}

//从兄弟节点中借节点或者合并
void getFromBrother(PF_PageHandle *pageHandle, PF_FileHandle *fileHandle,const int order,const AttrType attrType,const int attrLength,const int threshold)
{
	int status = 0;
	PageNum leftPageNum;
	PageNum nodePageNum;
	findLeftBrother(pageHandle, fileHandle, order, attrType, attrLength, leftPageNum);    //首先从左兄弟节点处理
	char *tempData = nullptr;
	char *tempKeys = nullptr;
	IX_Node* tempNodeControlInfo = nullptr;
	PF_PageHandle *parentPageHandle = nullptr;

	if (-1 != leftPageNum)   //如果左兄弟节点存在，对左兄弟进行处理
	{
		PF_PageHandle *leftHandle = nullptr;
		GetThisPage(fileHandle, leftPageNum, leftHandle);
		getFromLeft(pageHandle, leftHandle, order, attrType, attrLength, threshold, status);   //对左兄弟进行处理
		
		if (1 == status)		//情况1:从左兄弟借点.处理:修改本节点父亲页的值
		{
			GetPageNum(pageHandle, &nodePageNum);									//本页面的页号		
			GetData(pageHandle, &tempData);
			tempNodeControlInfo = (IX_Node*)(tempData + sizeof(IX_FileHeader));		//指向本页面节点
			tempKeys = tempData + sizeof(IX_FileHeader) + sizeof(IX_Node);			//本页面的关键字区
			GetThisPage(fileHandle, tempNodeControlInfo->parent, parentPageHandle);	//本页面的父亲
			
			deleteOrAlterParentNode(parentPageHandle, fileHandle, order, attrType, attrLength, nodePageNum, tempKeys, tempNodeControlInfo->parentOrder, false);
		}
		else if (2 == status)   //情况2:与左节点进行合并.处理:删除左兄弟的父亲页对应的关键字
		{
			// 拿到左兄弟父亲的关键值
			GetData(leftHandle, &tempData);
			tempNodeControlInfo = (IX_Node*)(tempData + sizeof(IX_FileHeader));		//指向左兄弟节点
			//tempKeys = tempData + sizeof(IX_FileHeader) + sizeof(IX_Node);			//左兄弟的关键字区
			GetThisPage(fileHandle, tempNodeControlInfo->parent, parentPageHandle);	//左兄弟父亲
			//进行删除
			deleteOrAlterParentNode(parentPageHandle, fileHandle, order, attrType, attrLength, leftPageNum, nullptr, tempNodeControlInfo->parentOrder, true);   
		}
	}
	else   //左兄弟节点不存在，对右兄弟进行处理
	{
		PF_PageHandle *rightHandle = nullptr;
		GetData(pageHandle, &tempData);
		tempNodeControlInfo = (IX_Node*)(tempData + sizeof(IX_FileHeader));		//指向本节点
		GetThisPage(fileHandle, tempNodeControlInfo->brother, rightHandle);
		getFromRight(pageHandle, rightHandle, order, attrType, attrLength, threshold, status);  //对右兄弟进行处理

		// 拿到右兄弟父亲的关键值
		GetData(rightHandle, &tempData);
		tempNodeControlInfo = (IX_Node*)(tempData + sizeof(IX_FileHeader));		//指向右兄弟节点
		tempKeys = tempData + sizeof(IX_FileHeader) + sizeof(IX_Node);	//右兄弟的关键字区
		GetThisPage(fileHandle, tempNodeControlInfo->parent, parentPageHandle);	//右兄弟父亲
		GetPageNum(rightHandle, &nodePageNum);	// 右兄弟的页号
		
		if (3 == status)		//情况3:从右兄弟借点.处理:修改右兄弟的父亲页的值
		{
			deleteOrAlterParentNode(parentPageHandle, &fileHandle, order, attrType, attrLength, nodePageNum, tempKeys, tempNodeControlInfo->parentOrder, false);  //递归修改右兄弟节点
		}
		else if (4 == status)	//情况4:将右兄弟合并到本节点.处理:删除右兄弟的父亲页对应的关键字
		{
			deleteOrAlterParentNode(parentPageHandle, &fileHandle, order, attrType, attrLength, nodePageNum, nullptr, tempNodeControlInfo->parentOrder, true);    //从父节点中删除右节点对应的关键字
		}
	}

}

//与右兄弟节点进行处理
void getFromRight(PF_PageHandle *pageHandle, PF_PageHandle *rightHandle, int order, AttrType attrType, int attrLength, const int threshold, int &status)
{
	char *pageData;
	char *pageKeys;
	char *pageRids;

	char *rightData;
	char *rightKeys;
	char *rightRids;

	GetData(pageHandle, &pageData);
	IX_Node* pageNodeControlInfo = (IX_Node*)(pageData + sizeof(IX_FileHeader));
	int pageKeynum = pageNodeControlInfo->keynum;
	pageKeys = pageData + sizeof(IX_FileHeader) + sizeof(IX_Node);	//获取关键字区
	pageRids = pageKeys + order*attrLength;							//获取指针区

	GetData(rightHandle, &rightData);
	//获取叶节点页面得节点控制信息
	IX_Node* rightNodeControlInfo = (IX_Node*)(rightData + sizeof(IX_FileHeader));
	rightKeys = rightData + sizeof(IX_FileHeader) + sizeof(IX_Node);//获取关键字区
	rightRids = rightKeys + order*attrLength;						//获取指针区

	int rightKeynum = rightNodeControlInfo->keynum;
	if (rightKeynum > threshold)   //可以借出去
	{
		memcpy(pageKeys + pageKeynum * attrLength, rightKeys, attrLength);  //复制右节点的第一个关键字
		memcpy(pageRids + pageKeynum * sizeof(RID), rightRids, sizeof(RID));  //复制右节点的第一个关键字指针

		memcpy(rightKeys, rightKeys + attrLength, (rightKeynum - 1) * attrLength);   //关键字整体前移一个位置
		memcpy(rightRids, rightRids + sizeof(RID), (rightKeynum - 1) * sizeof(RID));   //关键字指针整体前移一个位置

		rightNodeControlInfo->keynum = rightKeynum -1;    //修改关键字个数
		pageNodeControlInfo->keynum = pageKeynum + 1;   //修改关键字个数
		status = 3;										//情况3:从右兄弟借点，后续需要替换父节点对应的关键词
	}
	else   //不能借，进行合并
	{
		memcpy(pageKeys + pageKeynum*attrLength, rightKeys, rightKeynum*attrLength);  //复制右节点的所有关键字
		memcpy(pageRids + pageKeynum * sizeof(RID), rightRids, rightKeynum * sizeof(RID));  //复制右节点的所有关键字指针

		rightNodeControlInfo->keynum = 0;							//修改关键字个数
		pageNodeControlInfo->keynum = pageKeynum + rightKeynum;		//修改关键字个数
		status = 4;													//情况4:将右兄弟合并到本节点，后续需要删除父节点对应的右兄弟关键词
 
		pageNodeControlInfo->brother = rightNodeControlInfo->brother;   //修改页面链表指针
	}
}

//找出当前节点的左兄弟节点
void findLeftBrother(PF_PageHandle *pageHandle, PF_FileHandle *fileHandle, const int order, const AttrType attrType, const int attrLength, PageNum &leftBrother)
{
	char *data;
	PageNum nowPage;
	GetPageNum(pageHandle, &nowPage);   //获取当前页面号
	GetData(pageHandle, &data);
	IX_Node* nodeControlInfo = (IX_Node*)(data + sizeof(IX_FileHeader));

	PF_PageHandle *parentPageHandle = NULL;
	GetThisPage(fileHandle, nodeControlInfo->parent, parentPageHandle);
	char *parentData;
	char *parentKeys;
	char *parentRids;

	GetData(parentPageHandle, &parentData);
	//获取关键字区
	parentKeys = parentData + sizeof(IX_FileHeader) + sizeof(IX_Node);
	//获取指针区
	parentRids = parentKeys + order * attrLength;
	for (int offset = 0; ; offset++)
	{
		RID *tempRid = (RID*)parentRids + offset * sizeof(RID);
		if (tempRid->pageNum == nowPage)
		{
			if (offset != 0)			// 如果是第1个则没有左兄弟
			{
				offset--;			// 往左移一个单位
				tempRid = (RID*)parentRids + offset * sizeof(RID);
				leftBrother = tempRid->pageNum;
			}
			else
				leftBrother = -1;
			return;
		}
	}
}

//与左兄弟节点进行处理
void getFromLeft(PF_PageHandle *pageHandle, PF_PageHandle *leftHandle, int order, AttrType attrType, int attrLength, const int threshold, int &status)
{
	char *pageData;
	char *pageKeys;
	char *pageRids;

	char *leftData;
	char *leftKeys;
	char *leftRids;

	GetData(leftHandle, &leftData);
	//获取左节点页面得节点控制信息
	IX_Node* leftNodeControlInfo = (IX_Node*)(leftData + sizeof(IX_FileHeader));
	//获取关键字区
	leftKeys = leftData + sizeof(IX_FileHeader) + sizeof(IX_Node);
	//获取指针区
	leftRids = leftKeys + order*attrLength;

	GetData(pageHandle, &pageData);
	IX_Node* pageNodeControlInfo = (IX_Node*)(pageData + sizeof(IX_FileHeader));
	int pageKeynum = pageNodeControlInfo->keynum;
	//获取关键字区
	pageKeys = pageData + sizeof(IX_FileHeader) + sizeof(IX_Node);
	//获取指针区
	pageRids = pageKeys + order*attrLength;

	int leftKeynum = leftNodeControlInfo->keynum;
	if (leftKeynum > threshold)   //说明可以借出去
	{
		// 本页面的关键字向后移一个单位，将左兄弟的最后一个关键字复制到本页面的第一个位置
		memcpy(pageKeys + attrLength, pageKeys, pageKeynum * attrLength);   //关键字整体后移
		memcpy(pageRids + sizeof(RID), pageRids, pageKeynum * sizeof(RID));   //关键字指针整体后移

		memcpy(pageKeys, leftKeys + (leftKeynum - 1) * attrLength, attrLength);  //复制左节点的最后一个关键字
		memcpy(pageRids, leftRids + (leftKeynum - 1) * sizeof(RID), sizeof(RID));  //复制左节点最后一个关键字指针

		leftNodeControlInfo->keynum = leftKeynum - 1;    //修改关键字个数
		pageNodeControlInfo->keynum = pageKeynum + 1;   //修改关键字个数
		status = 1;		// 第一种情况：从左兄弟借一个节点， 后续需要改指向本节点的父节点中的关键值。用changeParentsFirstKey函数

	}
	else   //说明不能借，只能进行合并：合并时将本页内容加到左兄弟的最后一个节点后
	{
		memcpy(leftKeys + leftKeynum * attrLength, pageKeys, pageKeynum * attrLength);   //关键字整体复制到左节点中
		memcpy(leftRids + leftKeynum * sizeof(RID), pageRids, pageKeynum * sizeof(RID));   //关键字指针整体复制到左节点中

		leftNodeControlInfo->keynum = leftKeynum + pageKeynum;    //修改关键字个数
		pageNodeControlInfo->keynum = 0;   //修改关键字个数
		leftNodeControlInfo->brother = pageNodeControlInfo->brother;    //修改叶子页面链表指针
		status = 2;		// 第二种情况：将节点复制到左兄弟节点后面
	}

}

// 以迭代的方式删除或修改父节点的节点值
void deleteOrAlterParentNode(PF_PageHandle *parentPageHandle, PF_FileHandle *fileHandle, int order, AttrType attrType, int attrLength, PageNum nodePageNum, void *pData, int parentOrder, bool isDelete)
{
	IX_Node *nodeControlInfo;
	char *parentData;
	char *parentKeys;
	char *parentRids;
	int offset = parentOrder;	
	bool rootFlag = true;		//因循环判断条件不包含根节点，用一个标志来进行根的key覆盖
	//indexHandle->fileHeader->rootPage != node->parent	// 循环至根的子结点
	while(true) 
	{
		GetData(parentPageHandle, &parentData);
		nodeControlInfo = (IX_Node*)(parentData + sizeof(IX_FileHeader));
		int keynum = nodeControlInfo->keynum;								//获取父亲关键字数目
		parentKeys = parentData + sizeof(IX_FileHeader) + sizeof(IX_Node);	//获取父亲关键字区
		parentRids = parentKeys + order * attrLength;						//获取父亲指针区
											
		if (isDelete)
		{
			//对关键字和指针进行覆盖删除
			memcpy(parentKeys + offset * attrLength, parentKeys + (offset + 1) * attrLength, (keynum - offset - 1) * attrLength);
			memcpy(parentRids + offset * sizeof(RID), parentRids + (offset + 1) * sizeof(RID), (keynum - offset - 1) * sizeof(RID));
			nodeControlInfo->keynum = keynum - 1;
			return ;
		}
		else
		{
			//修改关键字
			memcpy(parentKeys + offset * attrLength, pData, attrLength);
			if (offset == 0 && nodeControlInfo->parent != 0)   //说明修改的关键字为第一个，需要递归地进行修改. 根节点页号为0（需确认）
			{
				GetPageNum(parentPageHandle, &nodePageNum);
				GetThisPage(fileHandle, nodeControlInfo->parent, parentPageHandle);   //递归地进行修改
			}
			else
				return ;
		}
		offset = nodeControlInfo->parentOrder;										// 记住父节点对应的节点序号
	}
	/*
		if (rootFlag && (0 == node->parentOrder))
		{
			GetThisPage(indexHandle->fileHandle, node->parent, parentPage);
			GetData(parentPage, &parentData);
			parentNode = (IX_Node*)(parentData + sizeof(IX_FileHeader));	
			// 获取关键字区
			parentKeys = parentData + sizeof(IX_FileHeader) + sizeof(IX_Node);
			// 对父节点关键字进行覆盖
		}
	*/
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
	//减一是为了留出一个位置使得每个节点存储的关键字数可以暂时超过限制1个
	fileHeader->order = (PF_PAGE_SIZE-sizeof(IX_FileHeader)-sizeof(IX_Node))/(2*sizeof(RID)+attrLength)-1;
	fileHeader->rootPage = 1;				
	// 在<索引控制信息>后添加<节点控制信息>
	IX_Node *ixNode = (IX_Node *)(firstPage->pFrame->page.pData+sizeof(IX_FileHeader));
	ixNode->is_leaf = 1;		// 默认为是叶子结点
	ixNode->keynum = 0;
	ixNode->parent = 0;
	ixNode->parentOrder = 0;
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
	int keyOffset,rtn;
	float newValue,valueInIndex;
	//遍历已有key，找到插入位置

	for (keyOffset=0;keyOffset<(*effectiveLength);keyOffset++)
	{
		switch(type)
		{
		case 0://字符串的比较
			rtn=strcmp(keyInsert+sizeof(RID),key+keyOffset*attrLength+sizeof(RID));
			break;
		case 1:
		case 2: //int以及float的比较
			newValue=*((float *)keyInsert+sizeof(RID));
			valueInIndex=*((float *)(key+keyOffset*attrLength+sizeof(RID)));
			rtn=(newValue<valueInIndex)?-1:((newValue==valueInIndex)?0:1);
			break;
		default:
			break;
		}
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
}

int deleteKey(char *key, RID *val, int *eLength, char *keyDelete, AttrType type, int attrLength){
	int keyOffset;
	switch (type)
	{	
		case chars: //字符串比较
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
							deleteKeyShift(keyOffset,key,val,eLength,attrLength);
							return keyOffset;
						}
						// 如果keyDelete槽号小于目前key的槽号则退出并返回-1
						else if(((RID *)keyDelete)->slotNum < ((RID *)key+keyOffset*attrLength)->slotNum)
							return -1;
						// 如果keyDelete槽号大于目前key的槽号则继续下一个循环
					}
					// 如果keyDelete页号小于目前key的页号则退出并返回-1
					else if(((RID *)keyDelete)->pageNum < ((RID *)key + keyOffset * attrLength)->pageNum)
						return -1;
					// 如果keyDelete页号大于目前key的页号则继续下一个循环
				}
				// 如果要删除的keyDelete大于目前查找的key则进行下一个循环
			}
			break;
		case ints:	//int
		case floats:	//float
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
							deleteKeyShift(keyOffset,key,val,eLength,attrLength);
							return keyOffset;
						}
						// 如果keyDelete槽号小于目前key的槽号则跳出循环
						else if(((RID *)keyDelete)->slotNum < ((RID *)key+keyOffset*attrLength)->slotNum)
							return -1;
						// 如果keyDelete槽号大于目前key的槽号则继续下一个循环
					}
					// 如果keyDelete页号小于目前key的页号则跳出循环
					else if(((RID *)keyDelete)->pageNum < ((RID *)key + keyOffset * attrLength)->pageNum)
						return -1;
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
	//return --(*eLength);

}

PF_PageHandle *FindNode(IX_IndexHandle *indexHandle,void *targetKey)
{
	//定位根节点
	int rootPage=indexHandle->fileHeader.rootPage;
	PF_PageHandle *currentPage;
	int rtn;
	float targetVal,indexVal;
	GetThisPage(indexHandle->fileHandle,rootPage,currentPage);
	IX_Node *nodeInfo;
	nodeInfo=(IX_Node *)(currentPage->pFrame->page.pData[sizeof(IX_FileHeader)]);
	int isLeaf=nodeInfo->is_leaf;
	
	int offset;
	while(isLeaf==0)
	{
		for(offset=0; offset<nodeInfo->keynum;offset++)
		{
			switch(indexHandle->fileHeader.attrType)
			{
			case 0:
				rtn=strcmp((char *)targetKey+sizeof(RID),
					nodeInfo->keys+offset*indexHandle->fileHeader.keyLength+sizeof(RID));
			case 1:
			case 2:
				targetVal=*((float *)targetKey+sizeof(RID));
				indexVal=*(float *)(nodeInfo->keys+offset*indexHandle->fileHeader.keyLength+sizeof(RID));
				rtn=(targetVal<indexVal)?-1:((targetVal==indexVal)?0:1);
				break;
			default:
				break;
			}
			if(rtn>0)
				continue; 
			else if(rtn==0)
			{
				int currentPageNum=((RID *)(nodeInfo->keys+offset*indexHandle->fileHeader.keyLength))->pageNum;
				if(((RID *)targetKey)->pageNum>currentPageNum)
					continue;
				else if(((RID *)targetKey)->pageNum==currentPageNum)
				{
					int currentSlotNum=((RID *)(nodeInfo->keys+offset*indexHandle->fileHeader.keyLength))->slotNum;
					if(((RID *)targetKey)->slotNum>currentSlotNum)
						continue;
					if(((RID *)targetKey)->slotNum)
						offset++;   
				}
			}
			RID child=(RID)nodeInfo->rids[offset==0?0:offset-1];
			GetThisPage(indexHandle->fileHandle,child.pageNum,currentPage);
			nodeInfo=(IX_Node *)(currentPage->pFrame->page.pData[sizeof(IX_FileHeader)]);
			int isLeaf=nodeInfo->is_leaf;
			break;
		}
	}
	return currentPage;
}
