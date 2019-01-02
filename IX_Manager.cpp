#include "stdafx.h"
#include "IX_Manager.h"
#include "RM_Manager.h" //ʹ�ñȽϺ���

/****************
*optimization notepad
*1.��������ķ���ֵ��ʧ�ܵ�������
*2.������������ֵ����
*3.�����������ת����
*4.����������ָ������
****************/

//12/27
RC OpenIndexScan(IX_IndexScan *indexScan,IX_IndexHandle *indexHandle,CompOp compOp,char *value)
{
	//��ʼ����������ֵ
	indexScan->bOpen=true;
	indexScan->compOp=compOp;
	indexScan->pIXIndexHandle=indexHandle;
	indexScan->value=value;
	PF_PageHandle *pageStart;
	//��ʼ��ҳ��š��������š�ҳ����
	switch (compOp) //С�ڡ�С�ڵ�������ڴ���С�������ʼ����
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
	pageStart=FindNode(indexHandle,value);  //�ҵ�������ʼ������ֵ���ڽڵ�
	IX_Node *startPageControl=(IX_Node *)(pageStart->pFrame->page.pData+sizeof(IX_FileHeader));  //��ÿ�ʼҳ��������¼��Ϣ
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
	if(indexOffset==startPageControl->keynum)  //����һ���������Ŀ��ֵ����ĳ�ڵ������ֵ����С�������ֵܽڵ����Сֵ
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

//���Ƚϲ���
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

//ctmd���Ӳ���д��
RC GetIndexTree(char *fileName, Tree *index)
{

	return SUCCESS;
}

//12/24
//ע�⴦����ֵ������
RC InsertEntry(IX_IndexHandle *indexHandle,void *pData,const RID *rid)
{
	PF_PageHandle *pageInsert = FindNode(indexHandle,pData); //��������������ҵ����������Ľڵ�
	//���õݹ麯��
	RecursionInsert(indexHandle,pData,rid,pageInsert);
	return FAIL;
}

//��������ĵݹ����
void RecursionInsert(IX_IndexHandle *indexHandle,void *pData,const RID *rid,PF_PageHandle *pageInsert)
{
	IX_Node *pageControl=(IX_Node *)(pageInsert->pFrame->page.pData+sizeof(IX_FileHeader));  //��õ�ǰҳ��������¼��Ϣ
	int posInsert=insertKey(pageControl->keys,pageControl->rids,&pageControl->keynum,(char *)pData,  //ǿ�Ʋ���һ��������
		rid,indexHandle->fileHeader.attrType,indexHandle->fileHeader.keyLength);
	if(pageControl->keynum<indexHandle->fileHeader.order)
		return;  //��������û�г����Դ�ϲ
	else
	{
		//���������ﵽ����󣬽ڵ����
		int splitOffset=int(pageControl->keynum/2+0.5);          //�ڵ�������¼ȡ�룬����ȡ��
		PF_PageHandle *brotherNode;                              //Ϊ��ǰ�ڵ����һ���ֵܽڵ�
		AllocatePage(indexHandle->fileHandle,brotherNode);
		pageControl->brother=brotherNode->pFrame->page.pageNum;  //����ֵܽڵ��ҳ��
		IX_Node *broPageControl=(IX_Node *)(brotherNode->pFrame->page.pData+sizeof(IX_FileHeader)); //�ֵܽڵ�Ŀ�����Ϣ
		broPageControl->keys=brotherNode->pFrame->page.pData+sizeof(IX_FileHeader)+sizeof(IX_Node); //�����ֵܽڵ����������������
		broPageControl->rids=(RID *)(broPageControl->keys+
			(indexHandle->fileHeader.order+1)*indexHandle->fileHeader.keyLength);
		//�����ֵܽڵ���Ʒ��ѳ�ȥ����������
		broPageControl->keynum=(pageControl->keynum-splitOffset);   //�����ֵܽڵ��ʵ��������
		memcpy(broPageControl->keys,
			pageControl->keys+splitOffset,
			broPageControl->keynum*indexHandle->fileHeader.keyLength); //��������������
		pageControl->keynum=splitOffset;         //���õ�ǰ�ڵ��ʵ��������
		memcpy(broPageControl->rids,
			pageControl->rids+splitOffset,
			broPageControl->keynum*sizeof(RID)); //����ָ������ֵ��������
		broPageControl->is_leaf=pageControl->is_leaf;  //����ֵܽڵ�Ҷ�ӽڵ�����
		broPageControl->brother=-1;    //�ֵܽڵ���ʱû�����ֵܽڵ�
		//����Ƿ��ǵ�ǰ�ĸ��ڵ��ڷ��ѣ��Ƿ��и���㣩
		if(pageControl->parent==0)    //��ǰ�ڵ��Ǹ��ڵ�
		{
			PF_PageHandle *parentNode;
			AllocatePage(indexHandle->fileHandle,parentNode);
			IX_Node *parentPageControl=(IX_Node *)(parentNode->pFrame->page.pData+sizeof(IX_FileHeader));;  //����������Ϣ
			//��ʼ���������Ϣ
			parentPageControl->keynum=2; //ֻ������������Ŀ
			parentPageControl->is_leaf=0;
			parentPageControl->parent=-1;
			parentPageControl->brother=-1;
			parentPageControl->keys=parentNode->pFrame->page.pData+sizeof(IX_FileHeader)+sizeof(IX_Node); //���㸸�ڵ����������������
			parentPageControl->rids=(RID *)(parentPageControl->keys+
				(indexHandle->fileHeader.order+1)*indexHandle->fileHeader->keyLength);
			indexHandle->fileHeader.rootPage=parentNode->pFrame->page.pageNum;  //���õ�ǰ�ĸ��ڵ�λ��
			memcpy(parentPageControl->keys,pageControl->keys,indexHandle->fileHeader.keyLength);  //��ǰ�ڵ�ĵ�һ������ֵ
			parentPageControl->rids->bValid=true;
			parentPageControl->rids->pageNum=pageInsert->pFrame->page.pageNum;  //һ��ָ��ָ��ǰ�ڵ�
			parentPageControl->rids->slotNum=0;   //�ڽڵ��ָ��Ĳ�ֵ��Ϊ0
			memcpy(parentPageControl->keys,broPageControl->keys,indexHandle->fileHeader.keyLength);  //�ֵܽڵ�ĵ�һ������ֵ
			parentPageControl->rids->bValid=true;
			parentPageControl->rids->pageNum=brotherNode->pFrame->page.pageNum;  //һ��ָ��ָ���ֵܽڵ�
			parentPageControl->rids->slotNum=0;   //�ڽڵ��ָ��Ĳ�ֵ��Ϊ0
			pageControl->parent=parentNode->pFrame->page.pageNum;  //��ǰ�ڵ�ָ�򸸽��
			broPageControl->parent=parentNode->pFrame->page.pageNum;  //�ֵܽڵ�ָ�򸸽��
			return;
		}
		else
		{
			broPageControl->parent=pageControl->parent;
			//�ݹ���ò��븸���
			RID *broPointer;
			broPointer->bValid=true;
			broPointer->pageNum=brotherNode->pFrame->page.pageNum;
			broPointer->slotNum=0;
			PF_PageHandle *parentPage;
			GetThisPage(indexHandle->fileHandle,pageControl->parent,parentPage);
			if(posInsert!=0)  //ǰ������ʱ������˵�ǰ�ڵ������࣬��Ҫ���¸��ڵ������ֵ
				memcpy(parentPage->pFrame->page.pData,pageControl->keys,indexHandle->fileHeader.keyLength);
			RecursionInsert(indexHandle,broPageControl->keys,broPointer,parentPage);
		}
	}
}

/*******
*��ʣ��������!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
*/
RC DeleteEntry(IX_IndexHandle *indexHandle,void *pData,const RID * rid)
{/*    ��
	   |
	   |
     I'm so lonely, all my brothers have been implemented,
	 but I still remain in a state of prototype.
	 I feel like I was abandoned by Wuzi, so sad o(�i�n�i)o
 */
	/* Your never code alone ~ */
	pageNum pageNum = FindNode(indexHandle, pData); //��������������ҵ����������Ľڵ�
	PF_PageHandle *pageDelete = new pageDelete;
	GetThisPage(indexHandle->fileHandle, pageNum, pageDelete);
	//���õݹ麯��
	RC rtn = RecursionDelete(indexHandle, pData, rid, pageDelete);
	free(pageDelete);
	return rtn;
}

//����ɾ���ĵݹ����
RC RecursionDelete(IX_IndexHandle *indexHandle, void *pData, const RID *rid, PF_PageHandle *pageDelete)
{
	PF_FileHandle *fileHandle = indexHandle->fileHandle;
	IX_Node *pageControl = (IX_Node *)(pageDelete->pFrame->page.pData + sizeof(IX_FileHeader));			// ��õ�ǰҳ��������¼��Ϣ
	int offset = deleteKey(pageControl->keys, pageControl->rids, &pageControl->keynum, (char *)pData,
						indexHandle->fileHeader->attrType, indexHandle->fileHeader->keyLength);	// ɾ����Ӧ��������
	if (-1 == offset) // ����ü�������
		return FAIL;
	int threshold = ceil((float)indexHandle->fileHeader->order / 2);
	// ��key���ڣ�������Ҷ�ӽ��ɾ����������һ���ж�:ÿ���ڲ��ڵ�ķ�֧����ΧӦΪ[ceil(m/2),m];
	if(pageControl->keynum >= threshold)							// �����������Ϲ涨,û������
	{
		if(offset == 0)	// ɾ������ҳ��ĵ�һ�����,�������ҳ���ֵ
		{
			PageNum nodePageNum;
			PF_PageHandle *parentPageHandle = new PF_PageHandle;
		
			GetPageNum(pageHandle, &nodePageNum);									//��ҳ���ҳ��
			GetThisPage(fileHandle, pageControl->parent, parentPageHandle);			//��ҳ��ĸ���
			deleteOrAlterParentNode(parentPageHandle, fileHandle, indexHandle->fileHeader.order, indexHandle->fileHeader.attrType,
							indexHandle->fileHeader.attrLength, nodePageNum, pageControl->keynum, pageControl->parentOrder, false); 
			free(parentPageHandle);
		}	
	}  
	else	// ����
	{
		/*	if(�ٽ��ֵ�e���ڰ���״̬) ���ܽ�ڵ㣬Ҫ��d�ϲ����ֵ�e
			*	1 �ϲ�
			*		1.1 e�����ֵܾͰ�Ŀǰҳ�����ݶ��ŵ�e���Ľڵ�֮��
			*		1.2 e�����ֵ��ڰ�Ŀǰҳ���ݲ��뵽e�Ŀ�ͷ����e��ԭ�еĽڵ�������
			*	2 ɾ���ڸ��ڵ���ָ��Ŀǰҳd������ 
			*	3 �ݹ�������
		 *  else ����e��һ���ڵ�ӵ�Ŀǰ�ڵ�, ���e����ڵ��������������ҽڵ������С��
		 */
		getFromBrother(pageDelete, fileHandle, indexHandle->fileHeader.order, indexHandle->fileHeader.attrType, 
						indexHandle->fileHeader.attrLength, threshold);   //���ֵܽڵ���д���(�����ڲ����Ⱥ��������ֵ�)
	}
	return SUCCESS;		// ���سɹ�,���õ�����ҳ���ֵ
}

//���ֵܽڵ��н�ڵ���ߺϲ�
void getFromBrother(PF_PageHandle *pageHandle, PF_FileHandle *fileHandle,const int order,const AttrType attrType,const int attrLength,const int threshold)
{
	int status = 0;
	PageNum leftPageNum;
	PageNum nodePageNum;
	findLeftBrother(pageHandle, fileHandle, order, attrType, attrLength, leftPageNum);    //���ȴ����ֵܽڵ㴦��
	char *tempData = nullptr;
	char *tempKeys = nullptr;
	IX_Node* tempNodeControlInfo = nullptr;
	PF_PageHandle *parentPageHandle =  = new PF_PageHandle;

	if (-1 != leftPageNum)   //������ֵܽڵ���ڣ������ֵܽ��д���
	{
		PF_PageHandle *leftHandle =  = new PF_PageHandle;
		GetThisPage(fileHandle, leftPageNum, leftHandle);
		getFromLeft(pageHandle, leftHandle, order, attrType, attrLength, threshold, status);   //�����ֵܽ��д���
		
		if (1 == status)		//���1:�����ֵܽ��.����:�޸ı��ڵ㸸��ҳ��ֵ
		{
			GetPageNum(pageHandle, &nodePageNum);									//��ҳ���ҳ��		
			GetData(pageHandle, &tempData);
			tempNodeControlInfo = (IX_Node*)(tempData + sizeof(IX_FileHeader));		//ָ��ҳ��ڵ�
			tempKeys = tempData + sizeof(IX_FileHeader) + sizeof(IX_Node);			//��ҳ��Ĺؼ�����
			GetThisPage(fileHandle, tempNodeControlInfo->parent, parentPageHandle);	//��ҳ��ĸ���
			
			deleteOrAlterParentNode(parentPageHandle, fileHandle, order, attrType, attrLength, nodePageNum, tempKeys, tempNodeControlInfo->parentOrder, false);
		}
		else if (2 == status)   //���2:����ڵ���кϲ�.����:ɾ�����ֵܵĸ���ҳ��Ӧ�Ĺؼ���
		{
			// �õ����ֵܸ��׵Ĺؼ�ֵ
			GetData(leftHandle, &tempData);
			tempNodeControlInfo = (IX_Node*)(tempData + sizeof(IX_FileHeader));		//ָ�����ֵܽڵ�
			//tempKeys = tempData + sizeof(IX_FileHeader) + sizeof(IX_Node);			//���ֵܵĹؼ�����
			GetThisPage(fileHandle, tempNodeControlInfo->parent, parentPageHandle);	//���ֵܸ���
			//����ɾ��
			deleteOrAlterParentNode(parentPageHandle, fileHandle, order, attrType, attrLength, leftPageNum, nullptr, tempNodeControlInfo->parentOrder, true);   
		}
		free(leftHandle);
	}
	else   //���ֵܽڵ㲻���ڣ������ֵܽ��д���
	{
		PF_PageHandle *rightHandle =  = new PF_PageHandle;
		GetData(pageHandle, &tempData);
		tempNodeControlInfo = (IX_Node*)(tempData + sizeof(IX_FileHeader));		//ָ�򱾽ڵ�
		GetThisPage(fileHandle, tempNodeControlInfo->brother, rightHandle);
		getFromRight(pageHandle, rightHandle, order, attrType, attrLength, threshold, status);  //�����ֵܽ��д���

		// �õ����ֵܸ��׵Ĺؼ�ֵ
		GetData(rightHandle, &tempData);
		tempNodeControlInfo = (IX_Node*)(tempData + sizeof(IX_FileHeader));		//ָ�����ֵܽڵ�
		tempKeys = tempData + sizeof(IX_FileHeader) + sizeof(IX_Node);	//���ֵܵĹؼ�����
		GetThisPage(fileHandle, tempNodeControlInfo->parent, parentPageHandle);	//���ֵܸ���
		GetPageNum(rightHandle, &nodePageNum);	// ���ֵܵ�ҳ��
		
		if (3 == status)		//���3:�����ֵܽ��.����:�޸����ֵܵĸ���ҳ��ֵ
		{
			deleteOrAlterParentNode(parentPageHandle, &fileHandle, order, attrType, attrLength, nodePageNum, tempKeys, tempNodeControlInfo->parentOrder, false);  //�ݹ��޸����ֵܽڵ�
		}
		else if (4 == status)	//���4:�����ֵܺϲ������ڵ�.����:ɾ�����ֵܵĸ���ҳ��Ӧ�Ĺؼ���
		{
			deleteOrAlterParentNode(parentPageHandle, &fileHandle, order, attrType, attrLength, nodePageNum, nullptr, tempNodeControlInfo->parentOrder, true);    //�Ӹ��ڵ���ɾ���ҽڵ��Ӧ�Ĺؼ���
		}
		free(rightHandle);
	}
	free(parentPageHandle);
}

//�����ֵܽڵ���д���
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
	pageKeys = pageData + sizeof(IX_FileHeader) + sizeof(IX_Node);	//��ȡ�ؼ�����
	pageRids = pageKeys + order * attrLength;							//��ȡָ����

	GetData(rightHandle, &rightData);
	//��ȡҶ�ڵ�ҳ��ýڵ������Ϣ
	IX_Node* rightNodeControlInfo = (IX_Node*)(rightData + sizeof(IX_FileHeader));
	rightKeys = rightData + sizeof(IX_FileHeader) + sizeof(IX_Node);//��ȡ�ؼ�����
	rightRids = rightKeys + order * attrLength;						//��ȡָ����

	int rightKeynum = rightNodeControlInfo->keynum;
	if (rightKeynum > threshold)   //���Խ��ȥ
	{
		memcpy(pageKeys + pageKeynum * attrLength, rightKeys, attrLength);  //�����ҽڵ�ĵ�һ���ؼ���
		memcpy(pageRids + pageKeynum * sizeof(RID), rightRids, sizeof(RID));  //�����ҽڵ�ĵ�һ���ؼ���ָ��

		memcpy(rightKeys, rightKeys + attrLength, (rightKeynum - 1) * attrLength);   //�ؼ�������ǰ��һ��λ��
		memcpy(rightRids, rightRids + sizeof(RID), (rightKeynum - 1) * sizeof(RID));   //�ؼ���ָ������ǰ��һ��λ��

		rightNodeControlInfo->keynum = rightKeynum -1;    //�޸Ĺؼ��ָ���
		pageNodeControlInfo->keynum = pageKeynum + 1;   //�޸Ĺؼ��ָ���
		status = 3;										//���3:�����ֵܽ�㣬������Ҫ�滻���ڵ��Ӧ�Ĺؼ���
	}
	else   //���ܽ裬���кϲ�
	{
		memcpy(pageKeys + pageKeynum*attrLength, rightKeys, rightKeynum*attrLength);  //�����ҽڵ�����йؼ���
		memcpy(pageRids + pageKeynum * sizeof(RID), rightRids, rightKeynum * sizeof(RID));  //�����ҽڵ�����йؼ���ָ��

		rightNodeControlInfo->keynum = 0;							//�޸Ĺؼ��ָ���
		pageNodeControlInfo->keynum = pageKeynum + rightKeynum;		//�޸Ĺؼ��ָ���
		status = 4;													//���4:�����ֵܺϲ������ڵ㣬������Ҫɾ�����ڵ��Ӧ�����ֵܹؼ���
 
		pageNodeControlInfo->brother = rightNodeControlInfo->brother;   //�޸�ҳ������ָ��
	}
	MarkDirty(PageHandle);
	UnpinPage(PageHandle);
	MarkDirty(leftHandle);
	UnpinPage(leftHandle);
}

//�ҳ���ǰ�ڵ�����ֵܽڵ�
void findLeftBrother(PF_PageHandle *pageHandle, PF_FileHandle *fileHandle, const int order, const AttrType attrType, const int attrLength, PageNum &leftBrother)
{
	char *data;
	PageNum nowPage;
	GetPageNum(pageHandle, &nowPage);   //��ȡ��ǰҳ���
	GetData(pageHandle, &data);
	IX_Node* nodeControlInfo = (IX_Node*)(data + sizeof(IX_FileHeader));

	PF_PageHandle *parentPageHandle = new PF_PageHandle;
	GetThisPage(fileHandle, nodeControlInfo->parent, parentPageHandle);
	char *parentData;
	char *parentKeys;
	char *parentRids;

	GetData(parentPageHandle, &parentData);
	//��ȡ�ؼ�����
	parentKeys = parentData + sizeof(IX_FileHeader) + sizeof(IX_Node);
	//��ȡָ����
	parentRids = parentKeys + order * attrLength;
	for (int offset = 0; ; offset++)
	{
		RID *tempRid = (RID*)parentRids + offset * sizeof(RID);
		if (tempRid->pageNum == nowPage)
		{
			if (offset != 0)			// ����ǵ�1����û�����ֵ�
			{
				offset--;			// ������һ����λ
				tempRid = (RID*)parentRids + offset * sizeof(RID);
				leftBrother = tempRid->pageNum;
			}
			else
				leftBrother = -1;
			return;
		}
	}
	free(parentPageHandle);
}

//�����ֵܽڵ���д���
void getFromLeft(PF_PageHandle *pageHandle, PF_PageHandle *leftHandle, int order, AttrType attrType, int attrLength, const int threshold, int &status)
{
	char *pageData;
	char *pageKeys;
	char *pageRids;

	char *leftData;
	char *leftKeys;
	char *leftRids;

	GetData(leftHandle, &leftData);
	//��ȡ��ڵ�ҳ��ýڵ������Ϣ
	IX_Node* leftNodeControlInfo = (IX_Node*)(leftData + sizeof(IX_FileHeader));
	//��ȡ�ؼ�����
	leftKeys = leftData + sizeof(IX_FileHeader) + sizeof(IX_Node);
	//��ȡָ����
	leftRids = leftKeys + order*attrLength;

	GetData(pageHandle, &pageData);
	IX_Node* pageNodeControlInfo = (IX_Node*)(pageData + sizeof(IX_FileHeader));
	int pageKeynum = pageNodeControlInfo->keynum;
	//��ȡ�ؼ�����
	pageKeys = pageData + sizeof(IX_FileHeader) + sizeof(IX_Node);
	//��ȡָ����
	pageRids = pageKeys + order*attrLength;

	int leftKeynum = leftNodeControlInfo->keynum;
	if (leftKeynum > threshold)   //˵�����Խ��ȥ
	{
		// ��ҳ��Ĺؼ��������һ����λ�������ֵܵ����һ���ؼ��ָ��Ƶ���ҳ��ĵ�һ��λ��
		memcpy(pageKeys + attrLength, pageKeys, pageKeynum * attrLength);   //�ؼ����������
		memcpy(pageRids + sizeof(RID), pageRids, pageKeynum * sizeof(RID));   //�ؼ���ָ���������

		memcpy(pageKeys, leftKeys + (leftKeynum - 1) * attrLength, attrLength);  //������ڵ�����һ���ؼ���
		memcpy(pageRids, leftRids + (leftKeynum - 1) * sizeof(RID), sizeof(RID));  //������ڵ����һ���ؼ���ָ��

		leftNodeControlInfo->keynum = leftKeynum - 1;    //�޸Ĺؼ��ָ���
		pageNodeControlInfo->keynum = pageKeynum + 1;   //�޸Ĺؼ��ָ���
		status = 1;		// ��һ������������ֵܽ�һ���ڵ㣬 ������Ҫ��ָ�򱾽ڵ�ĸ��ڵ��еĹؼ�ֵ����changeParentsFirstKey����

	}
	else   //˵�����ܽ裬ֻ�ܽ��кϲ����ϲ�ʱ����ҳ���ݼӵ����ֵܵ����һ���ڵ��
	{
		memcpy(leftKeys + leftKeynum * attrLength, pageKeys, pageKeynum * attrLength);   //�ؼ������帴�Ƶ���ڵ���
		memcpy(leftRids + leftKeynum * sizeof(RID), pageRids, pageKeynum * sizeof(RID));   //�ؼ���ָ�����帴�Ƶ���ڵ���

		leftNodeControlInfo->keynum = leftKeynum + pageKeynum;    //�޸Ĺؼ��ָ���
		pageNodeControlInfo->keynum = 0;   //�޸Ĺؼ��ָ���
		leftNodeControlInfo->brother = pageNodeControlInfo->brother;    //�޸�Ҷ��ҳ������ָ��
		status = 2;		// �ڶ�����������ڵ㸴�Ƶ����ֵܽڵ����
	}
	MarkDirty(PageHandle);
	UnpinPage(PageHandle);
	MarkDirty(leftHandle);
	UnpinPage(leftHandle);
}

// �Ե����ķ�ʽɾ�����޸ĸ��ڵ�Ľڵ�ֵ
void deleteOrAlterParentNode(PF_PageHandle *parentPageHandle, PF_FileHandle *fileHandle, int order, AttrType attrType, int attrLength, PageNum nodePageNum, void *pData, int parentOrder, bool isDelete)
{
	IX_Node *nodeControlInfo;
	char *parentData;
	char *parentKeys;
	char *parentRids;
	int offset = parentOrder;	
	bool rootFlag = true;		//��ѭ���ж��������������ڵ㣬��һ����־�����и���key����
	//indexHandle->fileHeader->rootPage != node->parent	// ѭ���������ӽ��
	while(true) 
	{
		GetData(parentPageHandle, &parentData);
		nodeControlInfo = (IX_Node*)(parentData + sizeof(IX_FileHeader));
		int keynum = nodeControlInfo->keynum;								//��ȡ���׹ؼ�����Ŀ
		parentKeys = parentData + sizeof(IX_FileHeader) + sizeof(IX_Node);	//��ȡ���׹ؼ�����
		parentRids = parentKeys + order * attrLength;						//��ȡ����ָ����
											
		if (isDelete)
		{
			//�Թؼ��ֺ�ָ����и���ɾ��
			memcpy(parentKeys + offset * attrLength, parentKeys + (offset + 1) * attrLength, (keynum - offset - 1) * attrLength);
			memcpy(parentRids + offset * sizeof(RID), parentRids + (offset + 1) * sizeof(RID), (keynum - offset - 1) * sizeof(RID));
			nodeControlInfo->keynum = keynum - 1;
			break;
		}
		else
		{
			//�޸Ĺؼ���
			memcpy(parentKeys + offset * attrLength, pData, attrLength);
			if (offset == 0 && nodeControlInfo->parent != 0)   //˵���޸ĵĹؼ���Ϊ��һ������Ҫ�ݹ�ؽ����޸�. ���ڵ�ҳ��Ϊ0����ȷ�ϣ�
			{
				MarkDirty(parentPageHandle);		// ���Ϊ��ҳ
				UnpinPage(parentPageHandle);

				GetPageNum(parentPageHandle, &nodePageNum);
				GetThisPage(fileHandle, nodeControlInfo->parent, parentPageHandle);   //�ݹ�ؽ����޸�
			}
			else
				break;
		}
		offset = nodeControlInfo->parentOrder;										// ��ס���ڵ��Ӧ�Ľڵ����
	}
	MarkDirty(parentPageHandle);		// ���Ϊ��ҳ
	UnpinPage(parentPageHandle);
	/*
		if (rootFlag && (0 == node->parentOrder))
		{
			GetThisPage(indexHandle->fileHandle, node->parent, parentPage);
			GetData(parentPage, &parentData);
			parentNode = (IX_Node*)(parentData + sizeof(IX_FileHeader));	
			// ��ȡ�ؼ�����
			parentKeys = parentData + sizeof(IX_FileHeader) + sizeof(IX_Node);
			// �Ը��ڵ�ؼ��ֽ��и���
		}
	*/
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
	//��һ��Ϊ������һ��λ��ʹ��ÿ���ڵ�洢�Ĺؼ�����������ʱ��������1��
	fileHeader->order = (PF_PAGE_SIZE-sizeof(IX_FileHeader)-sizeof(IX_Node))/(2*sizeof(RID)+attrLength)-1;
	fileHeader->rootPage = 1;				
	// ��<����������Ϣ>�����<�ڵ������Ϣ>
	IX_Node *ixNode = (IX_Node *)(firstPage->pFrame->page.pData+sizeof(IX_FileHeader));
	ixNode->is_leaf = 1;		// Ĭ��Ϊ��Ҷ�ӽ��
	ixNode->keynum = 0;
	ixNode->parent = 0;
	ixNode->parentOrder = 0;
	ixNode->brother = -1;
	ixNode->keys = (char *)(firstPage->pFrame->page.pData+sizeof(IX_FileHeader)+sizeof(IX_Node));
	ixNode->rids = (RID *)(ixNode->keys+(fileHeader->order+1)*fileHeader->keyLength);  //+1����Ҫ����Ϊ������һ����λ�Ŀռ�����ƽ��ڵ�ĵ���
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
	if(CloseFile(indexHandle->fileHandle))
		return FAIL;
	indexHandle->bOpen=FALSE;
	return SUCCESS;
}

//attrLength ����RID�ĳ���
int insertKey(char *key, RID *val, int *effectiveLength, char *keyInsert,const RID *valInsert, AttrType type, int attrLength)
{
	int keyOffset,rtn;
	float newValue,valueInIndex;
	//��������key���ҵ�����λ��

	for (keyOffset=0;keyOffset<(*effectiveLength);keyOffset++)
	{
		switch(type)
		{
		case 0://�ַ����ıȽ�
			rtn=strcmp(keyInsert+sizeof(RID),key+keyOffset*attrLength+sizeof(RID));
			break;
		case 1:
		case 2: //int�Լ�float�ıȽ�
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
				//��һ���Ƚ�RID
				if(((RID *)keyInsert)->pageNum==((RID *)key+keyOffset*attrLength)->pageNum)
				{
					if(((RID *)keyInsert)->slotNum==((RID *)key+keyOffset*attrLength)->slotNum)
					{
						//�������key�Ѵ��ڣ�����ֵ��RID)
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
		//������ȵ�ǰ�Աȼ����������һ��ѭ��
	}
}

int deleteKey(char *key, RID *val, int *eLength, char *keyDelete, AttrType type, int attrLength){
	int keyOffset;
	switch (type)
	{	
		case chars: //�ַ����Ƚ�
			for(keyOffset = 0; keyOffset < (*eLength); keyOffset++)
			{
				int rtn = strcmp(keyDelete + sizeof(RID), key + keyOffset*attrLength + sizeof(RID));
				if(rtn < 0) // ���Ҫɾ����keyDeleteС��Ŀǰkey������ѭ��
					break;
				else if(rtn == 0) // �ҵ���Ӧ��key
				{
					//��һ���Ƚ�RID
					if(((RID *)keyDelete)->pageNum == ((RID *)key + keyOffset * attrLength)->pageNum)	//ҳ��
					{
						if(((RID *)keyDelete)->slotNum == ((RID *)key + keyOffset * attrLength)->slotNum) //�ۺ�
						{
							//����ɾ����key
							deleteKeyShift(keyOffset,key,val,eLength,attrLength);
							return keyOffset;
						}
						// ���keyDelete�ۺ�С��Ŀǰkey�Ĳۺ����˳�������-1
						else if(((RID *)keyDelete)->slotNum < ((RID *)key+keyOffset*attrLength)->slotNum)
							return -1;
						// ���keyDelete�ۺŴ���Ŀǰkey�Ĳۺ��������һ��ѭ��
					}
					// ���keyDeleteҳ��С��Ŀǰkey��ҳ�����˳�������-1
					else if(((RID *)keyDelete)->pageNum < ((RID *)key + keyOffset * attrLength)->pageNum)
						return -1;
					// ���keyDeleteҳ�Ŵ���Ŀǰkey��ҳ���������һ��ѭ��
				}
				// ���Ҫɾ����keyDelete����Ŀǰ���ҵ�key�������һ��ѭ��
			}
			break;
		case ints:	//int
		case floats:	//float
			for(keyOffset = 0; keyOffset < (*eLength); keyOffset++)
			{
				int sub = *((float *)keyDelete + sizeof(RID)) - *((float *)(key + keyOffset*attrLength + sizeof(RID)));
				if(sub < 0) // ���Ҫɾ����keyDeleteС��Ŀǰkey������ѭ��
					break;
				else if(sub == 0) // �ҵ���Ӧ��key
				{
					//��һ���Ƚ�RID
					if(((RID *)keyDelete)->pageNum == ((RID *)key + keyOffset * attrLength)->pageNum)	//ҳ��
					{
						if(((RID *)keyDelete)->slotNum == ((RID *)key + keyOffset * attrLength)->slotNum) //�ۺ�
						{
							//����ɾ����key
							deleteKeyShift(keyOffset,key,val,eLength,attrLength);
							return keyOffset;
						}
						// ���keyDelete�ۺ�С��Ŀǰkey�Ĳۺ�������ѭ��
						else if(((RID *)keyDelete)->slotNum < ((RID *)key+keyOffset*attrLength)->slotNum)
							return -1;
						// ���keyDelete�ۺŴ���Ŀǰkey�Ĳۺ��������һ��ѭ��
					}
					// ���keyDeleteҳ��С��Ŀǰkey��ҳ��������ѭ��
					else if(((RID *)keyDelete)->pageNum < ((RID *)key + keyOffset * attrLength)->pageNum)
						return -1;
					// ���keyDeleteҳ�Ŵ���Ŀǰkey��ҳ���������һ��ѭ��
				}
				// ���Ҫɾ����keyDelete����Ŀǰ���ҵ�key�������һ��ѭ��
			}
			break;
		default:
			break;
	}
}

// ��keyShift�����Ѹ���,����ΪinsertKeyShift
int insertKeyShift(int keyOffset, char *key, RID *val, int *effectiveLength, char *keyInsert,const RID *valInsert, int attrLength)
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
	*((RID *)(val+keyOffset*sizeof(RID)))=*valInsert;
	free(valBuffer);
	//��ɼ�ֵ�ԵĲ��룬�����µĽڵ���Ч���ݴ�С
	return ++(*effectiveLength);
}

int deleteKeyShift(int keyOffset, char *key, RID *val, int *eLength, int attrLength){
	// �ؼ��������ƶ�
	char *buffer = (char *)malloc((*eLength - keyOffset - 1) * attrLength);
	memcpy(buffer, key + (keyOffset + 1) * attrLength, (*eLength - keyOffset - 1) * attrLength); // +1 
	memcpy(key + keyOffset * attrLength, buffer, (*eLength - keyOffset - 1) * attrLength);
	free(buffer);

	// ֵ���ƶ�
	RID *valBuffer=(RID *)malloc((*eLength - keyOffset - 1) * sizeof(RID));
	memcpy(buffer, val + (keyOffset + 1) * sizeof(RID), (*eLength - keyOffset - 1) * sizeof(RID)); // +1
	memcpy(val + keyOffset * sizeof(RID), buffer, (*eLength - keyOffset - 1) * sizeof(RID));
	free(valBuffer);

	//��ɼ�ֵ�Ե�ɾ���������µĽڵ���Ч���ݴ�С
	//return --(*eLength);

}

PF_PageHandle *FindNode(IX_IndexHandle *indexHandle,void *targetKey)
{
	//��λ���ڵ�
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
