#ifndef IX_MANAGER_H_H
#define IX_MANAGER_H_H

#include "RM_Manager.h"
#include "PF_Manager.h"
#include <cstdlib>
#include <cmath>

// ����ҳ��
typedef struct{
	int attrLength;			// ��������������ֵ�ĳ���
	int keyLength;			// B+���йؼ��ֵĳ���
	AttrType attrType;		// ��������������ֵ������
	PageNum rootPage;		// B+��������ҳ���
	PageNum first_leaf;		// B+����һ��Ҷ�ӽڵ��ҳ��
	int order;				// ����
}IX_FileHeader;

// �������
typedef struct{
	bool bOpen;					// �Ƿ���һ���ļ���
	PF_FileHandle fileHandle;	// ��Ӧ��ҳ���ļ���
	IX_FileHeader fileHeader;	// ��Ӧ�Ŀ���ҳ���
}IX_IndexHandle;

// 12/28:�޸������ݽṹ,�����parentOrder��leftBrother. ��ԭ����brother��ΪrightBrother
typedef struct{
	int is_leaf;		// �ýڵ��Ƿ�ΪҶ�ӽڵ�
	int keynum;			// �ýڵ�ʵ�ʰ����Ĺؼ��ָ���
	PageNum parent;		// ָ�򸸽ڵ����ڵ�ҳ���
	int parentOrder;	// ���ڵ�������ҳ�浱�е����
	PageNum brother;	// ָ�����ֵܽڵ����ڵ�ҳ���
	char *keys;			// ָ��ؼ�������ָ��
	RID *rids;			// ָ��ָ������ָ��
}IX_Node;

typedef struct{
	bool bOpen;		/*ɨ���Ƿ�� */
	IX_IndexHandle *pIXIndexHandle;	//ָ�������ļ�������ָ
	CompOp compOp;  /* ���ڱȽϵĲ�����*/
	char *value;		 /* �������бȽϵ�*/
    PF_PageHandle *pfPageHandle; // �̶��ڻ�����ҳ������Ӧ��ҳ�������
	PageNum pnNext; 	//��һ����Ҫ�������ҳ���
	int ridIx;
	IX_Node *currentPageControl; //ָ��ǰҳ�������������Ϣ
}IX_IndexScan;

typedef struct Tree_Node{
	int  keyNum;		//�ڵ��а����Ĺؼ��֣�����ֵ������
	char  **keys;		//�ڵ��а����Ĺؼ��֣�����ֵ������
	Tree_Node  *parent;	//����
	Tree_Node  *sibling;	//�ұߵ��ֵܽ�
	Tree_Node  *firstChild;	//����ߵĺ��ӽ�
}Tree_Node; //�ڵ����ݽṹ

typedef struct{
	AttrType  attrType;	//B+����Ӧ���Ե���������
	int  attrLength;	//B+����Ӧ����ֵ�ĳ���
	int  order;			//B+��������
	Tree_Node  *root;	//B+���ĸ���
}Tree;

RC CreateIndex(const char * fileName,AttrType attrType,int attrLength);
RC OpenIndex(const char *fileName,IX_IndexHandle *indexHandle);
RC CloseIndex(IX_IndexHandle *indexHandle);

RC InsertEntry(IX_IndexHandle *indexHandle,void *pData,const RID * rid);
RC DeleteEntry(IX_IndexHandle *indexHandle,void *pData,const RID * rid);
RC OpenIndexScan(IX_IndexScan *indexScan,IX_IndexHandle *indexHandle,CompOp compOp,char *value);
RC IX_GetNextEntry(IX_IndexScan *indexScan,RID * rid);
RC CloseIndexScan(IX_IndexScan *indexScan);
RC GetIndexTree(char *fileName, Tree *index);



int insertKey(char *key, RID *val, int *effectiveLength, char *keyInsert,const RID *valInsert, AttrType type, int attrLength);
int deleteKey(char *key, RID *val, int *eLength, char *keyDelete, AttrType type, int attrLength);

//int KeyShift(int keyOffset,char *key, RID *val, int *effectiveLength, char *keyInsert, RID valInsert, int attrLength);
int FindNode(IX_IndexHandle *indexHandle,void *targetKey);
//���ڽڵ��ֵ�������еĲ�����λ
int insertKeyShift(int keyOffset,char *key, RID *val, int *effectiveLength, char *keyInsert,const RID *valInsert, int attrLength);
//ɾ����λ
int deleteKeyShift(int keyOffset, char *key, RID *val, int *eLength, int attrLength);
void RecursionInsert(IX_IndexHandle *indexHandle,void *pData,const RID *rid,PF_PageHandle *pageInsert);
//����ɾ���ĵݹ����
RC RecursionDelete(IX_IndexHandle *indexHandle, void *pData, const RID *rid, PF_PageHandle *pageDelete);

//�����ֵܽڵ���д���
void getFromRight(PF_PageHandle *pageHandle, PF_PageHandle *rightHandle, int order, AttrType attrType, int attrLength, const int threshold, int &status);
//�ҳ���ǰ�ڵ�����ֵܽڵ�
void findLeftBrother(PF_PageHandle *pageHandle, PF_FileHandle *fileHandle, const int order, const AttrType attrType, const int attrLength, PageNum &leftBrother);
//�����ֵܽڵ���д���
void getFromLeft(PF_PageHandle *pageHandle, PF_PageHandle *leftHandle, int order, AttrType attrType, int attrLength, const int threshold, int &status);
// �Ե����ķ�ʽɾ�����޸ĸ��ڵ�Ľڵ�ֵ
void deleteOrAlterParentNode(PF_PageHandle *parentPageHandle, PF_FileHandle *fileHandle, int order, AttrType attrType, int attrLength, PageNum nodePageNum, void *pData, int parentOrder, bool isDelete);

#endif