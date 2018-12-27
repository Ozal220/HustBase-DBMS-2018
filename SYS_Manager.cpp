#include "stdafx.h"
#include "EditArea.h"
#include "SYS_Manager.h"
#include "QU_Manager.h"
#include <iostream>
#include <fstream>

struct table{
		char tablename[21];//表名
		int attrcount;//属性数量
}tab;
struct column{
		char tablename[21];//表名
		char attrname[21];//属性名
		int attrtype;//属性类型
		int attrlength;//属性长度
		int attroffset;//属性偏移地址
		char ix_flag;//索引是否存在
		char indexname[21];//索引名
}col;
void ExecuteAndMessage(char * sql,CEditArea* editArea){//根据执行的语句类型在界面上显示执行结果。此函数需修改
	std::string s_sql = sql;
	if(s_sql.find("select") == 0){//是查询语句则执行以下，否则跳过
		SelResult res;
		Init_Result(&res);
		//rc = Query(sql,&res);
		//将查询结果处理一下，整理成下面这种形式
		//调用editArea->ShowSelResult(col_num,row_num,fields,rows);
		int col_num = 5;//列
		int row_num = 3;//行
		char ** fields = new char *[5];//消息不超过5行
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
	RC rc = execute(sql);//非查询语句则执行其他SQL语句，成功返回SUCCESS
	int row_num = 0;
	char**messages;
	switch(rc){
	case SUCCESS:
		row_num = 1;
		messages = new char*[row_num];
		messages[0] = "操作成功";
		editArea->ShowMessage(row_num,messages);
		delete[] messages;
		break;
	case SQL_SYNTAX:
		row_num = 1;
		messages = new char*[row_num];
		messages[0] = "有语法错误";
		editArea->ShowMessage(row_num,messages);
		delete[] messages;
		break;
	default:
		row_num = 1;
		messages = new char*[row_num];
		messages[0] = "功能未实现";
		editArea->ShowMessage(row_num,messages);
	delete[] messages;
		break;
	}
}

RC execute(char * sql){
	sqlstr *sql_str = NULL;//声明
	RC rc;
	sql_str = get_sqlstr();//初始化
  	rc = parse(sql, sql_str);//只有两种返回结果SUCCESS和SQL_SYNTAX
	
	if (rc == SUCCESS)
	{
		int i = 0;
		switch (sql_str->flag)
		{
			//case 1:
			////判断SQL语句为select语句

			//break;

			case 2:
			//判断SQL语句为insert语句
				//RC Insert(char *relName,int nValues,Value * values);
			break;

			case 3:	
			//判断SQL语句为update语句
				//RC Update(char *relName,char *attrName,Value *value,int nConditions,Condition *conditions);
			break;

			case 4:					
			//判断SQL语句为delete语句
				//RC Delete(char *relName,int nConditions,Condition *conditions);
			break;

			case 5:
			//判断SQL语句为createTable语句
				//RC CreateTable(char *relName,int attrCount,AttrInfo *attributes);
			break;

			case 6:	
			//判断SQL语句为dropTable语句
				//RC DropTable(char *relName);
			break;

			case 7:
			//判断SQL语句为createIndex语句
				//RC CreateIndex(char *indexName,char *relName,char *attrName);
			break;
	
			case 8:	
			//判断SQL语句为dropIndex语句
				//RC DropIndex(char *indexName);
			break;
			
			case 9:
			//判断为help语句，可以给出帮助提示
			break;
		
			case 10: 
			//判断为exit语句，可以由此进行退出操作
			break;		
		}
	}else{
		AfxMessageBox(sql_str->sstr.errors);//弹出警告框，sql语句词法解析错误信息
		return rc;
	}
}

RC CreateDB(char *dbpath,char *dbname){//包括2个系统文件、0到多个记录文件和0到多个索引文件
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
		//创建对应的记录文件
		if(RM_CreateFile(strcat(strcat(path,strcat("\\",db)),strcat("\\",relName)),length)==true)return SUCCESS;*/
	char  *pData;
	RM_FileHandle *rm_table, *rm_column;
	RID *rid;
	int recordsize;//记录的大小
	AttrInfo *attrtmp = attributes;
	rm_table = (RM_FileHandle *)malloc(sizeof(RM_FileHandle));//打开系统表文件
	rm_table->bOpen = false;
	if(RM_OpenFile("SYSTABLES", rm_table)!= SUCCESS)return SQL_SYNTAX;
	rm_column = (RM_FileHandle *)malloc(sizeof(RM_FileHandle));//打开系统列文件
	rm_column->bOpen = false;
	if (RM_OpenFile("SYSCOLUMNS", rm_column)!= SUCCESS)return SQL_SYNTAX;
	pData = (char *)malloc(sizeof(tab));//增加系统表文件信息
	memcpy(pData, relName,21);
	memcpy(pData + 21, &attrCount, sizeof(int));
	rid = (RID *)malloc(sizeof(RID));
	rid->bValid = false;
	if (InsertRec(rm_table, pData, rid) != SUCCESS)return SQL_SYNTAX;
	if (RM_CloseFile(rm_table) != SUCCESS)return SQL_SYNTAX;
	free(rm_table);
	free(pData);
	free(rid);
	for (int i=0,offset=0;i<attrCount;++i,++attrtmp){//增加系统列文件信息
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
	recordsize=0;//建立对应的记录文件
	for (int i=0; i<attrCount;++i){
		recordsize +=attributes[i].attrLength;
	}
	if (RM_CreateFile(relName, recordsize) != SUCCESS)return SQL_SYNTAX;
	return SUCCESS;	
}

RC DropTable(char *relName){
/*	FILE *fp1,*fp2;
	fp1=fopen(strcat(strcat(path,strcat("\\",db)),"SYSTABLES"),"r");//删除系统表文件相关信息
	fp2=fopen(strcat(strcat(path,strcat("\\",db)),"beifen"),"w");//通过建立备份文件，把非删除信息交换复制两次达到删除目的
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
	fp1=fopen(strcat(strcat(path,strcat("\\",db)),"SYSTABLES"),"r");//删除系统列文件相关信息
	fp2=fopen(strcat(strcat(path,strcat("\\",db)),"beifen"),"w");//通过建立备份文件，把非删除信息交换复制两次达到删除目的
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
	//删除表
	if(remove(strcat(strcat(path,strcat("\\",db)),strcat("\\",relName)))==true)
	//删除索引
	if(RemoveDirectory(strcat(strcat(path,strcat("\\",db)),strcat("\\",relName)))==true)return SUCCESS;*/
	CFile tmp;
	RM_FileHandle *rm_table, *rm_column;
	RC rc;
	RM_FileScan FileScan;
	RM_Record rectab, reccol;
	tmp.Remove((LPCTSTR)relName);//删除数据表文件
	rm_table = (RM_FileHandle *)malloc(sizeof(RM_FileHandle));//打开系统表文件
	rm_table->bOpen = false;
	if(RM_OpenFile("SYSTABLES", rm_table)!= SUCCESS)return SQL_SYNTAX;
	rm_column = (RM_FileHandle *)malloc(sizeof(RM_FileHandle));//打开系统列文件
	rm_column->bOpen = false;
	if (RM_OpenFile("SYSCOLUMNS", rm_column)!= SUCCESS)return SQL_SYNTAX;
	FileScan.bOpen = false;//打开系统表文件进行扫描，删除同名表的记录项
	if (OpenScan(&FileScan, rm_table,0,NULL)!= SUCCESS)return SQL_SYNTAX;
	while(GetNextRec(&FileScan, &rectab)==SUCCESS){
		if (strcmp(relName, rectab.pData)==0){//符合条件则删除该项
			DeleteRec(rm_table,&(rectab.rid));
			break;//因为只有一项，所以直接跳出
		}
	}
	FileScan.bOpen = false;//打开系统列文件进行扫描，删除同名表的记录项
	if (OpenScan(&FileScan, rm_column,0,NULL) != SUCCESS)return SQL_SYNTAX;
	while(GetNextRec(&FileScan, &reccol) == SUCCESS){
		if (strcmp(relName, reccol.pData) == 0){//符合条件则删除该项
			DeleteRec(rm_column, &(reccol.rid));//因为表可能有多个列，所以要遍历
		}
	}
	if (RM_CloseFile(rm_table)!=SUCCESS)return SQL_SYNTAX;//关闭文件句柄
	free(rm_table);
	if (RM_CloseFile(rm_column)!=SUCCESS)return SQL_SYNTAX;
	free(rm_column);
	return SUCCESS;
}

RC CreateIndex(char *indexName,char *relName,char *attrName){//对索引项排序
	if(_access(strcat(strcat(path,strcat("\\",db)),strcat("\\",relName)),0)==-1){//文件夹不存在
		 if(CreateDirectory(strcat(strcat(path,strcat("\\",db)),strcat("\\",relName)),NULL)==true){
			if(CreateFile(strcat(strcat(strcat(path,strcat("\\",db)),strcat("\\",relName)),strcat("\\",indexName)))==true)		
				
			    return SUCCESS;
		}
	}
}

RC DropIndex(char *indexName){
	if(_access(strcat(path,indexName),0)==-1)return SQL_SYNTAX;//文件夹不存在
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

bool CanButtonClick(){//需要重新实现
	//如果当前有数据库已经打开
	return true;
	//如果当前没有数据库打开
	//return false;
}
