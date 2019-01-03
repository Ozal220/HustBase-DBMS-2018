#include "stdafx.h"
#include "EditArea.h"
#include "SYS_Manager.h"
#include "QU_Manager.h"
#include <iostream>
#include <fstream>
#include<vector>
//����һ��vector����������*filehandle
std::vector<RM_FileHandle *> vec;
struct table{
		char tablename[21];//����
		int attrcount;//��������
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
void ExecuteAndMessage(char * sql,CEditArea* editArea){//����ִ�е���������ڽ�������ʾִ�н����
	std::string s_sql = sql;
	RC rc;
	if(s_sql.find("select") == 0){//�ǲ�ѯ�����ִ�����£���������
		SelResult res;
		Init_Result(&res);
		rc = Query(sql,&res);
		if(rc!=SUCCESS)return;
		int col_num = res.col_num;//��
		int row_num = 0;//��
		SelResult *tmp=&res;
		while(tmp){//���нڵ�ļ�¼��֮��
			row_num+=tmp->row_num;
			tmp=tmp->next_res;
		}
		char ** fields = new char *[20];//���ֶ�����
		for(int i = 0;i<col_num;i++){
			fields[i] = new char[20];
			memset(fields[i],'\0',20);
			memcpy(fields[i],res.fields[i],20);
		}
		tmp=&res;
		char *** rows = new char**[row_num];//�����
		for(int i = 0;i<row_num;i++){
			rows[i] = new char*[col_num];//���һ����¼
			for (int j = 0; j <col_num; j++)
			{
				rows[i][j] = new char[20];//һ����¼��һ���ֶ�
				memset(rows[i][j], '\0', 20);
				memcpy(rows[i][j],tmp->res[i][j],20);
			}
			if (i==99)tmp=tmp->next_res;//ÿ������ڵ�����¼100����¼
		}
		editArea->ShowSelResult(col_num,row_num,fields,rows);
		for(int i = 0;i<20;i++){
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
	SelResult *res;
	if (rc == SUCCESS)
	{
		int i = 0;
		switch (sql_str->flag)
		{
			case 1:
			////�ж�SQL���Ϊselect���
			Select (sql_str->sstr.sel.nSelAttrs,sql_str->sstr.sel.selAttrs,sql_str->sstr.sel.nRelations,sql_str->sstr.sel.relations,sql_str->sstr.sel.nConditions,sql_str->sstr.sel.conditions,res);
			break;

			case 2:
			//�ж�SQL���Ϊinsert���
				Insert(sql_str->sstr.ins.relName,sql_str->sstr.ins.nValues,sql_str->sstr.ins.values);
			break;

			case 3:	
			//�ж�SQL���Ϊupdate���
				Update(sql_str->sstr.upd.relName,sql_str->sstr.upd.attrName,&sql_str->sstr.upd.value,sql_str->sstr.upd.nConditions,sql_str->sstr.upd.conditions);
			break;

			case 4:					
			//�ж�SQL���Ϊdelete���
				Delete(sql_str->sstr.del.relName,sql_str->sstr.del.nConditions,sql_str->sstr.del.conditions);
			break;

			case 5:
			//�ж�SQL���ΪcreateTable���
				CreateTable(sql_str->sstr.cret.relName,sql_str->sstr.cret.attrCount,sql_str->sstr.cret.attributes);
			break;

			case 6:	
			//�ж�SQL���ΪdropTable���
				DropTable(sql_str->sstr.drt.relName);
			break;

			case 7:
			//�ж�SQL���ΪcreateIndex���
				CreateIndex(sql_str->sstr.crei.indexName,sql_str->sstr.crei.relName,sql_str->sstr.crei.attrName);
			break;
	
			case 8:	
			//�ж�SQL���ΪdropIndex���
				DropIndex(sql_str->sstr.dri.indexName);
			break;
			
			case 9:
			//�ж�Ϊhelp��䣬���Ը���������ʾ
			break;
		
			case 10: 
			//�ж�Ϊexit��䣬�����ɴ˽����˳�����
				AfxGetMainWnd()->SendMessage(WM_CLOSE);
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
	if(SetCurrentDirectory(dbname))return SUCCESS;//����ָ��·��Ϊ����·��
        else return SQL_SYNTAX;
}


RC CloseDB(){	
	for(std::vector<RM_FileHandle *>::size_type i = 0; i < vec.size(); i++){//����vector�еľ���ر�ÿһ���򿪵ļ�¼�ļ�
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
	RM_FileScan FileScan;
	RM_Record rectab, reccol;
	char index[21];
	tmp.Remove((LPCTSTR)relName);//ɾ�����ݱ��ļ�(��¼�ļ�)
	rm_table = (RM_FileHandle *)malloc(sizeof(RM_FileHandle));//��ϵͳ���ļ�
	rm_table->bOpen = false;
	if(RM_OpenFile("SYSTABLES", rm_table)!= SUCCESS)return SQL_SYNTAX;
	rm_column = (RM_FileHandle *)malloc(sizeof(RM_FileHandle));//��ϵͳ���ļ�
	rm_column->bOpen = false;
	if (RM_OpenFile("SYSCOLUMNS", rm_column)!= SUCCESS)return SQL_SYNTAX;
	FileScan.bOpen = false;//��ϵͳ���ļ�����ɨ�裬ɾ��ͬ����ļ�¼��
	if (OpenScan(&FileScan, rm_table,0,NULL)!= SUCCESS)return SQL_SYNTAX;
	while(GetNextRec(&FileScan, &rectab)==SUCCESS){
		memcpy(tab.tablename,rectab.pData,21);
		if (strcmp(relName,tab.tablename)==0){//����������ɾ������
			DeleteRec(rm_table,&(rectab.rid));
			break;//��Ϊֻ��һ�����ֱ������
		}
	}
	if(CloseScan(&FileScan)!=SUCCESS)return SQL_SYNTAX;
	FileScan.bOpen = false;//��ϵͳ���ļ�����ɨ�裬ɾ��ͬ����ļ�¼��
	if (OpenScan(&FileScan, rm_column,0,NULL) != SUCCESS)return SQL_SYNTAX;
	while(GetNextRec(&FileScan, &reccol) == SUCCESS){
		memcpy(col.tablename,reccol.pData,21);
		if (strcmp(relName,col.tablename) == 0){//����������ɾ�������Ӧ�������ļ���֮����ɾ�������¼
			memcpy(index,reccol.pData+43+3*sizeof(int),21);
			if((reccol.pData+42+3*sizeof(int))=="1")tmp.Remove((LPCTSTR)index);	//ɾ�������ļ�
			DeleteRec(rm_column, &(reccol.rid));//��Ϊ������ж���У�����Ҫ����
		}
	}
	if(CloseScan(&FileScan)!=SUCCESS)return SQL_SYNTAX;
	if (RM_CloseFile(rm_table)!=SUCCESS)return SQL_SYNTAX;//�ر��ļ����
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
	rm_column = (RM_FileHandle *)malloc(sizeof(RM_FileHandle));//��ϵͳ���ļ�
	rm_column->bOpen = false;
	if (RM_OpenFile("SYSCOLUMNS", rm_column)!=SUCCESS){
		AfxMessageBox("ϵͳ���ļ���ʧ��");
		return SQL_SYNTAX;
	}
	FileScan.bOpen = false;//��ϵͳ���ļ�����ɨ�裬�޸�ͬ�������ļ�¼�ix_flag��0��
	if (OpenScan(&FileScan, rm_column, 0, NULL)!=SUCCESS){
		AfxMessageBox("ϵͳ���ļ�ɨ��ʧ��");
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
		if (strcmp(relName,col.tablename)==0&&strcmp(attrName,col.attrname)==0){//���������������,�ҵ�����
			if((reccol.pData+42+3*sizeof(int))=="1")return SQL_SYNTAX;//���������򱨴�
			else{//��������
				if(CloseScan(&FileScan)!=SUCCESS)return SQL_SYNTAX;
				CreateIndex(indexName,col.attrtype,col.attrlength);//���������ļ�
				memcpy(reccol.pData+42+3*sizeof(int),"1",1);//����ϵͳ���ļ���Ӧ���ix_flag��indexName
				memcpy(reccol.pData+43+3*sizeof(int),indexName,21);
				UpdateRec(rm_column,&reccol);
				rm_index = (IX_IndexHandle *)malloc(sizeof(IX_IndexHandle));//�������ļ�
	            rm_index->bOpen = false;
	            if(OpenIndex(indexName, rm_index)!=SUCCESS){
		            AfxMessageBox("�����ļ���ʧ��");
		            return SQL_SYNTAX;
	            }
				rm_data = (RM_FileHandle *)malloc(sizeof(RM_FileHandle));//�򿪱�ļ�¼�ļ�
	            rm_data->bOpen = false;
	            if (RM_OpenFile(relName, rm_data)!=SUCCESS){
		           AfxMessageBox("��¼�ļ���ʧ��");
		           return SQL_SYNTAX;
	            }
				FileScan.bOpen = false;//�򿪱�ļ�¼�ļ�����ɨ��
	            if (OpenScan(&FileScan, rm_data,0,NULL) != SUCCESS){
					AfxMessageBox("��¼�ļ�ɨ��ʧ��");
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
				break;//ֻ��һ����ϣ�������������
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
	rm_column = (RM_FileHandle *)malloc(sizeof(RM_FileHandle));//��ϵͳ���ļ�
	rm_column->bOpen = false;
	if (RM_OpenFile("SYSCOLUMNS", rm_column)!=SUCCESS){
		AfxMessageBox("ϵͳ���ļ���ʧ��");
		return SQL_SYNTAX;
	}
	FileScan.bOpen = false;//��ϵͳ���ļ�����ɨ�裬�޸�ͬ�������ļ�¼�flag��0��
	if (OpenScan(&FileScan, rm_column, 0, NULL)!=SUCCESS){
		AfxMessageBox("ϵͳ���ļ�ɨ��ʧ��");
		return SQL_SYNTAX;
	}
	while (GetNextRec(&FileScan, &reccol) == SUCCESS){
		memcpy(index,reccol.pData+43+3*sizeof(int),21);
		if (strcmp(indexName,index)==0){//���������,�ҵ�����
			if((reccol.pData+42+3*sizeof(int))=="0")return SQL_SYNTAX;//�����������򱨴�
			else{
				memcpy(reccol.pData+42+3*sizeof(int),"0",1);//��������λ��0��ɾ�������ļ�
			    if (UpdateRec(rm_column,&reccol)!=SUCCESS)return SQL_SYNTAX;
				tmp.Remove((LPCTSTR)indexName);//ɾ�������ļ�
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
	char *value;//��ȡ���ݱ���Ϣ
	RID *rid;
	column *Column, *ctmp;//���ڴ洢һ������������Ե�ֵ
	RM_FileScan FileScan;
	RM_Record rectab, reccol;
	int attrcount;//��������
	char index[21],attr[21];
	//�����ݱ�,ϵͳ��ϵͳ���ļ�
	rm_data = (RM_FileHandle *)malloc(sizeof(RM_FileHandle));
	rm_data->bOpen = false;
	if (RM_OpenFile(relName, rm_data)!= SUCCESS){
		AfxMessageBox("��¼�ļ���ʧ��");
		return SQL_SYNTAX;
	}
	rm_table = (RM_FileHandle *)malloc(sizeof(RM_FileHandle));
	rm_table->bOpen = false;
	if (RM_OpenFile("SYSTABLES", rm_table)!= SUCCESS){
		AfxMessageBox("ϵͳ���ļ���ʧ��");
		return SQL_SYNTAX;
	}
	rm_column = (RM_FileHandle *)malloc(sizeof(RM_FileHandle));
	rm_column->bOpen = false;
	if (RM_OpenFile("SYSCOLUMNS", rm_column)!= SUCCESS){
		AfxMessageBox("ϵͳ���ļ���ʧ��");
		return SQL_SYNTAX;
	}
	//��ϵͳ���ļ�����ɨ��
	FileScan.bOpen = false;
	if (OpenScan(&FileScan, rm_table, 0, NULL)!= SUCCESS){
		AfxMessageBox("ϵͳ���ļ�ɨ��ʧ��");
		return SQL_SYNTAX;
	}
	//ѭ�����ұ���ΪrelName��Ӧ��ϵͳ���еļ�¼,��ȡ��������
	while (GetNextRec(&FileScan, &rectab) == SUCCESS){
		if (strcmp(relName, rectab.pData) == 0){
			memcpy(&attrcount, rectab.pData + 21, sizeof(int));
			break;
		}	
	}
	//�ر�ϵͳ���ļ�ɨ��
	if(CloseScan(&FileScan)!=SUCCESS)return SQL_SYNTAX;
	//�ж����������Ƿ����
	if (attrcount != nValues){
		AfxMessageBox("���Ը������ȣ�����ʧ�ܣ�");
		return SQL_SYNTAX;
	}
	//��ϵͳ���ļ�ɨ��
	FileScan.bOpen = false;
	if (OpenScan(&FileScan, rm_column, 0, NULL)!= SUCCESS){
		AfxMessageBox("ϵͳ���ļ�ɨ��ʧ��");
		return SQL_SYNTAX;
	}
	//����֮ǰ��ȡ��ϵͳ������Ϣ����ȡ������Ϣ�����������ctmp��
	Column = (column *)malloc(attrcount*sizeof(column));
	ctmp = Column;
	while (GetNextRec(&FileScan, &reccol) == SUCCESS){
		if (strcmp(relName, reccol.pData) == 0){//�ҵ�����ΪrelName�ĵ�һ����¼�����ζ�ȡattrcount����¼
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
	//���¼�ļ���ѭ�������¼
	value = (char *)malloc(rm_data->recSize);
	values = values + nValues -1;
	for (int i = 0; i < nValues; i++, values--,ctmp++){
		memcpy(value + ctmp->attroffset, values->data, ctmp->attrlength);
	}
	rid = (RID*)malloc(sizeof(RID));
	rid->bValid = false;
	InsertRec(rm_data, value, rid);
	//�ر�ϵͳ���ļ�ɨ��
	if(CloseScan(&FileScan)!=SUCCESS)return SQL_SYNTAX;
	//��ϵͳ���ļ�ɨ��
	FileScan.bOpen = false;
	if (OpenScan(&FileScan, rm_column, 0, NULL)!= SUCCESS){
		AfxMessageBox("ϵͳ���ļ�ɨ��ʧ��");
		return SQL_SYNTAX;
	}
	//ɨ��ϵͳ���ļ�������������ϴ��������������������
	while (GetNextRec(&FileScan, &reccol) == SUCCESS){
		if (strcmp(relName, reccol.pData) == 0){//�ҵ�����ΪrelName�ĵ�һ����¼�����ζ�ȡattrcount����¼
			for (int i = 0; i < attrcount; i++){
				if((reccol.pData+42+3*sizeof(int))=="1"){//ix_flagΪ1���������ϴ���������������µ�������
					IX_IndexHandle *rm_index;
					memcpy(index,reccol.pData+43+2*sizeof(int)+sizeof(AttrType),21);
					rm_index = (IX_IndexHandle *)malloc(sizeof(IX_IndexHandle));//�������ļ�
	                rm_index->bOpen = false;
	                if(OpenIndex(index, rm_index)!=SUCCESS){
		               AfxMessageBox("�����ļ���ʧ��");
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
/*	//�ر�ϵͳ���ļ�ɨ��
	if(CloseScan(&FileScan)!=SUCCESS)return SQL_SYNTAX;
	//��ϵͳ���ļ�ɨ��
	FileScan.bOpen = false;
	if (OpenScan(&FileScan, rm_column, 0, NULL)!= SUCCESS){
		AfxMessageBox("ϵͳ���ļ�ɨ��ʧ��");
		return SQL_SYNTAX;
	}
	//ɨ��ϵͳ���ļ�������������ϴ�����������ɾ��ԭ���������´���
	while (GetNextRec(&FileScan, &reccol) == SUCCESS){
		if (strcmp(relName, reccol.pData) == 0){//�ҵ�����ΪrelName�ĵ�һ����¼�����ζ�ȡattrcount����¼
			for (int i = 0; i < attrcount; i++){
				if((reccol.pData+42+3*sizeof(int))=="1"){
					memcpy(attr,reccol.pData+21,21);
					memcpy(index,reccol.pData+43+2*sizeof(int)+sizeof(AttrType),21);
					memcpy(reccol.pData+42+3*sizeof(int),"0",1);//����������Ϊ0
			        if (UpdateRec(rm_column,&reccol)!=SUCCESS)return SQL_SYNTAX;
					tmp.Remove((LPCTSTR)index);//ɾ�������ļ�
					CreateIndex(index,relName,attr);
				}
			}
			break;
		}
	}*/
	//�ر�ϵͳ���ļ�ɨ��
	if(CloseScan(&FileScan)!=SUCCESS)return SQL_SYNTAX;
	//�ر��ļ�
	if (RM_CloseFile(rm_data)!= SUCCESS){
		AfxMessageBox("��¼�ļ��ر�ʧ��");
		return SQL_SYNTAX;
	}
	free(rm_data);
	if (RM_CloseFile(rm_table)!= SUCCESS){
		AfxMessageBox("ϵͳ���ļ��ر�ʧ��");
		return SQL_SYNTAX;
	}
	free(rm_table);
	if (RM_CloseFile(rm_column)!= SUCCESS){
		AfxMessageBox("ϵͳ���ļ��ر�ʧ��");
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
	int i,torf;//�Ƿ����ɾ������
	int attrcount;//��������
	int intleft,intright; 
	char *charleft,*charright;
	float floatleft,floatright;//���Ե�ֵ
	AttrType attrtype;
	char index[21],attr[21];
	//�򿪼�¼,ϵͳ��ϵͳ���ļ�
	rm_data = (RM_FileHandle *)malloc(sizeof(RM_FileHandle));
	rm_data->bOpen = false;
	if (RM_OpenFile(relName, rm_data)!= SUCCESS)return SQL_SYNTAX;
	rm_table = (RM_FileHandle *)malloc(sizeof(RM_FileHandle));
	rm_table->bOpen = false;
	if (RM_OpenFile("SYSTABLES", rm_table)!= SUCCESS)return SQL_SYNTAX;
	rm_column = (RM_FileHandle *)malloc(sizeof(RM_FileHandle));
	rm_column->bOpen = false;
	if (RM_OpenFile("SYSCOLUMNS", rm_column)!= SUCCESS)return SQL_SYNTAX;

	//��ϵͳ���ļ�ɨ��
	FileScan.bOpen = false;
	if (OpenScan(&FileScan, rm_table, 0, NULL)!= SUCCESS)return SQL_SYNTAX;
	//ѭ�����ұ���ΪrelName��Ӧ��ϵͳ���еļ�¼,��¼��������attrcount
	while (GetNextRec(&FileScan, &rectab) == SUCCESS){
		if (strcmp(relName, rectab.pData) == 0){
			memcpy(&attrcount, rectab.pData + 21, sizeof(int));
			break;
		}
	}
	//�ر�ϵͳ���ļ�ɨ��
	if(CloseScan(&FileScan)!=SUCCESS)return SQL_SYNTAX;
	//��ϵͳ���ļ�ɨ��
	FileScan.bOpen = false;
	if (OpenScan(&FileScan, rm_column, 0, NULL)!= SUCCESS)return SQL_SYNTAX;
	//����֮ǰ��ȡ��ϵͳ������Ϣ����ȡ������Ϣ�����������ctmp��
	Column = (column *)malloc(attrcount*sizeof(column));
	ctmp = Column;
	while (GetNextRec(&FileScan, &reccol) == SUCCESS){
		if (strcmp(relName, reccol.pData) == 0){//�ҵ�����ΪrelName�ĵ�һ����¼�����ζ�ȡattrcount����¼
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
	//�ر�ϵͳ���ļ�ɨ��
	if(CloseScan(&FileScan)!=SUCCESS)return SQL_SYNTAX;
	//�򿪼�¼�ļ�ɨ��
	FileScan.bOpen = false;
	if (OpenScan(&FileScan, rm_data, 0, NULL)!= SUCCESS)return SQL_SYNTAX;
	//ѭ�����ұ���ΪrelName��Ӧ�����ݱ��еļ�¼,������¼��Ϣ������recdata��
	while (GetNextRec(&FileScan, &recdata) == SUCCESS){	//ȡ��¼���ж�
		for (i = 0, torf = 1,contmp = conditions;i < nConditions; i++, contmp++){//conditions������һ�ж�
			ctmpleft = ctmpright = Column;//ÿ��ѭ����Ҫ����������ϵͳ���ļ����ҵ�����������Ӧ������
			//��������ֵ
			if (contmp->bLhsIsAttr == 1 && contmp->bRhsIsAttr == 0){
				for (int j = 0; j < attrcount; j++){//attrcount��������һ�ж�
					if (contmp->lhsAttr.relName == NULL){//��������δָ������ʱ��Ĭ��ΪrelName
						contmp->lhsAttr.relName = (char *)malloc(21);
						strcpy(contmp->lhsAttr.relName, relName);
					}
					if ((strcmp(ctmpleft->tablename, contmp->lhsAttr.relName) == 0)
						&& (strcmp(ctmpleft->attrname, contmp->lhsAttr.attrName) == 0)){//���ݱ����������ҵ���Ӧ����
						break;
					}
					ctmpleft++;
				}
				//��conditions��ĳһ�����������ж�
				switch (ctmpleft->attrtype){//�ж����Ե�����
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
			//��������ֵ
			if (contmp->bLhsIsAttr == 0 && contmp->bRhsIsAttr == 1){
				for (int j = 0; j < attrcount; j++){//attrcount��������һ�ж�
					if (contmp->rhsAttr.relName == NULL){//��������δָ������ʱ��Ĭ��ΪrelName
						contmp->rhsAttr.relName = (char *)malloc(21);
						strcpy(contmp->rhsAttr.relName, relName);
					}
					if ((strcmp(ctmpright->tablename, contmp->rhsAttr.relName) == 0)
						&& (strcmp(ctmpright->attrname, contmp->rhsAttr.attrName) == 0)){//���ݱ����������ҵ���Ӧ����
						break;
					}
					ctmpright++;
				}
				//��conditions��ĳһ�����������ж�
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
			//���Ҿ�����
			else  if (contmp->bLhsIsAttr == 1 && contmp->bRhsIsAttr == 1){
				for (int j = 0; j < attrcount; j++){//attrcount��������һ�ж�
					if (contmp->lhsAttr.relName == NULL){//��������δָ������ʱ��Ĭ��ΪrelName
						contmp->lhsAttr.relName = (char *)malloc(21);
						strcpy(contmp->lhsAttr.relName, relName);
					}
					if ((strcmp(ctmpleft->tablename, contmp->lhsAttr.relName) == 0)
						&& (strcmp(ctmpleft->attrname, contmp->lhsAttr.attrName) == 0)){//���ݱ����������ҵ���Ӧ����
						break;
					}
					ctmpleft++;
				}
				for (int j = 0; j < attrcount; j++){//attrcount��������һ�ж�
					if (contmp->rhsAttr.relName == NULL){//��������δָ������ʱ��Ĭ��ΪrelName
						contmp->rhsAttr.relName = (char *)malloc(21);
						strcpy(contmp->rhsAttr.relName, relName);
					}
					if ((strcmp(ctmpright->tablename, contmp->rhsAttr.relName) == 0)
						&& (strcmp(ctmpright->attrname, contmp->rhsAttr.attrName) == 0)){//���ݱ����������ҵ���Ӧ����
						break;
					}
					ctmpright++;
				}
				//��conditions��ĳһ�����������ж�
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
			//��ϵͳ���ļ�ɨ��
			FileScan.bOpen = false;
			if (OpenScan(&FileScan, rm_column, 0, NULL)!= SUCCESS){
				AfxMessageBox("ϵͳ���ļ�ɨ��ʧ��");
				return SQL_SYNTAX;
			}
			//ɨ��ϵͳ���ļ�������������ϴ�����������ɾ��������
			while (GetNextRec(&FileScan, &reccol) == SUCCESS){
				if (strcmp(relName, reccol.pData) == 0){//�ҵ�����ΪrelName�ĵ�һ����¼�����ζ�ȡattrcount����¼
					for (int i = 0; i < attrcount; i++){
						if((reccol.pData+42+3*sizeof(int))=="1"){//ix_flagΪ1���������ϴ�����������ɾ��ԭ�е�������
							IX_IndexHandle *rm_index;
							memcpy(index,reccol.pData+43+2*sizeof(int)+sizeof(AttrType),21);
							rm_index = (IX_IndexHandle *)malloc(sizeof(IX_IndexHandle));//�������ļ�
			                rm_index->bOpen = false;
			                if(OpenIndex(index, rm_index)!=SUCCESS){
				               AfxMessageBox("�����ļ���ʧ��");
				               return SQL_SYNTAX;
			                }
							char*length,*offset;
							memcpy(length,reccol.pData+42+sizeof(int),sizeof(int));
				            memcpy(offset,reccol.pData+42+2*sizeof(int),sizeof(int));
							char *data = (char *)malloc((int)length);
							memcpy(data,recdata.pData+(int)offset,(int)length);
				            DeleteEntry(rm_index,data,&(recdata.rid));
							if(CloseIndex(rm_index)!=SUCCESS)return SQL_SYNTAX;//�ر������ļ�
							free(rm_index);
						}
					}
					break;
				}
			}
		}	
	}
	free(Column);
	//�رռ�¼�ļ�ɨ��
	if(CloseScan(&FileScan)!=SUCCESS)return SQL_SYNTAX;
/*	//��ϵͳ���ļ�ɨ��
	FileScan.bOpen = false;
	if (OpenScan(&FileScan, rm_column, 0, NULL)!= SUCCESS){
		AfxMessageBox("ϵͳ���ļ�ɨ��ʧ��");
		return SQL_SYNTAX;
	}
	//ɨ��ϵͳ���ļ�������������ϴ�����������ɾ��ԭ���������´���
	while (GetNextRec(&FileScan, &reccol) == SUCCESS){
		if (strcmp(relName, reccol.pData) == 0){//�ҵ�����ΪrelName�ĵ�һ����¼�����ζ�ȡattrcount����¼
			for (int i = 0; i < attrcount; i++){
				if((reccol.pData+42+3*sizeof(int))=="1"){
					memcpy(attr,reccol.pData+21,21);
					memcpy(index,reccol.pData+43+2*sizeof(int)+sizeof(AttrType),21);
					memcpy(reccol.pData+42+3*sizeof(int),"0",1);//����������Ϊ0
			        if (UpdateRec(rm_column,&reccol)!=SUCCESS)return SQL_SYNTAX;
					tmp.Remove((LPCTSTR)index);//ɾ�������ļ�
					CreateIndex(index,relName,attr);
				}
			}
			break;
		}
	}
	//�ر�ϵͳ���ļ�ɨ��
	if(CloseScan(&FileScan)!=SUCCESS)return SQL_SYNTAX;*/
	//�ر��ļ�
	if (RM_CloseFile(rm_table)!= SUCCESS)return SQL_SYNTAX;
	free(rm_table);
	if (RM_CloseFile(rm_column)!= SUCCESS)return SQL_SYNTAX;
	free(rm_column);
	if (RM_CloseFile(rm_data)!= SUCCESS)return SQL_SYNTAX;
	free(rm_data);
	return SUCCESS;
}

RC Update(char *relName,char *attrName,Value *value,int nConditions,Condition *conditions){//ֻ�ܽ��е�ֵ����
	RM_FileHandle *rm_data, *rm_table, *rm_column;
	RM_FileScan FileScan;
	RM_Record recdata, rectab, reccol;
	column *Column, *ctmp,*cupdate,*ctmpleft,*ctmpright;
	Condition *contmp;
	int i, torf;//�Ƿ����ɾ������
	int attrcount;//��������
	int intleft,intright;
	char *charleft,*charright;
	float floatleft,floatright;//���Ե�ֵ
	AttrType attrtype;
	//�򿪼�¼,ϵͳ��ϵͳ���ļ�
	rm_data = (RM_FileHandle *)malloc(sizeof(RM_FileHandle));
	rm_data->bOpen = false;
	if (RM_OpenFile(relName, rm_data)!= SUCCESS)return SQL_SYNTAX;
	rm_table = (RM_FileHandle *)malloc(sizeof(RM_FileHandle));
	rm_table->bOpen = false;
	if (RM_OpenFile("SYSTABLES", rm_table)!= SUCCESS)return SQL_SYNTAX;
	rm_column = (RM_FileHandle *)malloc(sizeof(RM_FileHandle));
	rm_column->bOpen = false;
	if (RM_OpenFile("SYSCOLUMNS", rm_column)!= SUCCESS)return SQL_SYNTAX;
	//��ϵͳ���ļ�ɨ��
	FileScan.bOpen = false;
	if (OpenScan(&FileScan, rm_table, 0, NULL)!= SUCCESS)return SQL_SYNTAX;
	//ѭ�����ұ���ΪrelName��Ӧ��ϵͳ���еļ�¼,��¼��������attrcount
	while (GetNextRec(&FileScan, &rectab) == SUCCESS){
		if (strcmp(relName, rectab.pData) == 0){
			memcpy(&attrcount, rectab.pData + 21, sizeof(int));
			break;
		}
	}
	//�ر�ϵͳ���ļ�ɨ��
	if(CloseScan(&FileScan)!=SUCCESS)return SQL_SYNTAX;
	//��ϵͳ���ļ�ɨ��
	FileScan.bOpen = false;
	if (OpenScan(&FileScan, rm_column, 0, NULL)!= SUCCESS)return SQL_SYNTAX;
	//����֮ǰ��ȡ��ϵͳ������Ϣ����ȡ������Ϣ�����������ctmp��
	Column = (column *)malloc(attrcount*sizeof(column));
	cupdate = (column *)malloc(sizeof(column));
	ctmp = Column;
	while (GetNextRec(&FileScan, &reccol) == SUCCESS){
		if (strcmp(relName, reccol.pData) == 0){//�ҵ�����ΪrelName�ĵ�һ����¼�����ζ�ȡattrcount����¼
			for (int i = 0; i < attrcount; i++,ctmp++){
				memcpy(ctmp->tablename, reccol.pData, 21);
				memcpy(ctmp->attrname, reccol.pData + 21, 21);
				memcpy(&(ctmp->attrtype), reccol.pData + 42, sizeof(AttrType));
				memcpy(&(ctmp->attrlength), reccol.pData + 42 + sizeof(AttrType), sizeof(int));
				memcpy(&(ctmp->attroffset), reccol.pData + 42 + sizeof(int)+sizeof(AttrType), sizeof(int));
				if ((strcmp(relName,ctmp->tablename) == 0) && (strcmp(attrName,ctmp->attrname) == 0)){
					cupdate = ctmp;//�ҵ�Ҫ�������� ��Ӧ������
				}
				if (GetNextRec(&FileScan, &reccol)!= SUCCESS)break;
			}
			break;
		}
	}
	//�ر�ϵͳ���ļ�ɨ��
	if(CloseScan(&FileScan)!=SUCCESS)return SQL_SYNTAX;
	//�򿪼�¼�ļ�ɨ��
	FileScan.bOpen = false;
	if (OpenScan(&FileScan, rm_data, 0, NULL)!= SUCCESS)return SQL_SYNTAX;
	//ѭ�����ұ���ΪrelName��Ӧ�����ݱ��еļ�¼,������¼��Ϣ������recdata��
	while (GetNextRec(&FileScan, &recdata) == SUCCESS){
		for (i = 0, torf = 1, contmp = conditions; i < nConditions; i++, contmp++){//conditions������һ�ж�
			ctmpleft = ctmpright = Column;//ÿ��ѭ����Ҫ����������ϵͳ���ļ����ҵ�����������Ӧ������
			//��������ֵ
			if (contmp->bLhsIsAttr == 1 && contmp->bRhsIsAttr == 0){
				for (int j = 0; j < attrcount; j++){//attrcount��������һ�ж�
					if (contmp->lhsAttr.relName == NULL){//��������δָ������ʱ��Ĭ��ΪrelName
						contmp->lhsAttr.relName = (char *)malloc(21);
						strcpy(contmp->lhsAttr.relName, relName);
					}
					if ((strcmp(ctmpleft->tablename, contmp->lhsAttr.relName) == 0)
						&& (strcmp(ctmpleft->attrname, contmp->lhsAttr.attrName) == 0)){//���ݱ����������ҵ���Ӧ����
						break;
					}
					ctmpleft++;
				}
				//��conditions��ĳһ�����������ж�
				if (ctmpleft->attrtype == ints){//�ж����Ե�����
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
			//��������ֵ
			else  if (contmp->bLhsIsAttr == 0 && contmp->bRhsIsAttr == 1){
				for (int j = 0; j < attrcount; j++){//attrcount��������һ�ж�
					if (contmp->rhsAttr.relName == NULL){//��������δָ������ʱ��Ĭ��ΪrelName
						contmp->rhsAttr.relName = (char *)malloc(21);
						strcpy(contmp->rhsAttr.relName, relName);
					}
					if ((strcmp(ctmpright->tablename, contmp->rhsAttr.relName) == 0)
						&& (strcmp(ctmpright->attrname, contmp->rhsAttr.attrName) == 0)){//���ݱ����������ҵ���Ӧ����
						break;
					}
					ctmpright++;
				}
				//��conditions��ĳһ�����������ж�
				if (ctmpright->attrtype == ints){//�ж����Ե�����
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
			//���Ҿ�����
			else  if (contmp->bLhsIsAttr == 1 && contmp->bRhsIsAttr == 1){
				for (int j = 0; j < attrcount; j++){//attrcount��������һ�ж�
					if (contmp->lhsAttr.relName == NULL){//��������δָ������ʱ��Ĭ��ΪrelName
						contmp->lhsAttr.relName = (char *)malloc(21);
						strcpy(contmp->lhsAttr.relName, relName);
					}
					if ((strcmp(ctmpleft->tablename, contmp->lhsAttr.relName) == 0)
						&& (strcmp(ctmpleft->attrname, contmp->lhsAttr.attrName) == 0)){//���ݱ����������ҵ���Ӧ����
						break;
					}
					ctmpleft++;
				}
				for (int j = 0; j < attrcount; j++){//attrcount��������һ�ж�
					if (contmp->rhsAttr.relName == NULL){//��������δָ������ʱ��Ĭ��ΪrelName
						contmp->rhsAttr.relName = (char *)malloc(21);
						strcpy(contmp->rhsAttr.relName, relName);
					}
					if ((strcmp(ctmpright->tablename, contmp->rhsAttr.relName) == 0)
						&& (strcmp(ctmpright->attrname, contmp->rhsAttr.attrName) == 0)){//���ݱ����������ҵ���Ӧ����
						break;
					}
					ctmpright++;
				}
				//��conditions��ĳһ�����������ж�
				if (ctmpright->attrtype == ints && ctmpleft->attrtype == ints){//�ж����Ե�����
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
/*	//��ϵͳ���ļ�ɨ��
	FileScan.bOpen = false;
	if (OpenScan(&FileScan, rm_column, 0, NULL)!= SUCCESS){
		AfxMessageBox("ϵͳ���ļ�ɨ��ʧ��");
		return SQL_SYNTAX;
	}
	//ɨ��ϵͳ���ļ�������������ϴ�����������ɾ��ԭ���������´���
	while (GetNextRec(&FileScan, &reccol) == SUCCESS){
		if (strcmp(relName, reccol.pData) == 0){//�ҵ�����ΪrelName�ĵ�һ����¼�����ζ�ȡattrcount����¼
			for (int i = 0; i < attrcount; i++){
				if((reccol.pData+42+3*sizeof(int))=="1"){
					memcpy(attr,reccol.pData+21,21);
					memcpy(index,reccol.pData+43+2*sizeof(int)+sizeof(AttrType),21);
					memcpy(reccol.pData+42+3*sizeof(int),"0",1);//����������Ϊ0
			        if (UpdateRec(rm_column,&reccol)!=SUCCESS)return SQL_SYNTAX;
					tmp.Remove((LPCTSTR)index);//ɾ�������ļ�
					CreateIndex(index,relName,attr);
				}
			}
			break;
		}
	}
	//�ر�ϵͳ���ļ�ɨ��
	if(CloseScan(&FileScan)!=SUCCESS)return SQL_SYNTAX;*/
	//�رռ�¼�ļ�ɨ��
	if(CloseScan(&FileScan)!=SUCCESS)return SQL_SYNTAX;
	//�ر��ļ�
	if (RM_CloseFile(rm_table)!= SUCCESS)return SQL_SYNTAX;
	free(rm_table);
    if (RM_CloseFile(rm_column)!= SUCCESS)return SQL_SYNTAX;
	free(rm_column);
	if (RM_CloseFile(rm_data)!= SUCCESS)return SQL_SYNTAX;
	free(rm_data);
	return SUCCESS;	
}

bool CanButtonClick(){//��Ҫ����ʵ��
	//�����ǰ�����ݿ��Ѿ���
	return true;
	//�����ǰû�����ݿ��
	//return false;
}
