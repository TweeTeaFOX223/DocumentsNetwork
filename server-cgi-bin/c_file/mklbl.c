#include	<stdio.h>
#include	<stdlib.h>
#include	<math.h>
#include	<time.h>
#include	<string.h>

char	*mklblVecN, **mklblMatM; 
int		mklblDm, mklblDn, *mklblVecA, **mklblMatA, *mklblVecS, *mklblVecT; 
int     mklblVecNCap;


void    mklblReadValue0(const char *fn1)
{
	FILE		*fp;
	int		c, i, j, k;
	if((fp = fopen(fn1, "r")) == NULL) { fprintf(stderr, "Unknown File = %s\n", fn1); exit(1);}
	mklblDn = 0;
	while((c = getc(fp)) != EOF) if(c == '\n') mklblDn +=1; 
	fclose(fp); 
	mklblVecNCap = 4096;
	mklblVecN = (char *) malloc(sizeof(char)*mklblVecNCap);
	mklblMatM = (char **) malloc(sizeof(char *)*mklblDn);
	fp = fopen(fn1, "r"); 
	for(i = 0; i < mklblDn; i++){
		while((c = getc(fp)) != EOF) if(c == ' ') break; 
		for(j = 0; (c = getc(fp)) != EOF && c != ' '; j++){
			char *newBuf;
			if(j + 1 >= mklblVecNCap){
				mklblVecNCap *= 2;
				newBuf = (char *)realloc(mklblVecN, sizeof(char)*mklblVecNCap);
				if(newBuf == NULL){
					fprintf(stderr, "Memory allocation failed\n");
					exit(1);
				}
				mklblVecN = newBuf;
			}
			mklblVecN[j] = c;
		}
		while((c = getc(fp)) != EOF) if(c == '\n') break; 
		mklblMatM[i] = (char *) malloc(sizeof(char)*(j+1));
		for(k = 0; k < j; k++) mklblMatM[i][k] = mklblVecN[k]; 
		mklblMatM[i][j] = '\0'; 
	}
	fclose(fp);
}
int		mklblSearch(char *np, int b, int e)
{
	int		c, v;
	if(b == e) return(-1); 
	c = b + ((e-b)/2); 
	v = strcmp(np, mklblMatM[c]);
	if(v == 0) return(c); 
	else if(v < 0) return(mklblSearch(np, b, c)); 
	else return(mklblSearch(np, c+1, e));
}
void	mklblReadValue(const char *fn1)
{
	FILE	*fp;
	int		c, i, j, k;  
	if((fp = fopen(fn1, "r")) == NULL){ fprintf(stderr, "Unknown File = %s\n", fn1); return;}
	mklblDm = 0; 
	while((c = getc(fp)) != EOF) if(c == '\n') mklblDm +=1; 
	fclose(fp); 
	mklblVecA = (int *) malloc(sizeof(int)*mklblDm);
	mklblMatA = (int **) malloc(sizeof(int *)*mklblDm);
	fp = fopen(fn1, "r"); 
	i = j = k = 0; 
	while((c = getc(fp)) != EOF){ 
		if(c == ' '){ 
			if(j > 0) {mklblVecN[j] = '\0'; mklblVecS[k++] = mklblSearch(mklblVecN, 0, mklblDn); j = 0; } 
		}
		else if(c == '\n'){
			if(j > 0) { mklblVecN[j] = '\0'; mklblVecS[k++] = mklblSearch(mklblVecN, 0, mklblDn); } 
			mklblVecA[i] = k; 
			mklblMatA[i] = (int *) malloc(sizeof(int)*k);
			for(j = 0; j < k; j++) mklblMatA[i][j] = mklblVecS[j]; 
			i++; j = k = 0; 
		}
		else {
			char *newBuf;
			if(j + 1 >= mklblVecNCap){
				mklblVecNCap *= 2;
				newBuf = (char *)realloc(mklblVecN, sizeof(char)*mklblVecNCap);
				if(newBuf == NULL){
					fprintf(stderr, "Memory allocation failed\n");
					exit(1);
				}
				mklblVecN = newBuf;
			}
			mklblVecN[j++] = c;
		}
	}
	fclose(fp);
}
void	mklblPrintValue(const char *fn1)
{
	FILE	*fp;
	int		h, i, j, k;
	for(i = 0; i < mklblDn; i++) mklblVecT[i] = 0; 
	fp = fopen(fn1, "w"); 
	fprintf(fp, "%d %d 1\n", mklblDm, mklblDn); 
	for(i = 0; i < mklblDm; i++){ 
		for(j = k = 0; j < mklblVecA[i]; j++){ 
			h = mklblMatA[i][j]; 
			if(mklblVecT[h] != 0){ mklblVecT[h] += 1; continue; }
			mklblVecS[k++] = h; 
			mklblVecT[h] = 1;
		}
		fprintf(fp, "%d ", k); 
		for(j = 0; j < k; j++){ 
			h = mklblVecS[j]; 
			fprintf(fp, "%d:%d ", h+1, mklblVecT[h]); 
			mklblVecT[h] = 0;
		}
		fprintf(fp, "\n"); 
	}
	fclose(fp);
}
int	mklbl(const char **argv)
{
	int		i, j, k;
	mklblReadValue0(argv[0]);
	mklblVecS = (int *) malloc(sizeof(int)*mklblDn); 
	mklblVecT = (int *) malloc(sizeof(int)*mklblDn); 
	mklblReadValue(argv[1]);
	mklblPrintValue(argv[2]);
	return 0; 
}
