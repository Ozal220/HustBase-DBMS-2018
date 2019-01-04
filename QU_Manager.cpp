#include "StdAfx.h"
#include "QU_Manager.h"
#include "RM_Manager.h"
#include "SYS_Manager.h"
#include <iostream>
#include <fstream>
struct table{
		char tablename[21];//表名
		int attrcount;//属性数量
}tab;
struct column{
		char tablename[21];//表名
		char attrname[21];//属性名
		AttrType attrtype;//属性类型
		int attrlength;//属性长度
		int attroffset;//属性偏移地址
		char ix_flag;//索引是否存在
		char indexname[21];//索引名
}col;
void Init_Result(SelResult * res){
	res->next_res = NULL;
}

void Destory_Result(SelResult * res){
	for(int i = 0;i<res->row_num;i++){
		for(int j = 0;j<res->col_num;j++){
			delete[] res->res[i][j];
		}
		delete[] res->res[i];
	}
	if(res->next_res != NULL){
		Destory_Result(res->next_res);
	}
}

RC Select(int nSelAttrs,RelAttr *selAttrs,int nRelations,char **relations,int nConditions,Condition *conditions,SelResult * res){
	RM_FileHandle *rm_table,*rm_column,*rm_data;
	RM_FileScan FileScan;
	RM_Record rectab,reccol;
	column *Column,*ctmp;//保存笛卡尔集的属性的数组
	int allattrcount,allreccount=1;//获取所有涉及到的表的所有属性的个数，即所求笛卡尔集的属性个数;笛卡尔集的记录总数
	rm_table = (RM_FileHandle *)malloc(sizeof(RM_FileHandle));//打开系统表文件
	rm_table->bOpen = false;
	if(RM_OpenFile("SYSTABLES", rm_table)!= SUCCESS)return SQL_SYNTAX;
	rm_column = (RM_FileHandle *)malloc(sizeof(RM_FileHandle));//打开系统列文件
	rm_column->bOpen = false;
	if (RM_OpenFile("SYSCOLUMNS", rm_column)!= SUCCESS)return SQL_SYNTAX;
	rm_data=(RM_FileHandle *)malloc(nRelations*sizeof(RM_FileHandle));//打开nRelations个记录文件
	for(int i=0;i<nRelations;++i){
		(rm_data+i)->bOpen=false;
		if (RM_OpenFile(relations[i], rm_data)!= SUCCESS)return SQL_SYNTAX;
	}
	int *attrcount=(int*)malloc(nRelations*sizeof(int));//用于存储每个表的属性个数
	int *reccount=(int*)malloc(nRelations*sizeof(int));//用于存储每个表的记录个数
	for (int i=0;i<nRelations;++i){
		FileScan.bOpen = false;
		if (OpenScan(&FileScan, rm_table, 0, NULL)!= SUCCESS)return SQL_SYNTAX;
		while (GetNextRec(&FileScan, &rectab) == SUCCESS){
			if (strcmp(relations[i], rectab.pData) == 0){//名字匹配，读取属性个数
				memcpy(attrcount+i, rectab.pData+21, sizeof(int));
				allattrcount+=*(attrcount+i);
				break;
			}
		}
		if (CloseScan(&FileScan)!= SUCCESS)return SQL_SYNTAX;
		FileScan.bOpen = false;
		if (OpenScan(&FileScan, rm_data+i, 0, NULL)!= SUCCESS)return SQL_SYNTAX;
		while (GetNextRec(&FileScan, &reccol) == SUCCESS){
			reccount[i]++;
		}
		allreccount*=reccount[i];
		if (CloseScan(&FileScan)!= SUCCESS)return SQL_SYNTAX;
	}
	char ***results=new char**[allreccount];//保存笛卡尔集的记录数组
	Column=(column*)malloc(allattrcount*sizeof(column));//笛卡尔集的各项属性
	ctmp=Column;
	for(int i=0;i<nRelations;++i){
		FileScan.bOpen = false;
	    if(OpenScan(&FileScan, rm_column, 0, NULL)!= SUCCESS)return SQL_SYNTAX;//打开系统列文件扫描
		while(GetNextRec(&FileScan, &reccol) == SUCCESS){
			if(strcmp(relations[i],reccol.pData)==0){//表名相符
				for(int j=0;j<attrcount[i];++j,++ctmp){//依次读取该表的所有属性记录到笛卡尔集的属性中
					memcpy(ctmp->tablename,reccol.pData,21);
					memcpy(ctmp->attrname,reccol.pData+21,21);
					memcpy(&(ctmp->attrtype),reccol.pData+42,sizeof(AttrType));
				    memcpy(&(ctmp->attrlength),reccol.pData+42+sizeof(AttrType),sizeof(int));
				    memcpy(&(ctmp->attroffset),reccol.pData+42+sizeof(int)+sizeof(AttrType),sizeof(int));
					memcpy(&(ctmp->ix_flag),reccol.pData+43+2*sizeof(int),1);
					memcpy(ctmp->indexname,reccol.pData+43+3*sizeof(int),21);
				}
				break;
			}
		}
		if(CloseScan(&FileScan)!= SUCCESS)return SQL_SYNTAX;
	}

	return SUCCESS;
}

RC Query(char *sql,SelResult *res){
	sqlstr *sql_str = NULL;//声明
	RC rc;
	sql_str = get_sqlstr();//初始化
  	rc = parse(sql, sql_str);//只有两种返回结果SUCCESS和SQL_SYNTAX
	SelResult *res;
	if(rc==SUCCESS)
	if(Select (sql_str->sstr.sel.nSelAttrs,sql_str->sstr.sel.selAttrs,sql_str->sstr.sel.nRelations,sql_str->sstr.sel.relations,
		sql_str->sstr.sel.nConditions,sql_str->sstr.sel.conditions,res)==SUCCESS)return SUCCESS;
	return SQL_SYNTAX;
}