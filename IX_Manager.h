#ifndef IX_MANAGER_H_H
#define IX_MANAGER_H_H

#include "RM_Manager.h"
#include "PF_Manager.h"
#include <cstdlib>

// 控制页句柄
typedef struct{
	int attrLength;			// 建立索引的属性值的长度
	int keyLength;			// B+树中关键字的长度
	AttrType attrType;		// 建立索引的属性值的类型
	PageNum rootPage;		// B+树根结点的页面号
	PageNum first_leaf;		// B+树第一个叶子节点的页面号
	int order;				// 序数
}IX_FileHeader;

// 索引句柄
typedef struct{
	bool bOpen;					// 是否与一个文件关联
	PF_FileHandle *fileHandle;	// 对应的页面文件句柄
	IX_FileHeader *fileHeader;	// 对应的控制页句柄
}IX_IndexHandle;


typedef struct{
	int is_leaf;		// 该节点是否为叶子节点
	int keynum;			// 包含的关键字个数
	PageNum parent;		// 父节点页面号
	PageNum brother;	// 兄弟结点页面号
	char *keys;			// 关键字数组
	RID *rids;			// em?
}IX_Node;

typedef struct{
	bool bOpen;		/*扫描是否打开 */
	IX_IndexHandle *pIXIndexHandle;	//指向索引文件操作的指针
	CompOp compOp;  /* 用于比较的操作符*/
	char *value;		 /* 与属性行比较的值 */
    PF_PageHandle pfPageHandles[PF_BUFFER_SIZE]; // 固定在缓冲区页面所对应的页面操作列表
	PageNum pnNext; 	//下一个将要被读入的页面号
}IX_IndexScan;

typedef struct Tree_Node{
	int  keyNum;		//节点中包含的关键字（属性值）个数
	char  **keys;		//节点中包含的关键字（属性值）数组
	Tree_Node  *parent;	//父节点
	Tree_Node  *sibling;	//右边的兄弟节点
	Tree_Node  *firstChild;	//最左边的孩子节点
}Tree_Node; //节点数据结构

typedef struct{
	AttrType  attrType;	//B+树对应属性的数据类型
	int  attrLength;	//B+树对应属性值的长度
	int  order;			//B+树的序数
	Tree_Node  *root;	//B+树的根节点
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
int insertKey(char *key, RID *val, int *effectiveLength, char *keyInsert, RID valInsert, AttrType type, int attrLength);
int deleteKey(char *key, RID *val, int *eLength, char *keyDelete, AttrType type, int attrLength);
//���ڽڵ��ֵ�������еĲ�����λ
int KeyShift(int keyOffset,char *key, RID *val, int *effectiveLength, char *keyInsert, RID valInsert, int attrLength);
PF_PageHandle *FindNode(IX_IndexHandle *indexHandle,char *targetKey);
//用于节点键值对排序中的插入移位
int insertKeyShift(int keyOffset,char *key, RID *val, int *effectiveLength, char *keyInsert, RID valInsert, int attrLength);
//删除移位
int deleteKeyShift(int keyOffset, char *key, RID *val, int *eLength, int attrLength);
#endif