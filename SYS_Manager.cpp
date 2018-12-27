#include "stdafx.h"
#include "EditArea.h"
#include "SYS_Manager.h"
#include "QU_Manager.h"
#include <iostream>
#include <fstream>

struct table{
		char tablename[21];//����
		int attrcount;//��������
}tab;
struct column{
		char tablename[21];//����
		char attrname[21];//������
		int attrtype;//��������
		int attrlength;//���Գ���
		int attroffset;//����ƫ�Ƶ�ַ
		char ix_flag;//�����Ƿ����
		char indexname[21];//������
}col;
void ExecuteAndMessage(char * sql,CEditArea* editArea){//����ִ�е���������ڽ�������ʾִ�н�����˺������޸�
	std::string s_sql = sql;
	if(s_sql.find("select") == 0){//�ǲ�ѯ�����ִ�����£���������
		SelResult res;
		Init_Result(&res);
		//rc = Query(sql,&res);
		//����ѯ�������һ�£����������������ʽ
		//����editArea->ShowSelResult(col_num,row_num,fields,rows);
		int col_num = 5;//��
		int row_num = 3;//��
		char ** fields = new char *[5];//��Ϣ������5��
		for(int i = 0;i<col_num;i++){
			fields[i] = new char[20];
			memset(fields[i],0,20);
			fields[i][0] = 'f';
			fields[i][1] = i+'0';
		}
		char *** rows = new char**[row_num];
		for(int i = 0;i<row_num;i++){
			rows[i] = new char*[col_num];
			for(int j = 0;j<col_num;j++){
				rows[i][j] = new char[20];
				memset(rows[i][j],0,20);
				rows[i][j][0] = 'r';
				rows[i][j][1] = i + '0';
				rows[i][j][2] = '+';
				rows[i][j][3] = j + '0';
			}
		}
		editArea->ShowSelResult(col_num,row_num,fields,rows);
		for(int i = 0;i<5;i++){
			delete[] fields[i];
		}
		delete[] fields;
		Destory_Result(&res);
		return;
	}
	RC rc = execute(sql);//�ǲ�ѯ�����ִ������SQL��䣬�ɹ�����SUCCESS
	int row_num = 0;
	char**messages;
	switch(rc){
	case SUCCESS:
		row_num = 1;
		messages = new char*[row_num];
		messages[0] = "�����ɹ�";
		editArea->ShowMessage(row_num,messages);
		delete[] messages;
		break;
	case SQL_SYNTAX:
		row_num = 1;
		messages = new char*[row_num];
		messages[0] = "���﷨����";
		editArea->ShowMessage(row_num,messages);
		delete[] messages;
		break;
	default:
		row_num = 1;
		messages = new char*[row_num];
		messages[0] = "����δʵ��";
		editArea->ShowMessage(row_num,messages);
	delete[] messages;
		break;
	}
}

RC execute(char * sql){
	sqlstr *sql_str = NULL;//����
	RC rc;
	sql_str = get_sqlstr();//��ʼ��
  	rc = parse(sql, sql_str);//ֻ�����ַ��ؽ��SUCCESS��SQL_SYNTAX
	
	if (rc == SUCCESS)
	{
		int i = 0;
		switch (sql_str->flag)
		{
			//case 1:
			////�ж�SQL���Ϊselect���

			//break;

			case 2:
			//�ж�SQL���Ϊinsert���
				//RC Insert(char *relName,int nValues,Value * values);
			break;

			case 3:	
			//�ж�SQL���Ϊupdate���
				//RC Update(char *relName,char *attrName,Value *value,int nConditions,Condition *conditions);
			break;

			case 4:					
			//�ж�SQL���Ϊdelete���
				//RC Delete(char *relName,int nConditions,Condition *conditions);
			break;

			case 5:
			//�ж�SQL���ΪcreateTable���
				//RC CreateTable(char *relName,int attrCount,AttrInfo *attributes);
			break;

			case 6:	
			//�ж�SQL���ΪdropTable���
				//RC DropTable(char *relName);
			break;

			case 7:
			//�ж�SQL���ΪcreateIndex���
				//RC CreateIndex(char *indexName,char *relName,char *attrName);
			break;
	
			case 8:	
			//�ж�SQL���ΪdropIndex���
				//RC DropIndex(char *indexName);
			break;
			
			case 9:
			//�ж�Ϊhelp��䣬���Ը���������ʾ
			break;
		
			case 10: 
			//�ж�Ϊexit��䣬�����ɴ˽����˳�����
			break;		
		}
	}else{
		AfxMessageBox(sql_str->sstr.errors);//���������sql���ʷ�����������Ϣ
		return rc;
	}
}

RC CreateDB(char *dbpath,char *dbname){//����2��ϵͳ�ļ���0�������¼�ļ���0����������ļ�
	    if(CreateDirectory(strcat(dbpath,strcat("\\",dbname)),NULL)==SUCCESS){
			SetCurrentDirectory(strcat(dbpath,strcat("\\",dbname)));
			if(RM_CreateFile("SYSTABLES",sizeof(tab))==SUCCESS&&RM_CreateFile("SYSCOLUMNS",sizeof(col))==SUCCESS)		
			    return SUCCESS;
		}
		return SQL_SYNTAX;
}

RC DropDB(char *dbname){
	CFileFind find;
	bool isfinded=find.FindFile(strcat(dbname,"\\*.*"));
	while(isfinded){
		isfinded=find.FindNextFile();
		if(find.IsDots()){
			if(!find.IsDirectory()){
				DropTable(strcat(strcat(dbname,"\\"),find.GetFileName().GetBuffer(200)));
			}
		}
		else{
			DeleteFile(strcat(strcat(dbname,"\\"),find.GetFileName().GetBuffer(200)));
		}
	}
	find.Close();
	if(RemoveDirectory(dbname))return SUCCESS;
	else return SQL_SYNTAX;
}

RC OpenDB(char *dbname){
	return SUCCESS;
}


RC CloseDB(){	
	return SUCCESS;
}

RC CreateTable(char *relName,int attrCount,AttrInfo *attributes){
/*	FILE *fp1,*fp2;
	int length=0;
	if(fp1=fopen(strcat(strcat(path,strcat("\\",db)),"\\SYSTABLES"),"a+")){
		strcpy(tab.tablename,relName);
		tab.attrcount=attrCount;
		fwrite(&tab, sizeof(struct table), 1, fp1);
		if(fp2=fopen(strcat(strcat(path,strcat("\\",db)),"\\SYSCOLUMNS"),"a+")){
			for(int i=0;i<attrCount;++i){
			   strcpy(col.tablename,relName);
			   strcpy(col.attrname,attributes[i].attrName);
			   col.attrtype=attributes[i].attrType;
			   col.attrlength=attributes[i].attrLength;
			   length+=attributes[i].attrLength;
			   col.attroffset=i*76;
			   col.ix_flag='0';
			   fwrite(&col, sizeof(struct column), 1, fp2);
			}
		}
		fclose(fp1);
		fclose(fp2);
		//������Ӧ�ļ�¼�ļ�
		if(RM_CreateFile(strcat(strcat(path,strcat("\\",db)),strcat("\\",relName)),length)==true)return SUCCESS;*/
	char  *pData;
	RM_FileHandle *rm_table, *rm_column;
	RID *rid;
	int recordsize;//��¼�Ĵ�С
	AttrInfo *attrtmp = attributes;
	rm_table = (RM_FileHandle *)malloc(sizeof(RM_FileHandle));//��ϵͳ���ļ�
	rm_table->bOpen = false;
	if(RM_OpenFile("SYSTABLES", rm_table)!= SUCCESS)return SQL_SYNTAX;
	rm_column = (RM_FileHandle *)malloc(sizeof(RM_FileHandle));//��ϵͳ���ļ�
	rm_column->bOpen = false;
	if (RM_OpenFile("SYSCOLUMNS", rm_column)!= SUCCESS)return SQL_SYNTAX;
	pData = (char *)malloc(sizeof(tab));//����ϵͳ���ļ���Ϣ
	memcpy(pData, relName,21);
	memcpy(pData + 21, &attrCount, sizeof(int));
	rid = (RID *)malloc(sizeof(RID));
	rid->bValid = false;
	if (InsertRec(rm_table, pData, rid) != SUCCESS)return SQL_SYNTAX;
	if (RM_CloseFile(rm_table) != SUCCESS)return SQL_SYNTAX;
	free(rm_table);
	free(pData);
	free(rid);
	for (int i=0,offset=0;i<attrCount;++i,++attrtmp){//����ϵͳ���ļ���Ϣ
		pData = (char *)malloc(sizeof(col));
		memcpy(pData, relName,21);
		memcpy(pData+21, attributes[i].attrName,21);
		memcpy(pData+42, &(attrtmp->attrType),sizeof(int));
		memcpy(pData+42+sizeof(int),&(attrtmp->attrLength),sizeof(int));
		memcpy(pData+42+2*sizeof(int),&offset,sizeof(int));
		memcpy(pData+42+3*sizeof(int),"0",sizeof(char));
		rid = (RID *)malloc(sizeof(RID));
		rid->bValid = false;
		if (InsertRec(rm_column, pData, rid) != SUCCESS)return SQL_SYNTAX;
		free(pData);
		free(rid);
		offset +=attrtmp->attrLength;
	}
	if (RM_CloseFile(rm_column) != SUCCESS)return SQL_SYNTAX;
	free(rm_column);
	recordsize=0;//������Ӧ�ļ�¼�ļ�
	for (int i=0; i<attrCount;++i){
		recordsize +=attributes[i].attrLength;
	}
	if (RM_CreateFile(relName, recordsize) != SUCCESS)return SQL_SYNTAX;
	return SUCCESS;	
}

RC DropTable(char *relName){
/*	FILE *fp1,*fp2;
	fp1=fopen(strcat(strcat(path,strcat("\\",db)),"SYSTABLES"),"r");//ɾ��ϵͳ���ļ������Ϣ
	fp2=fopen(strcat(strcat(path,strcat("\\",db)),"beifen"),"w");//ͨ�����������ļ����ѷ�ɾ����Ϣ�����������δﵽɾ��Ŀ��
	while(!feof(fp1)){
	    fread(&tab,sizeof(struct table),1,fp1);
		if(strcmp(tab.tablename,relName))fwrite(&tab,sizeof(table),1,fp2);
	}
	fclose(fp1);
	fclose(fp2);
	fp1=fopen(strcat(strcat(path,strcat("\\",db)),"SYSTABLES"),"w");
	fp2=fopen(strcat(strcat(path,strcat("\\",db)),"beifen"),"r");
	while(!feof(fp2)){
		fread(&tab,sizeof(struct table),1,fp2);
		fwrite(&tab,sizeof(struct table),1,fp1);
	}
	fclose(fp1);
	fclose(fp2);
	fp1=fopen(strcat(strcat(path,strcat("\\",db)),"SYSTABLES"),"r");//ɾ��ϵͳ���ļ������Ϣ
	fp2=fopen(strcat(strcat(path,strcat("\\",db)),"beifen"),"w");//ͨ�����������ļ����ѷ�ɾ����Ϣ�����������δﵽɾ��Ŀ��
	while(!feof(fp1)){
	    fread(&col,sizeof(struct column),1,fp1);
		if(strcmp(col.tablename,relName))fwrite(&tab,sizeof(column),1,fp2);
	}
	fclose(fp1);
	fclose(fp2);
	fp1=fopen(strcat(strcat(path,strcat("\\",db)),"SYSTABLES"),"w");
	fp2=fopen(strcat(strcat(path,strcat("\\",db)),"beifen"),"r");
	while(!feof(fp2)){
		fread(&col,sizeof(struct column),1,fp2);
		fwrite(&col,sizeof(struct column),1,fp1);
	}
	fclose(fp1);
	fclose(fp2);
	//ɾ����
	if(remove(strcat(strcat(path,strcat("\\",db)),strcat("\\",relName)))==true)
	//ɾ������
	if(RemoveDirectory(strcat(strcat(path,strcat("\\",db)),strcat("\\",relName)))==true)return SUCCESS;*/
	CFile tmp;
	RM_FileHandle *rm_table, *rm_column;
	RC rc;
	RM_FileScan FileScan;
	RM_Record rectab, reccol;
	tmp.Remove((LPCTSTR)relName);//ɾ�����ݱ��ļ�
	rm_table = (RM_FileHandle *)malloc(sizeof(RM_FileHandle));//��ϵͳ���ļ�
	rm_table->bOpen = false;
	if(RM_OpenFile("SYSTABLES", rm_table)!= SUCCESS)return SQL_SYNTAX;
	rm_column = (RM_FileHandle *)malloc(sizeof(RM_FileHandle));//��ϵͳ���ļ�
	rm_column->bOpen = false;
	if (RM_OpenFile("SYSCOLUMNS", rm_column)!= SUCCESS)return SQL_SYNTAX;
	FileScan.bOpen = false;//��ϵͳ���ļ�����ɨ�裬ɾ��ͬ����ļ�¼��
	if (OpenScan(&FileScan, rm_table,0,NULL)!= SUCCESS)return SQL_SYNTAX;
	while(GetNextRec(&FileScan, &rectab)==SUCCESS){
		if (strcmp(relName, rectab.pData)==0){//����������ɾ������
			DeleteRec(rm_table,&(rectab.rid));
			break;//��Ϊֻ��һ�����ֱ������
		}
	}
	FileScan.bOpen = false;//��ϵͳ���ļ�����ɨ�裬ɾ��ͬ����ļ�¼��
	if (OpenScan(&FileScan, rm_column,0,NULL) != SUCCESS)return SQL_SYNTAX;
	while(GetNextRec(&FileScan, &reccol) == SUCCESS){
		if (strcmp(relName, reccol.pData) == 0){//����������ɾ������
			DeleteRec(rm_column, &(reccol.rid));//��Ϊ������ж���У�����Ҫ����
		}
	}
	if (RM_CloseFile(rm_table)!=SUCCESS)return SQL_SYNTAX;//�ر��ļ����
	free(rm_table);
	if (RM_CloseFile(rm_column)!=SUCCESS)return SQL_SYNTAX;
	free(rm_column);
	return SUCCESS;
}

RC CreateIndex(char *indexName,char *relName,char *attrName){//������������
	if(_access(strcat(strcat(path,strcat("\\",db)),strcat("\\",relName)),0)==-1){//�ļ��в�����
		 if(CreateDirectory(strcat(strcat(path,strcat("\\",db)),strcat("\\",relName)),NULL)==true){
			if(CreateFile(strcat(strcat(strcat(path,strcat("\\",db)),strcat("\\",relName)),strcat("\\",indexName)))==true)		
				
			    return SUCCESS;
		}
	}
}

RC DropIndex(char *indexName){
	if(_access(strcat(path,indexName),0)==-1)return SQL_SYNTAX;//�ļ��в�����
	else ;
}

RC Insert(char *relName,int nValues,Value * values){
	RM_OpenFile(strcat(strcat(path,strcat("\\",db)),strcat("\\",relName)),);
	InsertRec();
	RM_CloseFile();
}

RC Delete(char *relName,int nConditions,Condition *conditions){
	RM_OpenFile(strcat(strcat(path,strcat("\\",db)),strcat("\\",relName)),);
	DeleteRec();
	RM_CloseFile();
}

RC Update(char *relName,char *attrName,Value *value,int nConditions,Condition *conditions){
	RM_OpenFile(strcat(strcat(path,strcat("\\",db)),strcat("\\",relName)),);
	UpdateRec();
	RM_CloseFile();
}

bool CanButtonClick(){//��Ҫ����ʵ��
	//�����ǰ�����ݿ��Ѿ���
	return true;
	//�����ǰû�����ݿ��
	//return false;
}
