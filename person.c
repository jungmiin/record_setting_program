#include <stdio.h>
#include <memory.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include "person.h"

typedef struct _Header
{
	int entire_page; //전체 페이지 수
	int entire_record; //전체 레코드 수
	int delete_page; //최근 삭제된 페이지 인덱스
	int delete_record; //최근 삭제된 레코드 인덱스
} Header;

Header header;
//sprintf(headerbuf,"%d%d%d%d",header.entire_page,entire_record,delete_page,delete_record);

//필요한 경우 헤더 파일과 함수를 추가할 수 있음

// 과제 설명서대로 구현하는 방식은 각자 다를 수 있지만 약간의 제약을 둡니다.
// 레코드 파일이 페이지 단위로 저장 관리되기 때문에 사용자 프로그램에서 레코드 파일로부터 데이터를 읽고 쓸 때도
// 페이지 단위를 사용합니다. 따라서 아래의 두 함수가 필요합니다.
// 1. readPage(): 주어진 페이지 번호의 페이지 데이터를 프로그램 상으로 읽어와서 pagebuf에 저장한다
// 2. writePage(): 프로그램 상의 pagebuf의 데이터를 주어진 페이지 번호에 저장한다
// 레코드 파일에서 기존의 레코드를 읽거나 새로운 레코드를 쓰거나 삭제 레코드를 수정할 때나
// 모든 I/O는 위의 두 함수를 먼저 호출해야 합니다. 즉 페이지 단위로 읽거나 써야 합니다.

//
// 페이지 번호에 해당하는 페이지를 주어진 페이지 버퍼에 읽어서 저장한다. 페이지 버퍼는 반드시 페이지 크기와 일치해야 한다.
//
void readPage(FILE *fp, char *pagebuf, int pagenum)
{
	fseek(fp, PAGE_SIZE*pagenum, SEEK_SET);
	fread(pagebuf, PAGE_SIZE, 1, fp);

}

//
// 페이지 버퍼의 데이터를 주어진 페이지 번호에 해당하는 위치에 저장한다. 페이지 버퍼는 반드시 페이지 크기와 일치해야 한다.
//
void writePage(FILE *fp, const char *pagebuf, int pagenum)
{
	fseek(fp,PAGE_SIZE*pagenum, SEEK_SET);
	fwrite(pagebuf, PAGE_SIZE, 1, fp);

}

//
// 새로운 레코드를 저장할 때 터미널로부터 입력받은 정보를 Person 구조체에 먼저 저장하고, pack() 함수를 사용하여
// 레코드 파일에 저장할 레코드 형태를 recordbuf에 만든다. 그런 후 이 레코드를 저장할 페이지를 readPage()를 통해 프로그램 상에
// 읽어 온 후 pagebuf에 recordbuf에 저장되어 있는 레코드를 저장한다. 그 다음 writePage() 호출하여 pagebuf를 해당 페이지 번호에
// 저장한다. pack() 함수에서 readPage()와 writePage()를 호출하는 것이 아니라 pack()을 호출하는 측에서 pack() 함수 호출 후
// readPage()와 writePage()를 차례로 호출하여 레코드 쓰기를 완성한다는 의미이다.
// 
void pack(char *recordbuf, const Person *p)
{

	sprintf(recordbuf,"%s#%s#%s#%s#%s#%s#", p->sn, p->name, p->age, p->addr, p->phone, p->email);

}

// 
// 아래의 unpack() 함수는 recordbuf에 저장되어 있는 레코드를 구조체로 변환할 때 사용한다. 이 함수가 언제 호출되는지는
// 위에서 설명한 pack()의 시나리오를 참조하면 된다.
//
void unpack(const char *recordbuf, Person *p)
{
	char *pf = malloc(RECORD_SIZE);
	char *buf[6];
	char *tmp = malloc(RECORD_SIZE);


	for(int i=0; i<6; i++){
		buf[i]=malloc(30*sizeof(char));
	}

	strcpy(tmp, recordbuf);

	pf=strtok(tmp, "#");
	buf[0]=pf;
	strncpy(p->sn, buf[0], strlen(buf[0]));

	pf=strtok(NULL, "#");
	buf[1]=pf;
	strncpy(p->name, buf[1], strlen(buf[1]));

	pf=strtok(NULL, "#");
	buf[2]=pf;
	strncpy(p->age, buf[2], strlen(buf[2]));

	pf=strtok(NULL, "#");
	buf[3]=pf;
	strncpy(p->addr, buf[3], strlen(buf[3]));

	pf=strtok(NULL, "#");
	buf[4]=pf;
	strncpy(p->phone, buf[4], strlen(buf[4]));

	pf=strtok(NULL,"#");
	buf[5]=pf;
	strncpy(p->email, buf[5], strlen(buf[5]));

}

//
// 새로운 레코드를 저장하는 기능을 수행하며, 터미널로부터 입력받은 필드값을 구조체에 저장한 후 아래의 insert() 함수를 호출한다.
//
void insert(FILE *fp, const Person *p)
{
	char *recordbuf, *pagebuf;
	int pagenum, recordnum;
	char* headerchar = malloc(PAGE_SIZE);
	char* headerbuf = malloc(PAGE_SIZE);
	char *pf, *df;
	int del_page, del_record;

	pf=headerbuf;

	recordbuf=malloc(RECORD_SIZE);
	pagebuf=malloc(PAGE_SIZE);

	memset(recordbuf, (char)0xFF, PAGE_SIZE);
	memset(pagebuf, (char)0xFF, PAGE_SIZE);

	pack(recordbuf, p); //pack 함수 호출

	readPage(fp, headerbuf, 0);

	memcpy(&header.entire_page, pf, sizeof(int));
	pf+=sizeof(int);
	memcpy(&header.entire_record, pf, sizeof(int));
	pf+=sizeof(int);
	memcpy(&header.delete_page, pf, sizeof(int));
	pf+=sizeof(int);
	memcpy(&header.delete_record, pf, sizeof(int));
	pf=headerbuf; //처음 위치로

	if(header.entire_page==-1){ //바로 pagebuf에 record 넣고 쓰삼

		memcpy(pagebuf, recordbuf, strlen(recordbuf));

		writePage(fp, pagebuf, 1);

		header.entire_page = 2;
		header.entire_record = 1;

	}
	else{ //먼저 delete_page, delete_record 검사한 후, 있으면 해당 pagenum 불러오고 아니면 entire_page 확인 후 해당 pagenum 불러오

		if((header.delete_page!=-1)&&(header.delete_record!=-1)){ //삭제된 리코드가 있을 경우 

			readPage(fp, pagebuf, header.delete_page);

			df=pagebuf;
			df+=RECORD_SIZE*header.delete_record;
			df+=sizeof(char);
			memcpy(&del_page, df, sizeof(int));
			df+=sizeof(int);
			memcpy(&del_record, df, sizeof(int));

			memcpy(pagebuf+(RECORD_SIZE*header.delete_record), recordbuf, RECORD_SIZE);

			writePage(fp, pagebuf, header.delete_page);

			header.delete_page=del_page;
			header.delete_record=del_record;



		}
		else{ //삭제된 리코드가 없을 경우

			readPage(fp, pagebuf, header.entire_page-1);

			for(int j=0; PAGE_SIZE>(RECORD_SIZE*j);j++){

				if(pagebuf[RECORD_SIZE*j]==(char)0xFF){ //비었다는 의미

					memcpy(pagebuf+(RECORD_SIZE*j), recordbuf, strlen(recordbuf));

					writePage(fp,pagebuf,header.entire_page-1);

					header.entire_record++;

					break;
				}
				else if(PAGE_SIZE<(RECORD_SIZE*(j+2))){ //꽉차있다면

					memset(pagebuf, (char)0xFF, PAGE_SIZE);

					memcpy(pagebuf, recordbuf, strlen(recordbuf));

					writePage(fp, pagebuf, header.entire_page);

					header.entire_page++;
					header.entire_record++;

					break;

				}
			}//page 순회

		}

	}


	memcpy(pf, &header.entire_page,sizeof(int));
	pf+=sizeof(int);
	memcpy(pf, &header.entire_record,sizeof(int));
	pf+=sizeof(int);
	memcpy(pf, &header.delete_page,sizeof(int));
	pf+=sizeof(int);
	memcpy(pf, &header.delete_record,sizeof(int));

	writePage(fp, headerbuf, 0); //headerbuf 갱신

}

//
// 주민번호와 일치하는 레코드를 찾아서 삭제하는 기능을 수행한다.
//
void delete(FILE *fp, const char *sn)
{
	char *pagebuf=malloc(PAGE_SIZE);
	char *headerbuf=malloc(PAGE_SIZE);
	char *recordbuf=malloc(RECORD_SIZE);
	char *pf, *rf;
	Person p;

	memset(&p,0,sizeof(Person));

	readPage(fp, headerbuf, 0);
	pf=headerbuf;

	memcpy(&header.entire_page, pf, sizeof(int));
	pf+=sizeof(int);
	memcpy(&header.entire_record, pf, sizeof(int));
	pf+=sizeof(int);
	memcpy(&header.delete_page, pf, sizeof(int));
	pf+=sizeof(int);
	memcpy(&header.delete_record, pf, sizeof(int));
	pf=headerbuf; //처음 위치로

	//page를 읽어와
	for(int i=1; i<=header.entire_page-1; i++){

		readPage(fp, pagebuf, i);

		for(int j=0; PAGE_SIZE>(RECORD_SIZE*j) ; j++){
			if((pagebuf[RECORD_SIZE*j]!='*')&&(pagebuf[RECORD_SIZE*j]!=(char)0xFF)){
				memcpy(recordbuf, pagebuf+(RECORD_SIZE*j), RECORD_SIZE);
				unpack(recordbuf, &p);
				if(!strcmp(p.sn, sn)){ //같다면
					rf=recordbuf;
					memcpy(rf, "*", sizeof(char));
					rf+=sizeof(char);
					memcpy(rf,&header.delete_page,sizeof(int));
					rf+=sizeof(int);
					memcpy(rf,&header.delete_record,sizeof(int));

					memcpy(pagebuf+(RECORD_SIZE*j), recordbuf, RECORD_SIZE);

					writePage(fp, pagebuf, i);

					header.delete_page = i;
					header.delete_record = j;

					memcpy(pf, &header.entire_page,sizeof(int));
					pf+=sizeof(int);
					memcpy(pf, &header.entire_record,sizeof(int));
					pf+=sizeof(int);
					memcpy(pf, &header.delete_page,sizeof(int));
					pf+=sizeof(int);
					memcpy(pf, &header.delete_record,sizeof(int));

					writePage(fp, headerbuf, 0); //headerbuf 갱신

					return;
				}
			}
			else if(PAGE_SIZE<(RECORD_SIZE*(j+2))){ //꽉차있다면
				break;
			}


		}

	}
	//page를 record로 나눠
	//record를 unpack 해 sn을 받아
	//검사해
	//다르면 다음 리코드반복 

}

int main(int argc, char *argv[])
{
	FILE *fp;  // 레코드 파일의 파일 포인터
	Person p;
	struct stat statbuf;
	char init_headerbuf[PAGE_SIZE];
	int init_num=-1;
	char *pf;

	pf=init_headerbuf;

	memset(init_headerbuf,0xFF, sizeof(init_headerbuf));

	if(argc<4){
		fprintf(stderr,"명령인자 갯수가 부족합니다.\n");
		exit(1);
	}

	switch(argv[1][0]){

		case 'i':

			if(argc<8){ //명령인자 갯수가 부족할 경우 에러 처리
				fprintf(stderr,"insert 옵션에 해당하는 명령인자 갯수가 부족합니다.\n");
				exit(1);
			}

			if(access(argv[2], F_OK)<0){ //맨 처음 실행할 경우
				fp=fopen(argv[2],"w+");
			}
			else{ //이미 존재하는 파일일 경우
				fp = fopen(argv[2], "r+");
			}

			stat(argv[2], &statbuf);

			if(statbuf.st_size==0){ //파일 사이즈가 0이라면, headerbuf 생성 및 초기화 필요

				for(int k=0; k<4; k++){
					memcpy(pf, &init_num, sizeof(int));
					pf+=sizeof(int);
				}

				writePage(fp, init_headerbuf, 0);
				fclose(fp);
				fp = fopen(argv[2], "r+");
			}

			strcpy(p.sn,argv[3]); //구조체에 토큰 넣기
			strcpy(p.name, argv[4]);
			strcpy(p.age, argv[5]);
			strcpy(p.addr, argv[6]);
			strcpy(p.phone, argv[7]);
			strcpy(p.email, argv[8]);

			insert(fp, &p); //insert 함수 실행

			break;

		case 'd':

			if(argc<4){
				fprintf(stderr,"delete 옵션에 해당하는 명령인자 갯수가 부족합니다.\n");
				exit(1);
			}

			fp=fopen(argv[2],"r+");

			delete(fp, argv[3]);

			break;

		default :

			fprintf(stderr, "알맞은 옵션이 아닙니다.\n");
			exit(1);

			break;
	}
}

