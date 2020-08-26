/* 
 * File : myutil.c
 * 
 */
 
 
 #include <ctype.h>
 #include <time.h>
 #include <string.h>
 #include <stdio.h>
 
 
 /* Function to convert digits char to int
 * The return is number from 0-9
 */
 
 int CharToInt(char *sym)
{
	int inum;
	const char c = *sym;
	if (isdigit(c)){
		switch(c){
			case '0':
			inum = 0;
			break;
			case '1':
			inum = 1;
			break;
			case '2':
			inum = 2;
			break;
			case '3':
			inum = 3;
			break;
			case '4':
			inum = 4;
			break;
			case '5':
			inum = 5;
			break;
			case '6':
			inum = 6;
			break;
			case '7':
			inum = 7;
			break;
			case '8':
			inum = 8;
			break;
			case '9':
			inum = 9;
			break;
		}
	} else inum = -1;
	return inum;
}


int TenToOne(int number)
{
	int tmp = number;
	if ((number >= 10) && (number < 20)) tmp -= 10;
	else if ((number >= 20) && (number < 30)) tmp -= 20;
	else if ((number >= 30) && (number < 40)) tmp -= 30;
	else if ((number >= 40) && (number < 50)) tmp -= 40;
	else if ((number >= 50) && (number < 60)) tmp -= 50;
	else if ((number >= 60) && (number < 70)) tmp -= 60;
	else if ((number >= 70) && (number < 80)) tmp -= 70;
	else if ((number >= 80) && (number < 90)) tmp -= 80;
	else if ((number >= 90) && (number < 100)) tmp -= 90;
	printf("Zamena1 %i z %i \n", number, tmp);
	return tmp;
}

int OneFromTens(int number)
{
	int tmp = number;
	if ((number >= 10) || (number < 20)) tmp /= 10;
	else if ((number >= 20) || (number < 30)) tmp /= 20;
	else if ((number >= 30) || (number < 40)) tmp /= 30;
	else if ((number >= 40) || (number < 50)) tmp /= 40;
	else if ((number >= 50) || (number < 60)) tmp /= 50;
	else if ((number >= 60) || (number < 70)) tmp /= 60;
	else if ((number >= 70) || (number < 80)) tmp /= 70;
	else if ((number >= 80) || (number < 90)) tmp /= 80;
	else if ((number >= 90) || (number < 100)) tmp /= 90;
	printf("Zamena10 %i z %i \n", number, tmp);
	return tmp;
}

void DateToChar(char *table)
{
	char tmp[10];
	struct tm *rawtime;
	time_t mytime;
	time(&mytime);
	int tmp_num; 
	int num;
	rawtime = localtime(&mytime);
	tmp_num = rawtime->tm_mday;
	if (tmp_num > 10){
		num = OneFromTens(tmp_num);
		tmp[0] = num + '0';
		num = TenToOne(tmp_num);
		tmp[1] = num + '0';
	} else {
		tmp[0] = '0';
		tmp[1] = (rawtime->tm_mday) + '0';
	}
	tmp[2] = '-';
	tmp_num = rawtime->tm_mon;
	if (tmp_num > 10){
		num = OneFromTens(tmp_num);
		tmp[3] = num + '0';
		num = TenToOne(tmp_num);
		tmp[4] = num + '0';
	} else {
		tmp[3] = '0';
		tmp[4] = (rawtime->tm_mon) + '0';
	}
	
	tmp[5] = '-';
	tmp_num = rawtime->tm_year - 100;					// back num, add 1900 for actual year
	num = tmp_num / 10;
	tmp[6] = num + '0';
	num = TenToOne(tmp_num);
	tmp[7] = num + '0';
	tmp[8] = '-';
	
	strcpy(table, tmp);
}

void TimeToChar(char *table)
{
	char tmp[10];
	struct tm *rawtime;
	time_t mytime;
	time(&mytime);
	int tmp_num; 
	int num;
	rawtime = localtime(&mytime);
	tmp_num = rawtime->tm_hour;
	if (tmp_num > 10)
	{
		num = OneFromTens(tmp_num);
		tmp[0] = num + '0';
		num = TenToOne(tmp_num);
		tmp[1] = num + '0';
	}else {
		tmp[0] = '0';
		tmp[1] = tmp_num + '0';
	}
	tmp[2] = ':';
	tmp_num = rawtime->tm_min;
	if (tmp_num > 10)
	{
		num = OneFromTens(tmp_num);
		tmp[3] = num + '0';
		num = TenToOne(tmp_num);
		tmp[4] = num + '0';
	}else {
		tmp[3] = '0';
		tmp[4] = tmp_num + '0';
	}
	tmp[5] = ':';
	tmp_num = rawtime->tm_sec;
	if (tmp_num > 10)
	{
		num = OneFromTens(tmp_num);
		tmp[6] = num + '0';
		num = TenToOne(tmp_num);
		tmp[7] = num + '0';
	}else {
		tmp[6] = '0';
		tmp[7] = tmp_num + '0';
	}
	
	strcpy(table, tmp);
}

