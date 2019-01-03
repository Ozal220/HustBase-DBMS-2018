#include "stdafx.h"
#include "EditArea.h"
#include "SYS_Manager.h"
#include "QU_Manager.h"
#include <iostream>
#include <fstream>
#include<vector>
//创建一个vector用来保存句柄*filehandle
std::vector<RM_FileHandle *> vec;
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
void ExecuteAndMessage(char * sql,CEditArea* editArea){//根据执行的语句类型在界面上显示执行结果。
	std::string s_sql = sql;
	RC rc;
	if(s_sql.find("select") == 0){//是查询语句则执行以下，否则跳过
		SelResult res;
		Init_Result(&res);
		rc = Query(sql,&res);
		if(rc!=SUCCESS)return;
		int col_num = res.col_num;//列
		int row_num = 0;//行
		SelResult *tmp=&res;
		while(tmp){//所有节点的记录数之和
			row_num+=tmp->row_num;
			tmp=tmp->next_res;
		}
		char ** fields = new char *[20];//各字段名称
		for(int i = 0;i<col_num;i++){
			fields[i] = new char[20];
			memset(fields[i],'\0',20);
			memcpy(fields[i],res.fields[i],20);
		}
		tmp=&res;
		char *** rows = new char**[row_num];//结果集
		for(int i = 0;i<row_num;i++){
			rows[i] = new char*[col_num];//存放一条记录
			for (int j = 0; j <col_num; j++)
			{
				rows[i][j] = new char[20];//一条记录的一个字段
				memset(rows[i][j], '\0', 20);
				memcpy(rows[i][j],tmp->res[i][j],20);
			}
			if (i==99)tmp=tmp->next_res;//每个链表节点最多记录100条记录
		}
		editArea->ShowSelResult(col_num,row_num,fields,rows);
		for(int i = 0;i<20;i++){
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
	SelResult *res;
	if (rc == SUCCESS)
	{
		int i = 0;
		switch (sql_str->flag)
		{
			case 1:
			////判断SQL语句为select语句
			Select (sql_str->sstr.sel.nSelAttrs,sql_str->sstr.sel.selAttrs,sql_str->sstr.sel.nRelations,sql_str->sstr.sel.relations,sql_str->sstr.sel.nConditions,sql_str->sstr.sel.conditions,res);
			break;

			case 2:
			//判断SQL语句为insert语句
				Insert(sql_str->sstr.ins.relName,sql_str->sstr.ins.nValues,sql_str->sstr.ins.values);
			break;

			case 3:	
			//判断SQL语句为update语句
				Update(sql_str->sstr.upd.relName,sql_str->sstr.upd.attrName,&sql_str->sstr.upd.value,sql_str->sstr.upd.nConditions,sql_str->sstr.upd.conditions);
			break;

			case 4:					
			//判断SQL语句为delete语句
				Delete(sql_str->sstr.del.relName,sql_str->sstr.del.nConditions,sql_str->sstr.del.conditions);
			break;

			case 5:
			//判断SQL语句为createTable语句
				CreateTable(sql_str->sstr.cret.relName,sql_str->sstr.cret.attrCount,sql_str->sstr.cret.attributes);
			break;

			case 6:	
			//判断SQL语句为dropTable语句
				DropTable(sql_str->sstr.drt.relName);
			break;

			case 7:
			//判断SQL语句为createIndex语句
				CreateIndex(sql_str->sstr.crei.indexName,sql_str->sstr.crei.relName,sql_str->sstr.crei.attrName);
			break;
	
			case 8:	
			//判断SQL语句为dropIndex语句
				DropIndex(sql_str->sstr.dri.indexName);
			break;
			
			case 9:
			//判断为help语句，可以给出帮助提示
			break;
		
			case 10: 
			//判断为exit语句，可以由此进行退出操作
				AfxGetMainWnd()->SendMessage(WM_CLOSE);
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
	if(SetCurrentDirectory(dbname))return SUCCESS;//设置指定路径为工作路径
        else return SQL_SYNTAX;
}


RC CloseDB(){	
	for(std::vector<RM_FileHandle *>::size_type i = 0; i < vec.size(); i++){//遍历vector中的句柄关闭每一个打开的记录文件
		if(vec[i] != NULL)
			RM_CloseFile(vec[i]);
	}
	vec.clear();
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
	RM_FileScan FileScan;
	RM_Record rectab, reccol;
	char index[21];
	tmp.Remove((LPCTSTR)relName);//删除数据表文件(记录文件)
	rm_table = (RM_FileHandle *)malloc(sizeof(RM_FileHandle));//打开系统表文件
	rm_table->bOpen = false;
	if(RM_OpenFile("SYSTABLES", rm_table)!= SUCCESS)return SQL_SYNTAX;
	rm_column = (RM_FileHandle *)malloc(sizeof(RM_FileHandle));//打开系统列文件
	rm_column->bOpen = false;
	if (RM_OpenFile("SYSCOLUMNS", rm_column)!= SUCCESS)return SQL_SYNTAX;
	FileScan.bOpen = false;//打开系统表文件进行扫描，删除同名表的记录项
	if (OpenScan(&FileScan, rm_table,0,NULL)!= SUCCESS)return SQL_SYNTAX;
	while(GetNextRec(&FileScan, &rectab)==SUCCESS){
		memcpy(tab.tablename,rectab.pData,21);
		if (strcmp(relName,tab.tablename)==0){//符合条件则删除该项
			DeleteRec(rm_table,&(rectab.rid));
			break;//因为只有一项，所以直接跳出
		}
	}
	if(CloseScan(&FileScan)!=SUCCESS)return SQL_SYNTAX;
	FileScan.bOpen = false;//打开系统列文件进行扫描，删除同名表的记录项
	if (OpenScan(&FileScan, rm_column,0,NULL) != SUCCESS)return SQL_SYNTAX;
	while(GetNextRec(&FileScan, &reccol) == SUCCESS){
		memcpy(col.tablename,reccol.pData,21);
		if (strcmp(relName,col.tablename) == 0){//符合条件则删除该项对应的索引文件，之后再删除该项记录
			memcpy(index,reccol.pData+43+3*sizeof(int),21);
			if((reccol.pData+42+3*sizeof(int))=="1")tmp.Remove((LPCTSTR)index);	//删除索引文件
			DeleteRec(rm_column, &(reccol.rid));//因为表可能有多个列，所以要遍历
		}
	}
	if(CloseScan(&FileScan)!=SUCCESS)return SQL_SYNTAX;
	if (RM_CloseFile(rm_table)!=SUCCESS)return SQL_SYNTAX;//关闭文件句柄
	free(rm_table);
	if (RM_CloseFile(rm_column)!=SUCCESS)return SQL_SYNTAX;
	free(rm_column);
	return SUCCESS;
}

RC CreateIndex(char *indexName,char *relName,char *attrName){
	RM_FileHandle *rm_column,*rm_data;
	IX_IndexHandle *rm_index;
	RM_FileScan FileScan;					
	RM_Record reccol;
	rm_column = (RM_FileHandle *)malloc(sizeof(RM_FileHandle));//打开系统列文件
	rm_column->bOpen = false;
	if (RM_OpenFile("SYSCOLUMNS", rm_column)!=SUCCESS){
		AfxMessageBox("系统列文件打开失败");
		return SQL_SYNTAX;
	}
	FileScan.bOpen = false;//打开系统列文件进行扫描，修改同名索引的记录项，ix_flag变0。
	if (OpenScan(&FileScan, rm_column, 0, NULL)!=SUCCESS){
		AfxMessageBox("系统列文件扫描失败");
		return SQL_SYNTAX;
	}
	while (GetNextRec(&FileScan, &reccol) == SUCCESS){
		memcpy(col.tablename,reccol.pData,21);
		memcpy(col.attrname,reccol.pData+21,21);
		char*type,*length,*offset;
		memcpy(type,reccol.pData+42,sizeof(int));
		switch((int)type){
		case 0:col.attrtype=chars;break;
		case 1:col.attrtype=ints;break;
		case 2:col.attrtype=floats;break; 
		}
		memcpy(length,reccol.pData+42+sizeof(int),sizeof(int));
		col.attrlength=(int)length;
		memcpy(offset,reccol.pData+42+2*sizeof(int),sizeof(int));
		col.attroffset=(int)offset;
		if (strcmp(relName,col.tablename)==0&&strcmp(attrName,col.attrname)==0){//表名和属性名相符,找到该项
			if((reccol.pData+42+3*sizeof(int))=="1")return SQL_SYNTAX;//存在索引则报错
			else{//创建索引
				if(CloseScan(&FileScan)!=SUCCESS)return SQL_SYNTAX;
				CreateIndex(indexName,col.attrtype,col.attrlength);//创建索引文件
				memcpy(reccol.pData+42+3*sizeof(int),"1",1);//更改系统列文件对应项的ix_flag和indexName
				memcpy(reccol.pData+43+3*sizeof(int),indexName,21);
				UpdateRec(rm_column,&reccol);
				rm_index = (IX_IndexHandle *)malloc(sizeof(IX_IndexHandle));//打开索引文件
	            rm_index->bOpen = false;
	            if(OpenIndex(indexName, rm_index)!=SUCCESS){
		            AfxMessageBox("索引文件打开失败");
		            return SQL_SYNTAX;
	            }
				rm_data = (RM_FileHandle *)malloc(sizeof(RM_FileHandle));//打开表的记录文件
	            rm_data->bOpen = false;
	            if (RM_OpenFile(relName, rm_data)!=SUCCESS){
		           AfxMessageBox("记录文件打开失败");
		           return SQL_SYNTAX;
	            }
				FileScan.bOpen = false;//打开表的记录文件进行扫描
	            if (OpenScan(&FileScan, rm_data,0,NULL) != SUCCESS){
					AfxMessageBox("记录文件扫描失败");
					return SQL_SYNTAX;
				}
	            while (GetNextRec(&FileScan, &reccol) == SUCCESS){
		            char *data = (char *)malloc(col.attrlength);
		            memcpy(data, reccol.pData + col.attroffset, col.attrlength);
		            InsertEntry(rm_index, data, &(reccol.rid));
	            }
	            if(CloseScan(&FileScan)!=SUCCESS)return SQL_SYNTAX;
				if(RM_CloseFile(rm_data)!=SUCCESS)return SQL_SYNTAX;
				if(CloseIndex(rm_index)!=SUCCESS)return SQL_SYNTAX;
				if(RM_CloseFile(rm_column)!=SUCCESS)return SQL_SYNTAX;
				break;//只有一项符合，创建完后就跳出
			}
		}
	}
	return SUCCESS;
}

RC DropIndex(char *indexName){
	CFile tmp;
	RM_FileHandle *rm_column;
	RM_FileScan FileScan;
	RM_Record reccol;	
	char index[21];
	rm_column = (RM_FileHandle *)malloc(sizeof(RM_FileHandle));//打开系统列文件
	rm_column->bOpen = false;
	if (RM_OpenFile("SYSCOLUMNS", rm_column)!=SUCCESS){
		AfxMessageBox("系统列文件打开失败");
		return SQL_SYNTAX;
	}
	FileScan.bOpen = false;//打开系统列文件进行扫描，修改同名索引的记录项，flag变0。
	if (OpenScan(&FileScan, rm_column, 0, NULL)!=SUCCESS){
		AfxMessageBox("系统列文件扫描失败");
		return SQL_SYNTAX;
	}
	while (GetNextRec(&FileScan, &reccol) == SUCCESS){
		memcpy(index,reccol.pData+43+3*sizeof(int),21);
		if (strcmp(indexName,index)==0){//索引名相符,找到该项
			if((reccol.pData+42+3*sizeof(int))=="0")return SQL_SYNTAX;//不存在索引则报错
			else{
				memcpy(reccol.pData+42+3*sizeof(int),"0",1);//存在则标记位置0且删除索引文件
			    if (UpdateRec(rm_column,&reccol)!=SUCCESS)return SQL_SYNTAX;
				tmp.Remove((LPCTSTR)indexName);//删除索引文件
			}
		}
	}
	if(CloseScan(&FileScan)!=SUCCESS)return SQL_SYNTAX;
	if (RM_CloseFile(rm_column)!= SUCCESS)return SQL_SYNTAX;
	return SUCCESS;
}

RC Insert(char *relName,int nValues,Value *values){
	CFile tmp;
	RM_FileHandle *rm_data,*rm_table,*rm_column;
	char *value;//读取数据表信息
	RID *rid;
	column *Column, *ctmp;//用于存储一个表的所有属性的值
	RM_FileScan FileScan;
	RM_Record rectab, reccol;
	int attrcount;//属性数量
	char index[21],attr[21];
	//打开数据表,系统表，系统列文件
	rm_data = (RM_FileHandle *)malloc(sizeof(RM_FileHandle));
	rm_data->bOpen = false;
	if (RM_OpenFile(relName, rm_data)!= SUCCESS){
		AfxMessageBox("记录文件打开失败");
		return SQL_SYNTAX;
	}
	rm_table = (RM_FileHandle *)malloc(sizeof(RM_FileHandle));
	rm_table->bOpen = false;
	if (RM_OpenFile("SYSTABLES", rm_table)!= SUCCESS){
		AfxMessageBox("系统表文件打开失败");
		return SQL_SYNTAX;
	}
	rm_column = (RM_FileHandle *)malloc(sizeof(RM_FileHandle));
	rm_column->bOpen = false;
	if (RM_OpenFile("SYSCOLUMNS", rm_column)!= SUCCESS){
		AfxMessageBox("系统列文件打开失败");
		return SQL_SYNTAX;
	}
	//打开系统表文件进行扫描
	FileScan.bOpen = false;
	if (OpenScan(&FileScan, rm_table, 0, NULL)!= SUCCESS){
		AfxMessageBox("系统表文件扫描失败");
		return SQL_SYNTAX;
	}
	//循环查找表名为relName对应的系统表中的记录,读取属性数量
	while (GetNextRec(&FileScan, &rectab) == SUCCESS){
		if (strcmp(relName, rectab.pData) == 0){
			memcpy(&attrcount, rectab.pData + 21, sizeof(int));
			break;
		}	
	}
	//关闭系统表文件扫描
	if(CloseScan(&FileScan)!=SUCCESS)return SQL_SYNTAX;
	//判定属性数量是否相等
	if (attrcount != nValues){
		AfxMessageBox("属性个数不等，插入失败！");
		return SQL_SYNTAX;
	}
	//打开系统列文件扫描
	FileScan.bOpen = false;
	if (OpenScan(&FileScan, rm_column, 0, NULL)!= SUCCESS){
		AfxMessageBox("系统列文件扫描失败");
		return SQL_SYNTAX;
	}
	//根据之前读取的系统表中信息，读取属性信息，结果保存在ctmp中
	Column = (column *)malloc(attrcount*sizeof(column));
	ctmp = Column;
	while (GetNextRec(&FileScan, &reccol) == SUCCESS){
		if (strcmp(relName, reccol.pData) == 0){//找到表名为relName的第一个记录，依次读取attrcount个记录
			for (int i = 0; i < attrcount; i++, ctmp++){
				memcpy(ctmp->tablename, reccol.pData, 21);
				memcpy(ctmp->attrname, reccol.pData + 21, 21);
				memcpy(&(ctmp->attrtype), reccol.pData + 42, sizeof(AttrType));
				memcpy(&(ctmp->attrlength), reccol.pData + 42 + sizeof(AttrType), sizeof(int));
				memcpy(&(ctmp->attroffset), reccol.pData + 42 + sizeof(int)+sizeof(AttrType), sizeof(int));
				if (GetNextRec(&FileScan, &reccol)!= SUCCESS)break;
			}
			break;
		}
	}
	ctmp = Column;
	//向记录文件中循环插入记录
	value = (char *)malloc(rm_data->recSize);
	values = values + nValues -1;
	for (int i = 0; i < nValues; i++, values--,ctmp++){
		memcpy(value + ctmp->attroffset, values->data, ctmp->attrlength);
	}
	rid = (RID*)malloc(sizeof(RID));
	rid->bValid = false;
	InsertRec(rm_data, value, rid);
	//关闭系统列文件扫描
	if(CloseScan(&FileScan)!=SUCCESS)return SQL_SYNTAX;
	//打开系统列文件扫描
	FileScan.bOpen = false;
	if (OpenScan(&FileScan, rm_column, 0, NULL)!= SUCCESS){
		AfxMessageBox("系统列文件扫描失败");
		return SQL_SYNTAX;
	}
	//扫描系统列文件，如果该属性上存在索引，则插入索引项
	while (GetNextRec(&FileScan, &reccol) == SUCCESS){
		if (strcmp(relName, reccol.pData) == 0){//找到表名为relName的第一个记录，依次读取attrcount个记录
			for (int i = 0; i < attrcount; i++){
				if((reccol.pData+42+3*sizeof(int))=="1"){//ix_flag为1，该属性上存在索引，需插入新的索引项
					IX_IndexHandle *rm_index;
					memcpy(index,reccol.pData+43+2*sizeof(int)+sizeof(AttrType),21);
					rm_index = (IX_IndexHandle *)malloc(sizeof(IX_IndexHandle));//打开索引文件
	                rm_index->bOpen = false;
	                if(OpenIndex(index, rm_index)!=SUCCESS){
		               AfxMessageBox("索引文件打开失败");
		               return SQL_SYNTAX;
	                }
					char*length,*offset;
					memcpy(length,reccol.pData+42+sizeof(int),sizeof(int));
		            memcpy(offset,reccol.pData+42+2*sizeof(int),sizeof(int));
					char *data = (char *)malloc((int)length);
		            memcpy(data,value+(int)offset,(int)length);
		            InsertEntry(rm_index,data,&(reccol.rid));
					if(CloseIndex(rm_index)!=SUCCESS)return SQL_SYNTAX;
					free(rm_index);
				}
			}
			break;
		}
	}
	free(value);
	free(rid);
	free(Column);
/*	//关闭系统列文件扫描
	if(CloseScan(&FileScan)!=SUCCESS)return SQL_SYNTAX;
	//打开系统列文件扫描
	FileScan.bOpen = false;
	if (OpenScan(&FileScan, rm_column, 0, NULL)!= SUCCESS){
		AfxMessageBox("系统列文件扫描失败");
		return SQL_SYNTAX;
	}
	//扫描系统列文件，如果该属性上存在索引，则删除原有索引重新创建
	while (GetNextRec(&FileScan, &reccol) == SUCCESS){
		if (strcmp(relName, reccol.pData) == 0){//找到表名为relName的第一个记录，依次读取attrcount个记录
			for (int i = 0; i < attrcount; i++){
				if((reccol.pData+42+3*sizeof(int))=="1"){
					memcpy(attr,reccol.pData+21,21);
					memcpy(index,reccol.pData+43+2*sizeof(int)+sizeof(AttrType),21);
					memcpy(reccol.pData+42+3*sizeof(int),"0",1);//索引标记项改为0
			        if (UpdateRec(rm_column,&reccol)!=SUCCESS)return SQL_SYNTAX;
					tmp.Remove((LPCTSTR)index);//删除索引文件
					CreateIndex(index,relName,attr);
				}
			}
			break;
		}
	}*/
	//关闭系统列文件扫描
	if(CloseScan(&FileScan)!=SUCCESS)return SQL_SYNTAX;
	//关闭文件
	if (RM_CloseFile(rm_data)!= SUCCESS){
		AfxMessageBox("记录文件关闭失败");
		return SQL_SYNTAX;
	}
	free(rm_data);
	if (RM_CloseFile(rm_table)!= SUCCESS){
		AfxMessageBox("系统表文件关闭失败");
		return SQL_SYNTAX;
	}
	free(rm_table);
	if (RM_CloseFile(rm_column)!= SUCCESS){
		AfxMessageBox("系统列文件关闭失败");
		return SQL_SYNTAX;
	}
	free(rm_column);
	return SUCCESS;
}

RC Delete(char *relName,int nConditions,Condition *conditions){
	CFile tmp;
	RM_FileHandle *rm_data,*rm_table,*rm_column;
	RM_FileScan FileScan;
	RM_Record recdata,rectab,reccol;
	column *Column, *ctmp,*ctmpleft,*ctmpright;
	Condition *contmp;
	int i,torf;//是否符合删除条件
	int attrcount;//属性数量
	int intleft,intright; 
	char *charleft,*charright;
	float floatleft,floatright;//属性的值
	AttrType attrtype;
	char index[21],attr[21];
	//打开记录,系统表，系统列文件
	rm_data = (RM_FileHandle *)malloc(sizeof(RM_FileHandle));
	rm_data->bOpen = false;
	if (RM_OpenFile(relName, rm_data)!= SUCCESS)return SQL_SYNTAX;
	rm_table = (RM_FileHandle *)malloc(sizeof(RM_FileHandle));
	rm_table->bOpen = false;
	if (RM_OpenFile("SYSTABLES", rm_table)!= SUCCESS)return SQL_SYNTAX;
	rm_column = (RM_FileHandle *)malloc(sizeof(RM_FileHandle));
	rm_column->bOpen = false;
	if (RM_OpenFile("SYSCOLUMNS", rm_column)!= SUCCESS)return SQL_SYNTAX;

	//打开系统表文件扫描
	FileScan.bOpen = false;
	if (OpenScan(&FileScan, rm_table, 0, NULL)!= SUCCESS)return SQL_SYNTAX;
	//循环查找表名为relName对应的系统表中的记录,记录属性数量attrcount
	while (GetNextRec(&FileScan, &rectab) == SUCCESS){
		if (strcmp(relName, rectab.pData) == 0){
			memcpy(&attrcount, rectab.pData + 21, sizeof(int));
			break;
		}
	}
	//关闭系统表文件扫描
	if(CloseScan(&FileScan)!=SUCCESS)return SQL_SYNTAX;
	//打开系统列文件扫描
	FileScan.bOpen = false;
	if (OpenScan(&FileScan, rm_column, 0, NULL)!= SUCCESS)return SQL_SYNTAX;
	//根据之前读取的系统表中信息，读取属性信息，结果保存在ctmp中
	Column = (column *)malloc(attrcount*sizeof(column));
	ctmp = Column;
	while (GetNextRec(&FileScan, &reccol) == SUCCESS){
		if (strcmp(relName, reccol.pData) == 0){//找到表名为relName的第一个记录，依次读取attrcount个记录
			for (int i = 0; i < attrcount; i++,ctmp++){
				memcpy(ctmp->tablename, reccol.pData, 21);
				memcpy(ctmp->attrname, reccol.pData + 21, 21);
				memcpy(&(ctmp->attrtype), reccol.pData + 42, sizeof(AttrType));
				memcpy(&(ctmp->attrlength), reccol.pData + 42 + sizeof(AttrType), sizeof(int));
				memcpy(&(ctmp->attroffset), reccol.pData + 42 + sizeof(int)+sizeof(AttrType), sizeof(int));				
				if(GetNextRec(&FileScan, &reccol)!= SUCCESS)break;
			}
			break;
		}
	}
	//关闭系统列文件扫描
	if(CloseScan(&FileScan)!=SUCCESS)return SQL_SYNTAX;
	//打开记录文件扫描
	FileScan.bOpen = false;
	if (OpenScan(&FileScan, rm_data, 0, NULL)!= SUCCESS)return SQL_SYNTAX;
	//循环查找表名为relName对应的数据表中的记录,并将记录信息保存于recdata中
	while (GetNextRec(&FileScan, &recdata) == SUCCESS){	//取记录做判断
		for (i = 0, torf = 1,contmp = conditions;i < nConditions; i++, contmp++){//conditions条件逐一判断
			ctmpleft = ctmpright = Column;//每次循环都要将遍历整个系统列文件，找到各个条件对应的属性
			//左属性右值
			if (contmp->bLhsIsAttr == 1 && contmp->bRhsIsAttr == 0){
				for (int j = 0; j < attrcount; j++){//attrcount个属性逐一判断
					if (contmp->lhsAttr.relName == NULL){//当条件中未指定表名时，默认为relName
						contmp->lhsAttr.relName = (char *)malloc(21);
						strcpy(contmp->lhsAttr.relName, relName);
					}
					if ((strcmp(ctmpleft->tablename, contmp->lhsAttr.relName) == 0)
						&& (strcmp(ctmpleft->attrname, contmp->lhsAttr.attrName) == 0)){//根据表名属性名找到对应属性
						break;
					}
					ctmpleft++;
				}
				//对conditions的某一个条件进行判断
				switch (ctmpleft->attrtype){//判定属性的类型
					case ints:
						attrtype = ints;
						memcpy(&intleft, recdata.pData + ctmpleft->attroffset, sizeof(int));
						memcpy(&intright, contmp->rhsValue.data, sizeof(int));
						break;
					case chars:
						attrtype = chars;
						charleft = (char *)malloc(ctmpleft->attrlength);
						memcpy(charleft, recdata.pData + ctmpleft->attroffset, ctmpleft->attrlength);
						charright = (char *)malloc(ctmpleft->attrlength);
						memcpy(charright, contmp->rhsValue.data, ctmpleft->attrlength);
						break;
					case floats:
						attrtype = floats;
						memcpy(&floatleft, recdata.pData + ctmpleft->attroffset, sizeof(float));
						memcpy(&floatright, contmp->rhsValue.data, sizeof(float));
						break;
				}
			}
			//右属性左值
			if (contmp->bLhsIsAttr == 0 && contmp->bRhsIsAttr == 1){
				for (int j = 0; j < attrcount; j++){//attrcount个属性逐一判断
					if (contmp->rhsAttr.relName == NULL){//当条件中未指定表名时，默认为relName
						contmp->rhsAttr.relName = (char *)malloc(21);
						strcpy(contmp->rhsAttr.relName, relName);
					}
					if ((strcmp(ctmpright->tablename, contmp->rhsAttr.relName) == 0)
						&& (strcmp(ctmpright->attrname, contmp->rhsAttr.attrName) == 0)){//根据表名属性名找到对应属性
						break;
					}
					ctmpright++;
				}
				//对conditions的某一个条件进行判断
				switch (ctmpright->attrtype){
				case ints:
					attrtype = ints;
					memcpy(&intleft, contmp->lhsValue.data, sizeof(int));
					memcpy(&intright, recdata.pData + ctmpright->attroffset, sizeof(int));
					break;
				case chars:
					attrtype = chars;
					charleft = (char *)malloc(ctmpright->attrlength);
					memcpy(charleft, contmp->lhsValue.data, ctmpright->attrlength);
					charright = (char *)malloc(ctmpright->attrlength);
					memcpy(charright, recdata.pData + ctmpright->attroffset, ctmpright->attrlength);
					break;
				case floats:
					attrtype = floats;
					memcpy(&floatleft, contmp->lhsValue.data, sizeof(float));
					memcpy(&floatright, recdata.pData + ctmpright->attroffset, sizeof(float));
					break;
				}
			}
			//左右均属性
			else  if (contmp->bLhsIsAttr == 1 && contmp->bRhsIsAttr == 1){
				for (int j = 0; j < attrcount; j++){//attrcount个属性逐一判断
					if (contmp->lhsAttr.relName == NULL){//当条件中未指定表名时，默认为relName
						contmp->lhsAttr.relName = (char *)malloc(21);
						strcpy(contmp->lhsAttr.relName, relName);
					}
					if ((strcmp(ctmpleft->tablename, contmp->lhsAttr.relName) == 0)
						&& (strcmp(ctmpleft->attrname, contmp->lhsAttr.attrName) == 0)){//根据表名属性名找到对应属性
						break;
					}
					ctmpleft++;
				}
				for (int j = 0; j < attrcount; j++){//attrcount个属性逐一判断
					if (contmp->rhsAttr.relName == NULL){//当条件中未指定表名时，默认为relName
						contmp->rhsAttr.relName = (char *)malloc(21);
						strcpy(contmp->rhsAttr.relName, relName);
					}
					if ((strcmp(ctmpright->tablename, contmp->rhsAttr.relName) == 0)
						&& (strcmp(ctmpright->attrname, contmp->rhsAttr.attrName) == 0)){//根据表名属性名找到对应属性
						break;
					}
					ctmpright++;
				}
				//对conditions的某一个条件进行判断
				switch (ctmpright->attrtype){
					case ints:
						attrtype = ints;
						memcpy(&intleft, recdata.pData + ctmpleft->attroffset, sizeof(int));
						memcpy(&intright, recdata.pData + ctmpright->attroffset, sizeof(int));
						break;
					case chars:
						attrtype = chars;
						charleft = (char *)malloc(ctmpright->attrlength);
						memcpy(charleft, recdata.pData + ctmpleft->attroffset, ctmpright->attrlength);
						charright = (char *)malloc(ctmpright->attrlength);
						memcpy(charright, recdata.pData + ctmpright->attroffset, ctmpright->attrlength);
						break;
					case floats:
						attrtype = floats;
						memcpy(&floatleft, recdata.pData + ctmpleft->attroffset, sizeof(float));
						memcpy(&floatright, recdata.pData + ctmpright->attroffset, sizeof(float));
						break;
				}
			}
			if (attrtype == ints){
				if ((intleft == intright && contmp->op == EQual) ||
					(intleft > intright && contmp->op == GreatT) ||
					(intleft >= intright && contmp->op == GEqual) ||
					(intleft < intright && contmp->op == LessT) ||
					(intleft <= intright && contmp->op == LEqual) ||
					(intleft != intright && contmp->op == NEqual))
					torf &= 1;
				else
					torf &= 0;
			}
			else if (attrtype == chars){
				if ((strcmp(charleft, charright) == 0 && contmp->op == EQual) ||
					(strcmp(charleft, charright) > 0 && contmp->op == GreatT) ||
					((strcmp(charleft, charright) > 0 || strcmp(charleft, charright) == 0) && contmp->op == GEqual) ||
					(strcmp(charleft, charright) < 0 && contmp->op == LessT) ||
					((strcmp(charleft, charright) < 0 || strcmp(charleft, charright) == 0) && contmp->op == LEqual) ||
					(strcmp(charleft, charright) != 0 && contmp->op == NEqual))
					torf &= 1;
				else
					torf &= 0;
				free(charleft);
				free(charright);
			}
			else if (attrtype == floats){
				if ((floatleft == floatright && contmp->op == EQual) ||
					(floatleft > floatright && contmp->op == GreatT) ||
					(floatleft >= floatright && contmp->op == GEqual) ||
					(floatleft < floatright && contmp->op == LessT) ||
					(floatleft <= floatright && contmp->op == LEqual) ||
					(floatleft != floatright && contmp->op == NEqual))
					torf &= 1;
				else
					torf &= 0;
			}
			else
				torf &= 0;
		}

		if (torf == 1){
			DeleteRec(rm_data, &(recdata.rid));
			//打开系统列文件扫描
			FileScan.bOpen = false;
			if (OpenScan(&FileScan, rm_column, 0, NULL)!= SUCCESS){
				AfxMessageBox("系统列文件扫描失败");
				return SQL_SYNTAX;
			}
			//扫描系统列文件，如果该属性上存在索引，则删除索引项
			while (GetNextRec(&FileScan, &reccol) == SUCCESS){
				if (strcmp(relName, reccol.pData) == 0){//找到表名为relName的第一个记录，依次读取attrcount个记录
					for (int i = 0; i < attrcount; i++){
						if((reccol.pData+42+3*sizeof(int))=="1"){//ix_flag为1，该属性上存在索引，需删除原有的索引项
							IX_IndexHandle *rm_index;
							memcpy(index,reccol.pData+43+2*sizeof(int)+sizeof(AttrType),21);
							rm_index = (IX_IndexHandle *)malloc(sizeof(IX_IndexHandle));//打开索引文件
			                rm_index->bOpen = false;
			                if(OpenIndex(index, rm_index)!=SUCCESS){
				               AfxMessageBox("索引文件打开失败");
				               return SQL_SYNTAX;
			                }
							char*length,*offset;
							memcpy(length,reccol.pData+42+sizeof(int),sizeof(int));
				            memcpy(offset,reccol.pData+42+2*sizeof(int),sizeof(int));
							char *data = (char *)malloc((int)length);
							memcpy(data,recdata.pData+(int)offset,(int)length);
				            DeleteEntry(rm_index,data,&(recdata.rid));
							if(CloseIndex(rm_index)!=SUCCESS)return SQL_SYNTAX;//关闭索引文件
							free(rm_index);
						}
					}
					break;
				}
			}
		}	
	}
	free(Column);
	//关闭记录文件扫描
	if(CloseScan(&FileScan)!=SUCCESS)return SQL_SYNTAX;
/*	//打开系统列文件扫描
	FileScan.bOpen = false;
	if (OpenScan(&FileScan, rm_column, 0, NULL)!= SUCCESS){
		AfxMessageBox("系统列文件扫描失败");
		return SQL_SYNTAX;
	}
	//扫描系统列文件，如果该属性上存在索引，则删除原有索引重新创建
	while (GetNextRec(&FileScan, &reccol) == SUCCESS){
		if (strcmp(relName, reccol.pData) == 0){//找到表名为relName的第一个记录，依次读取attrcount个记录
			for (int i = 0; i < attrcount; i++){
				if((reccol.pData+42+3*sizeof(int))=="1"){
					memcpy(attr,reccol.pData+21,21);
					memcpy(index,reccol.pData+43+2*sizeof(int)+sizeof(AttrType),21);
					memcpy(reccol.pData+42+3*sizeof(int),"0",1);//索引标记项改为0
			        if (UpdateRec(rm_column,&reccol)!=SUCCESS)return SQL_SYNTAX;
					tmp.Remove((LPCTSTR)index);//删除索引文件
					CreateIndex(index,relName,attr);
				}
			}
			break;
		}
	}
	//关闭系统列文件扫描
	if(CloseScan(&FileScan)!=SUCCESS)return SQL_SYNTAX;*/
	//关闭文件
	if (RM_CloseFile(rm_table)!= SUCCESS)return SQL_SYNTAX;
	free(rm_table);
	if (RM_CloseFile(rm_column)!= SUCCESS)return SQL_SYNTAX;
	free(rm_column);
	if (RM_CloseFile(rm_data)!= SUCCESS)return SQL_SYNTAX;
	free(rm_data);
	return SUCCESS;
}

RC Update(char *relName,char *attrName,Value *value,int nConditions,Condition *conditions){//只能进行单值更新
	RM_FileHandle *rm_data, *rm_table, *rm_column;
	RM_FileScan FileScan;
	RM_Record recdata, rectab, reccol;
	column *Column, *ctmp,*cupdate,*ctmpleft,*ctmpright;
	Condition *contmp;
	int i, torf;//是否符合删除条件
	int attrcount;//属性数量
	int intleft,intright;
	char *charleft,*charright;
	float floatleft,floatright;//属性的值
	AttrType attrtype;
	//打开记录,系统表，系统列文件
	rm_data = (RM_FileHandle *)malloc(sizeof(RM_FileHandle));
	rm_data->bOpen = false;
	if (RM_OpenFile(relName, rm_data)!= SUCCESS)return SQL_SYNTAX;
	rm_table = (RM_FileHandle *)malloc(sizeof(RM_FileHandle));
	rm_table->bOpen = false;
	if (RM_OpenFile("SYSTABLES", rm_table)!= SUCCESS)return SQL_SYNTAX;
	rm_column = (RM_FileHandle *)malloc(sizeof(RM_FileHandle));
	rm_column->bOpen = false;
	if (RM_OpenFile("SYSCOLUMNS", rm_column)!= SUCCESS)return SQL_SYNTAX;
	//打开系统表文件扫描
	FileScan.bOpen = false;
	if (OpenScan(&FileScan, rm_table, 0, NULL)!= SUCCESS)return SQL_SYNTAX;
	//循环查找表名为relName对应的系统表中的记录,记录属性数量attrcount
	while (GetNextRec(&FileScan, &rectab) == SUCCESS){
		if (strcmp(relName, rectab.pData) == 0){
			memcpy(&attrcount, rectab.pData + 21, sizeof(int));
			break;
		}
	}
	//关闭系统表文件扫描
	if(CloseScan(&FileScan)!=SUCCESS)return SQL_SYNTAX;
	//打开系统列文件扫描
	FileScan.bOpen = false;
	if (OpenScan(&FileScan, rm_column, 0, NULL)!= SUCCESS)return SQL_SYNTAX;
	//根据之前读取的系统表中信息，读取属性信息，结果保存在ctmp中
	Column = (column *)malloc(attrcount*sizeof(column));
	cupdate = (column *)malloc(sizeof(column));
	ctmp = Column;
	while (GetNextRec(&FileScan, &reccol) == SUCCESS){
		if (strcmp(relName, reccol.pData) == 0){//找到表名为relName的第一个记录，依次读取attrcount个记录
			for (int i = 0; i < attrcount; i++,ctmp++){
				memcpy(ctmp->tablename, reccol.pData, 21);
				memcpy(ctmp->attrname, reccol.pData + 21, 21);
				memcpy(&(ctmp->attrtype), reccol.pData + 42, sizeof(AttrType));
				memcpy(&(ctmp->attrlength), reccol.pData + 42 + sizeof(AttrType), sizeof(int));
				memcpy(&(ctmp->attroffset), reccol.pData + 42 + sizeof(int)+sizeof(AttrType), sizeof(int));
				if ((strcmp(relName,ctmp->tablename) == 0) && (strcmp(attrName,ctmp->attrname) == 0)){
					cupdate = ctmp;//找到要更新数据 对应的属性
				}
				if (GetNextRec(&FileScan, &reccol)!= SUCCESS)break;
			}
			break;
		}
	}
	//关闭系统列文件扫描
	if(CloseScan(&FileScan)!=SUCCESS)return SQL_SYNTAX;
	//打开记录文件扫描
	FileScan.bOpen = false;
	if (OpenScan(&FileScan, rm_data, 0, NULL)!= SUCCESS)return SQL_SYNTAX;
	//循环查找表名为relName对应的数据表中的记录,并将记录信息保存于recdata中
	while (GetNextRec(&FileScan, &recdata) == SUCCESS){
		for (i = 0, torf = 1, contmp = conditions; i < nConditions; i++, contmp++){//conditions条件逐一判断
			ctmpleft = ctmpright = Column;//每次循环都要将遍历整个系统列文件，找到各个条件对应的属性
			//左属性右值
			if (contmp->bLhsIsAttr == 1 && contmp->bRhsIsAttr == 0){
				for (int j = 0; j < attrcount; j++){//attrcount个属性逐一判断
					if (contmp->lhsAttr.relName == NULL){//当条件中未指定表名时，默认为relName
						contmp->lhsAttr.relName = (char *)malloc(21);
						strcpy(contmp->lhsAttr.relName, relName);
					}
					if ((strcmp(ctmpleft->tablename, contmp->lhsAttr.relName) == 0)
						&& (strcmp(ctmpleft->attrname, contmp->lhsAttr.attrName) == 0)){//根据表名属性名找到对应属性
						break;
					}
					ctmpleft++;
				}
				//对conditions的某一个条件进行判断
				if (ctmpleft->attrtype == ints){//判定属性的类型
					attrtype = ints;
					memcpy(&intleft, recdata.pData + ctmpleft->attroffset, sizeof(int));
					memcpy(&intright, contmp->rhsValue.data, sizeof(int));
				}
				else if (ctmpleft->attrtype == chars){
					attrtype = chars;
					charleft = (char *)malloc(ctmpleft->attrlength);
					memcpy(charleft, recdata.pData + ctmpleft->attroffset, ctmpleft->attrlength);
					charright = (char *)malloc(ctmpleft->attrlength);
					memcpy(charright, contmp->rhsValue.data, ctmpleft->attrlength);
				}
				else if (ctmpleft->attrtype == floats){
					attrtype = floats;
					memcpy(&floatleft, recdata.pData + ctmpleft->attroffset, sizeof(float));
					memcpy(&floatright, contmp->rhsValue.data, sizeof(float));
				}
				else
					torf &= 0;
			}
			//右属性左值
			else  if (contmp->bLhsIsAttr == 0 && contmp->bRhsIsAttr == 1){
				for (int j = 0; j < attrcount; j++){//attrcount个属性逐一判断
					if (contmp->rhsAttr.relName == NULL){//当条件中未指定表名时，默认为relName
						contmp->rhsAttr.relName = (char *)malloc(21);
						strcpy(contmp->rhsAttr.relName, relName);
					}
					if ((strcmp(ctmpright->tablename, contmp->rhsAttr.relName) == 0)
						&& (strcmp(ctmpright->attrname, contmp->rhsAttr.attrName) == 0)){//根据表名属性名找到对应属性
						break;
					}
					ctmpright++;
				}
				//对conditions的某一个条件进行判断
				if (ctmpright->attrtype == ints){//判定属性的类型
					attrtype = ints;
					memcpy(&intleft, contmp->lhsValue.data, sizeof(int));
					memcpy(&intright, recdata.pData + ctmpright->attroffset, sizeof(int));
				}
				else if (ctmpright->attrtype == chars){
					attrtype = chars;
					charleft = (char *)malloc(ctmpright->attrlength);
					memcpy(charleft, contmp->lhsValue.data, ctmpright->attrlength);
					charright = (char *)malloc(ctmpright->attrlength);
					memcpy(charright, recdata.pData + ctmpright->attroffset, ctmpright->attrlength);
				}
				else if (ctmpright->attrtype == floats){
					attrtype = floats;
					memcpy(&floatleft, contmp->lhsValue.data, sizeof(float));
					memcpy(&floatright, recdata.pData + ctmpright->attroffset, sizeof(float));
				}
				else
					torf &= 0;
			}
			//左右均属性
			else  if (contmp->bLhsIsAttr == 1 && contmp->bRhsIsAttr == 1){
				for (int j = 0; j < attrcount; j++){//attrcount个属性逐一判断
					if (contmp->lhsAttr.relName == NULL){//当条件中未指定表名时，默认为relName
						contmp->lhsAttr.relName = (char *)malloc(21);
						strcpy(contmp->lhsAttr.relName, relName);
					}
					if ((strcmp(ctmpleft->tablename, contmp->lhsAttr.relName) == 0)
						&& (strcmp(ctmpleft->attrname, contmp->lhsAttr.attrName) == 0)){//根据表名属性名找到对应属性
						break;
					}
					ctmpleft++;
				}
				for (int j = 0; j < attrcount; j++){//attrcount个属性逐一判断
					if (contmp->rhsAttr.relName == NULL){//当条件中未指定表名时，默认为relName
						contmp->rhsAttr.relName = (char *)malloc(21);
						strcpy(contmp->rhsAttr.relName, relName);
					}
					if ((strcmp(ctmpright->tablename, contmp->rhsAttr.relName) == 0)
						&& (strcmp(ctmpright->attrname, contmp->rhsAttr.attrName) == 0)){//根据表名属性名找到对应属性
						break;
					}
					ctmpright++;
				}
				//对conditions的某一个条件进行判断
				if (ctmpright->attrtype == ints && ctmpleft->attrtype == ints){//判定属性的类型
					attrtype = ints;
					memcpy(&intleft, recdata.pData + ctmpleft->attroffset, sizeof(int));
					memcpy(&intright, recdata.pData + ctmpright->attroffset, sizeof(int));
				}
				else if (ctmpright->attrtype == chars &&ctmpleft->attrtype == chars){
					attrtype = chars;
					charleft = (char *)malloc(ctmpright->attrlength);
					memcpy(charleft, recdata.pData + ctmpleft->attroffset, ctmpright->attrlength);
					charright = (char *)malloc(ctmpright->attrlength);
					memcpy(charright, recdata.pData + ctmpright->attroffset, ctmpright->attrlength);
				}
				else if (ctmpright->attrtype == floats &&ctmpleft->attrtype == floats){
					attrtype = floats;
					memcpy(&floatleft, recdata.pData + ctmpleft->attroffset, sizeof(float));
					memcpy(&floatright, recdata.pData + ctmpright->attroffset, sizeof(float));
				}
				else
					torf &= 0;
			}
			if (attrtype == ints){
				if ((intleft == intright && contmp->op == EQual) ||
					(intleft > intright && contmp->op == GreatT) ||
					(intleft >= intright && contmp->op == GEqual) ||
					(intleft < intright && contmp->op == LessT) ||
					(intleft <= intright && contmp->op == LEqual) ||
					(intleft != intright && contmp->op == NEqual))
					torf &= 1;
				else
					torf &= 0;
			}
			else if (attrtype == chars){
				if ((strcmp(charleft, charright) == 0 && contmp->op == EQual) ||
					(strcmp(charleft, charright) > 0 && contmp->op == GreatT) ||
					((strcmp(charleft, charright) > 0 || strcmp(charleft, charright) == 0) && contmp->op == GEqual) ||
					(strcmp(charleft, charright) < 0 && contmp->op == LessT) ||
					((strcmp(charleft, charright) < 0 || strcmp(charleft, charright) == 0) && contmp->op == LEqual) ||
					(strcmp(charleft, charright) != 0 && contmp->op == NEqual))
					torf &= 1;
				else
					torf &= 0;
				free(charleft);
				free(charright);
			}
			else if (attrtype == floats){
				if ((floatleft == floatright && contmp->op == EQual) ||
					(floatleft > floatright && contmp->op == GreatT) ||
					(floatleft >= floatright && contmp->op == GEqual) ||
					(floatleft < floatright && contmp->op == LessT) ||
					(floatleft <= floatright && contmp->op == LEqual) ||
					(floatleft != floatright && contmp->op == NEqual))
					torf &= 1;
				else
					torf &= 0;
			}
			else
				torf &= 0;
		}
		if (torf == 1){
			memcpy(recdata.pData + cupdate->attroffset,value->data,cupdate->attrlength);
			UpdateRec(rm_data, &recdata);
		}
	}
	free(Column);
/*	//打开系统列文件扫描
	FileScan.bOpen = false;
	if (OpenScan(&FileScan, rm_column, 0, NULL)!= SUCCESS){
		AfxMessageBox("系统列文件扫描失败");
		return SQL_SYNTAX;
	}
	//扫描系统列文件，如果该属性上存在索引，则删除原有索引重新创建
	while (GetNextRec(&FileScan, &reccol) == SUCCESS){
		if (strcmp(relName, reccol.pData) == 0){//找到表名为relName的第一个记录，依次读取attrcount个记录
			for (int i = 0; i < attrcount; i++){
				if((reccol.pData+42+3*sizeof(int))=="1"){
					memcpy(attr,reccol.pData+21,21);
					memcpy(index,reccol.pData+43+2*sizeof(int)+sizeof(AttrType),21);
					memcpy(reccol.pData+42+3*sizeof(int),"0",1);//索引标记项改为0
			        if (UpdateRec(rm_column,&reccol)!=SUCCESS)return SQL_SYNTAX;
					tmp.Remove((LPCTSTR)index);//删除索引文件
					CreateIndex(index,relName,attr);
				}
			}
			break;
		}
	}
	//关闭系统列文件扫描
	if(CloseScan(&FileScan)!=SUCCESS)return SQL_SYNTAX;*/
	//关闭记录文件扫描
	if(CloseScan(&FileScan)!=SUCCESS)return SQL_SYNTAX;
	//关闭文件
	if (RM_CloseFile(rm_table)!= SUCCESS)return SQL_SYNTAX;
	free(rm_table);
    if (RM_CloseFile(rm_column)!= SUCCESS)return SQL_SYNTAX;
	free(rm_column);
	if (RM_CloseFile(rm_data)!= SUCCESS)return SQL_SYNTAX;
	free(rm_data);
	return SUCCESS;	
}

bool CanButtonClick(){//需要重新实现
	//如果当前有数据库已经打开
	return true;
	//如果当前没有数据库打开
	//return false;
}
