#include "stdafx.h"
#include "RM_Manager.h"
#include "str.h"
#include <cstring>
#include <bitset>

RC OpenScan(RM_FileScan *rmFileScan,RM_FileHandle *fileHandle,int conNum,Con *conditions)//��ʼ��ɨ��
{
	//��ɨ���Ѿ���
	if(rmFileScan->bOpen)
		return RM_FSOPEN;
	//��ʼ��ɨ��ṹ
	rmFileScan->bOpen=TRUE;
	rmFileScan->conNum=conNum;
	rmFileScan->conditions=conditions;
	//ȡ�ô�ɨ�����ҳ��
	if(GetThisPage(fileHandle->file,2,&rmFileScan->PageHandle))
		return FAIL;
	rmFileScan->pn=3;
	rmFileScan->sn=0; //��۴�0��ʼ���
	return SUCCESS;
}

//������������ʹ�������Ż�һ�³�����ʽ��̫����
RC GetNextRec(RM_FileScan *rmFileScan,RM_Record *rec)
{
	//ȷ���Ƿ������
	//if(rmFileScan->pn==(rmFileScan->pRMFileHandle->file->pFileSubHeader->pageCount-1))
	//���ҵ�һ���Ѿ������ҳ
	char bitmap,bitmapRec;
	while(rmFileScan->pn<=(rmFileScan->pRMFileHandle->file->pFileSubHeader->pageCount-1))
	{
		//���ж��Ƿ�Ϊ��ҳ
		bitmap=*(rmFileScan->pRMFileHandle->file->pBitmap+(rmFileScan->pn/8));
		if((bitmap&(0x01<<(rmFileScan->pn%8)))==0) //�ǿ�ҳ
		{
			rmFileScan->pn++;
			rmFileScan->sn=0; //����ҳ��0�Ų�ۿ�ʼ����
		}
		else  //���ǿ�ҳ����������¼���бȽ�
		{
			//���Ȼ�ø�ҳ���
			if(GetThisPage(rmFileScan->pRMFileHandle->file,rmFileScan->pn,&rmFileScan->PageHandle))
				return FAIL;	
			for(;rmFileScan->sn<rmFileScan->pRMFileHandle->recNum;rmFileScan->sn++)
			{
				//�ȼ��λͼ����¼Ϊ��ֱ������
				bitmapRec=*(rmFileScan->PageHandle.pFrame->page.pData+(rmFileScan->sn/8));
				if(bitmapRec&(0x01<<rmFileScan->sn%8)==0)
					continue;
				//��ʼ�Ƚϼ�¼���ѯ����
				char *recOffset=rmFileScan->PageHandle.pFrame->page.pData+(rmFileScan->pRMFileHandle->recOffset+rmFileScan->sn*rmFileScan->pRMFileHandle->recSize);  //��ǰʹ�ü�¼��ָ��
				for(int con=0;con<rmFileScan->conNum;con++)
				{
					//�ȽϹ���00,01,10,11����
					int type=0;
					type|=rmFileScan->conditions[con].bLhsIsAttr;
					type<<=1;
					type|=rmFileScan->conditions[con].bRhsIsAttr;
					switch(type)
					{
					case 0:
						//shit, damn hard
					}
				}
			}
		}
	}
	/*
	char bitmap=*(rmFileScan->pRMFileHandle->file->pBitmap+((rmFileScan->pn+1)/8));
	while((bitmap&(0x01<<(rmFileScan->pn%8)))==0) //δ����ҳ
	{
		rmFileScan->pn++;
		if(rmFileScan->pn==(rmFileScan->pRMFileHandle->file->pFileSubHeader->pageCount-1))
			return RM_NOMORERECINMEM;
		bitmap=*(rmFileScan->pRMFileHandle->file->pBitmap+((rmFileScan->pn+1)%8));
	}

	return SUCCESS;
	*/
	rec->bValid=FALSE;
	return RM_NOMORERECINMEM;
}

RC GetRec (RM_FileHandle *fileHandle,RID *rid, RM_Record *rec) 
{
	//�����ж�rid�Ƿ���Ч��ҳ�Ƿ񱻷��䣬��¼�Ƿ���Ч��
	rec->bValid=FALSE;
	//char bitmapPage=*(fileHandle->file->pBitmap+rid->pageNum/8);
	if(!fileHandle->pageCtlBitmap->atPos(rid->pageNum)/*(bitmapPage&(0x01<<(rid->pageNum%8))==0)*/||(rid->pageNum>fileHandle->file->pFileSubHeader->pageCount)||(rid->slotNum>fileHandle->recPerPage-1))
		return RM_INVALIDRID;
	PF_PageHandle *targetPage;
	if(GetThisPage(fileHandle->file,rid->pageNum,targetPage))
		return FAIL;
	char bitmapRec=*(targetPage->pFrame->page.pData+rid->slotNum/8);
	if(bitmapRec&(0x01<<(rid->slotNum%8))==0)
		return RM_INVALIDRID;
	//RID��Ч
	rid->bValid=TRUE;
	rec->bValid=TRUE;
	rec->pData=targetPage->pFrame->page.pData+fileHandle->recOffset+rid->slotNum*fileHandle->recSize;
	return SUCCESS;
}

RC InsertRec (RM_FileHandle *fileHandle,char *pData, RID *rid)
{
	/*************
	�����¼ʱ���������ļ����ҵ�һ������ҳ����û������Ҫ����һ���µ�ҳ�棩��
	�ڸ�ҳ���ҵ�һ���ղ�۲������¼����¼�����Ҫ���¿���ҳ��
	�����ļ������ļ�¼����1��ͬʱ�����λͼ�е���Ӧλ��1��
	�����¼����󣬸�ҳ���Ѿ�û�пղ�ۣ��򽫸�ҳ����Ϊ��ҳ��
	���ڼ�¼�ļ��Ŀ���ҳ��λͼ�Ͻ���ҳ������Ӧ��λ��1��
	*************/
	//��û��δ��ҳ����ȫ��ҳ���Ѿ�����
	if(!fileHandle->recCtlBitmap->anyZero())
		return FAIL;
	//���Ȳ����Ƿ�����ѷ����δ��ҳ
	int unfillPage=fileHandle->recCtlBitmap->firstZero(2);
	while((!fileHandle->pageCtlBitmap->atPos(unfillPage))&&unfillPage<fileHandle->bitmapLength*8)
		unfillPage=fileHandle->recCtlBitmap->firstZero(unfillPage+1);
	if(unfillPage>=fileHandle->bitmapLength*8)    //��û���Ѿ������δ��ҳ��������ҳ
	{
		if(!fileHandle->pageCtlBitmap->anyZero())   //�Ѿ�û�п���ҳ��
			return FAIL;
		//���п���ҳ�棨��δ����ҳ�棩
		PF_PageHandle *newPage;
		if(AllocatePage(fileHandle->file,newPage))
			return FAIL;
		bitmanager bmpNewPage(fileHandle->bitmapLength,newPage->pFrame->page.pData,1/*ȥ��*/);
		memcpy(newPage->pFrame->page.pData+fileHandle->recOffset,pData,fileHandle->recSize);
		bmpNewPage.setBitmap(2,1);
	}
	else    //�����Ѿ������δ��ҳ��ҳ��ΪunfillPage
	{
		PF_PageHandle *page;
		if(GetThisPage(fileHandle->file,unfillPage,page))
			return FAIL;
		//ͨ��λͼ�ҵ��ղ�
		bitmanager bmp(fileHandle->bitmapLength,page->pFrame->page.pData,fileHandle->bitmapLength*8);
		int emptySlot=bmp.firstZero(0);
		//�����¼
		memcpy(page->pFrame->page.pData+fileHandle->recOffset+emptySlot*fileHandle->recSize,pData,fileHandle->recSize);
		//���²�λͼ
		bmp.setBitmap(emptySlot,1);
		//��ҳ�����������¼�¼����λͼ
		if(!bmp.anyZero())
			fileHandle->recCtlBitmap->setBitmap(unfillPage,1);
	}
	//���¼�¼��
	fileHandle->recNum++;
	return SUCCESS;
}

RC DeleteRec (RM_FileHandle *fileHandle,const RID *rid)
{
	//���RID��Ч��
	if(rid->pageNum>fileHandle->file->pFileSubHeader->pageCount||(!fileHandle->pageCtlBitmap->atPos(rid->pageNum)))
		return RM_INVALIDRID;
	//���Ŀ��ҳ�ľ��
	PF_PageHandle *target;
	if(GetThisPage(fileHandle->file,rid->pageNum,target))
		return FAIL;
	//����¼�Ƿ���Ч
	if(
	return SUCCESS;
}

RC UpdateRec (RM_FileHandle *fileHandle,const RM_Record *rec)
{

	return SUCCESS;
}

RC RM_CreateFile (char *fileName, int recordSize)
{
	int rtn=CreateFile(fileName);
	if(rtn)
		return FAIL;  //����ֵ��Ҫ����һ��
	//����ɹ������ɼ�¼��Ϣ����ҳ�����Ȼ�ô����ļ��ľ��
	PF_FileHandle *file;
	rtn=openFile(fileName,file);
	if(rtn)
		return FAIL;  //����ֵ��Ҫ����һ��
	//������ҳ��
	PF_PageHandle *ctrPage;
	rtn=AllocatePage(file,ctrPage);
	if(rtn)
		return FAIL;
	//�ڼ�¼����ҳ��ż�¼��Ϣ���ݽṹ
	RM_recControl *recCtl;
	recCtl=(RM_recControl *)ctrPage->pFrame->page.pData;
	//��ʼ��
	recCtl->recNum=0;
	recCtl->recSize=recordSize;
	recCtl->recPerPage=PF_PAGE_SIZE/(recordSize+0.125); //������Դ�ŵļ�¼������ÿ���¼ռ��recordSize���0.125��λͼ��С�ֽ�,��1��Ϊ��ȷ��λͼ��С�ܹ�Ϊ8�ı���
	//Ϊ�˷���λͼ����ƣ��˴�����¼������ȡ8�ı���
	recCtl->recPerPage=(int(recCtl->recPerPage/8))*8;
	recCtl->recordOffset=recCtl->recPerPage/8; //λͼ�Ĵ�С����Ϊ8��������
	//recCtl->fileNum=1; //�մ���ʱ��ֻʹ����һ����ҳ�ļ�
	//�رոոմ򿪵��ļ�
	CloseFile(file);
	return SUCCESS;
}

RC RM_OpenFile(char *fileName, RM_FileHandle *fileHandle)
{
	if(fileHandle->bOpen)  //��ʹ�õľ���Ѿ���Ӧһ���򿪵��ļ�
		return RM_FHOPENNED;
	PF_FileHandle *filePF;
	if(openFile(fileName,filePF))
		return FAIL;
	fileHandle->bOpen=TRUE;
	//fileHandle->fileName=fileName;
	//fileHandle->file[0]=filePF;
	fileHandle->file=filePF;
	//��ȡ��¼���������Ϣ
	PF_PageHandle *ctrPage;
	if(GetThisPage(filePF,1,ctrPage))
	{
		CloseFile(filePF);
		return FAIL;
	}
	RM_recControl *recCtl;
	recCtl=(RM_recControl *)ctrPage->pFrame->page.pData;
	fileHandle->recNum=recCtl->recNum;
	fileHandle->recOffset=recCtl->recordOffset;
	fileHandle->recPerPage=recCtl->recPerPage;
	fileHandle->recSize=recCtl->recSize;
	fileHandle->bitmapLength=PF_PAGE_SIZE-sizeof(RM_recControl); //���Է�������ҳ��/8
	//��ȡ��¼����λͼ
	fileHandle->recCtlBitmap=new bitmanager(fileHandle->bitmapLength,ctrPage->pFrame->page.pData+sizeof(RM_recControl),fileHandle->file->pFileSubHeader->pageCount+1);
	//��ȡҳ�����λͼ
	fileHandle->pageCtlBitmap=new bitmanager(fileHandle->bitmapLength,filePF->pBitmap,fileHandle->file->pFileSubHeader->pageCount+1);
	/*
	//ȷ���ܹ����ٸ���ҳ�ļ�����Ҫ��ȡ
	PF_PageHandle *ctrPage;
	if(GetThisPage(filePF,1,ctrPage))
		return FAIL;
	RM_recControl *control;
	control=(RM_recControl *)ctrPage->pFrame->page.pData;
	fileHandle->fileNum=control->fileNum;
	//��ʹ���˶����ҳ�ļ��洢�������
	if(fileHandle->fileNum>1)
	{
		char name[sizeof(fileName)+2];  //����������ļ���
		strcpy(name,fileName);
		name[sizeof(fileName)+1]='\0';  //ȷ���ַ�����β
		for(int i=1;i<fileHandle->fileNum;i++)
		{
			name[sizeof(fileName)]=(char)(i+48);
			if(openFile(name,fileHandle->file[i]))  //�޸�
			{
				for(int close=0;close<i;close++)       //����һ���ļ���ʧ����ʧ�ܣ����Ѿ��򿪵Ĺر�
					CloseFile(fileHandle->file[close]);
				return FAIL;
			}
		}
	}
	*/
	return SUCCESS;
}

RC RM_CloseFile(RM_FileHandle *fileHandle)
{
	//���Ѿ��ر�
	if(!fileHandle->bOpen)
		return RM_FHCLOSED;
	if(CloseFile(fileHandle->file))
			return FAIL;
	/*
	//�������йصķ�ҳ�ļ�ȫ���ر�
	for(int close=0;close<fileHandle->fileNum;close++)
	{
		if(CloseFile(fileHandle->file[close]))
			return FAIL;
	}
	*/
	fileHandle->bOpen=FALSE;
	return SUCCESS;
}

/*

RC��RM���֣�

RM_FH CLOSED,
RM_FH OPENNED,
RM_INVALID RECSIZE,
RM_INVALID RID,
RM_FS CLOSED,
RM_NO MORE REC IN MEM,
RM_FS OPEN,
RM_NO MORE IDX IN MEM,
*/