#include "StdAfx.h"
#include "QU_Manager.h"
#include "RM_Manager.h"
#include "SYS_Manager.h"
#include <iostream>
#include <fstream>
struct table{
		char tablename[21];//����
		int attrcount;//�������
}tab;
struct column{
		char tablename[21];//����
		char attrname[21];//������
		AttrType attrtype;//��������
		int attrlength;//���Գ���
		int attroffset;//����ƫ�Ƶ�ַ
		char ix_flag;//�����Ƿ����
		char indexname[21];//������
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
	column *Column,*ctmp;//����ѿ�������Ե�����
	int allattrcount,allreccount=1;//��ȡ�����漰���ı���������Եĸ�������ѿ�������Ը���;�ѿ���ļ�¼����
	rm_table = (RM_FileHandle *)malloc(sizeof(RM_FileHandle));//��ϵͳ���ļ�
	rm_table->bOpen = false;
	if(RM_OpenFile("SYSTABLES", rm_table)!= SUCCESS)return SQL_SYNTAX;
	rm_column = (RM_FileHandle *)malloc(sizeof(RM_FileHandle));//��ϵͳ���ļ�
	rm_column->bOpen = false;
	if (RM_OpenFile("SYSCOLUMNS", rm_column)!= SUCCESS)return SQL_SYNTAX;
	rm_data=(RM_FileHandle *)malloc(nRelations*sizeof(RM_FileHandle));//��nRelations���¼�ļ�
	for(int i=0;i<nRelations;++i){
		(rm_data+i)->bOpen=false;
		if (RM_OpenFile(relations[i], rm_data)!= SUCCESS)return SQL_SYNTAX;
	}
	int *attrcount=(int*)malloc(nRelations*sizeof(int));//���ڴ洢ÿ�������Ը���
	int *reccount=(int*)malloc(nRelations*sizeof(int));//���ڴ洢ÿ���ļ�¼����
	for (int i=0;i<nRelations;++i){
		FileScan.bOpen = false;
		if (OpenScan(&FileScan, rm_table, 0, NULL)!= SUCCESS)return SQL_SYNTAX;
		while (GetNextRec(&FileScan, &rectab) == SUCCESS){
			if (strcmp(relations[i], rectab.pData) == 0){//����ƥ�䣬��ȡ���Ը���
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
	char ***results=new char**[allreccount];//����ѿ���ļ�¼����
	Column=(column*)malloc(allattrcount*sizeof(column));//�ѿ���ĸ�������
	ctmp=Column;
	for(int i=0;i<nRelations;++i){
		FileScan.bOpen = false;
	    if(OpenScan(&FileScan, rm_column, 0, NULL)!= SUCCESS)return SQL_SYNTAX;//��ϵͳ���ļ�ɨ��
		while(GetNextRec(&FileScan, &reccol) == SUCCESS){
			if(strcmp(relations[i],reccol.pData)==0){//�������
				for(int j=0;j<attrcount[i];++j,++ctmp){//��ζ�ȡ�ñ���������Լ�¼���ѿ����������
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

RC Query(char *sql,SelResult *res){
	sqlstr *sql_str = NULL;//����
	RC rc;
	sql_str = get_sqlstr();//��ʼ��
  	rc = parse(sql, sql_str);//ֻ����ַ��ؽ��SUCCESS��SQL_SYNTAX
	SelResult *res;
	if(rc==SUCCESS)
	if(Select (sql_str->sstr.sel.nSelAttrs,sql_str->sstr.sel.selAttrs,sql_str->sstr.sel.nRelations,sql_str->sstr.sel.relations,
		sql_str->sstr.sel.nConditions,sql_str->sstr.sel.conditions,res)==SUCCESS)return SUCCESS;
	return SQL_SYNTAX;
}