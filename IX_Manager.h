#ifndef IX_MANAGER_H_H
#define IX_MANAGER_H_H

#include "RM_Manager.h"
#include "PF_Manager.h"

// ����ҳ���
typedef struct{
	int attrLength;			// ��������������ֵ�ĳ���
	int keyLength;			// B+���йؼ��ֵĳ���
	AttrType attrType;		// ��������������ֵ������
	PageNum rootPage;		// B+��������ҳ���
	PageNum first_leaf;		// B+����һ��Ҷ�ӽڵ��ҳ���
	int order;				// ����
}IX_FileHeader;

// �������
typedef struct{
	bool bOpen;					// �Ƿ���һ���ļ�����
	PF_FileHandle fileHandle;	// ��Ӧ��ҳ���ļ����
	IX_FileHeader fileHeader;	// ��Ӧ�Ŀ���ҳ���
}IX_IndexHandle;


typedef struct{
	int is_leaf;		// �ýڵ��Ƿ�ΪҶ�ӽڵ�
	int keynum;			// �����Ĺؼ��ָ���
	PageNum parent;		// ���ڵ�ҳ���
	PageNum brother;	// �ֵܽ��ҳ���
	char *keys;			// �ؼ�������
	RID *rids;			// em?
}IX_Node;

typedef struct{
	bool bOpen;		/*ɨ���Ƿ�� */
	IX_IndexHandle *pIXIndexHandle;	//ָ�������ļ�������ָ��
	CompOp compOp;  /* ���ڱȽϵĲ�����*/
	char *value;		 /* �������бȽϵ�ֵ */
    PF_PageHandle pfPageHandles[PF_BUFFER_SIZE]; // �̶��ڻ�����ҳ������Ӧ��ҳ������б�
	PageNum pnNext; 	//��һ����Ҫ�������ҳ���
}IX_IndexScan;

typedef struct Tree_Node{
	int  keyNum;		//�ڵ��а����Ĺؼ��֣�����ֵ������
	char  **keys;		//�ڵ��а����Ĺؼ��֣�����ֵ������
	Tree_Node  *parent;	//���ڵ�
	Tree_Node  *sibling;	//�ұߵ��ֵܽڵ�
	Tree_Node  *firstChild;	//����ߵĺ��ӽڵ�
}Tree_Node; //�ڵ����ݽṹ

typedef struct{
	AttrType  attrType;	//B+����Ӧ���Ե���������
	int  attrLength;	//B+����Ӧ����ֵ�ĳ���
	int  order;			//B+��������
	Tree_Node  *root;	//B+���ĸ��ڵ�
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

#endif