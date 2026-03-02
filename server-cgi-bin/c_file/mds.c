/* 多次元尺度法 */

#include	<stdio.h>
#include	<stdlib.h>
#include	<math.h>
#include	<time.h>

char	*mdsVecN, **mdsMatF; 
int		mdsDm, mdsDn, mdsDc, mdsDs, *mdsVecA, **mdsMatA, *mdsVecT, *mdsVecC; 
double	**mdsMatX, **mdsMatG, *mdsVecW, **mdsMatW, *mdsVecE, mdsValE; 
int     mdsVecNCap;

void mdsCategoryToColor(int category, char color[8])
{
	unsigned int x = (unsigned int)category * 2654435761u;
	unsigned int r = 64u + (x & 0x7Fu);
	unsigned int g = 64u + ((x >> 8) & 0x7Fu);
	unsigned int b = 64u + ((x >> 16) & 0x7Fu);
	snprintf(color, 8, "#%02X%02X%02X", r, g, b);
}

void    mdsReadValue(const char *fn1)
{
	FILE		*fp;
	int		i, j, k;
	double		v;
	if((fp = fopen(fn1, "r")) == NULL) {
		fprintf(stderr, "Unknown File = %s\n", fn1);
		exit(1);
	}
	fscanf(fp, "%d %d %d", &mdsDm, &mdsDn, &mdsDc); 
	mdsVecA = (int *) malloc(sizeof(int)*mdsDm);
	mdsMatA = (int **) malloc(sizeof(int *)*mdsDm);
	mdsMatX = (double **) malloc(sizeof(double *)*mdsDm);
	for(i = 0; i < mdsDm; i++){
		fscanf(fp, "%d", &mdsVecA[i]);
		mdsMatA[i] = (int *) malloc(sizeof(int)*mdsVecA[i]);
		mdsMatX[i] = (double *) malloc(sizeof(double)*mdsVecA[i]);
		for(j = 0; j < mdsVecA[i]; j++){ 
			fscanf(fp, "%d:%lf", &k, &v);
			mdsMatA[i][j] = k-1;
			mdsMatX[i][j] = v;
		}
		fscanf(fp, "%d %d", &j, &k);
	}
	fclose(fp);
	for(i = 0, mdsDn = 0; i < mdsDm; i++) mdsDn += mdsVecA[i]; 
	//printf("%d %d %d\n", mdsDm, mdsDn/2, mdsDc);
}
void    mdsReadUid(const char *fn1)
{
	FILE		*fp;
	char	c; 
	int		i, j, k;
	if((fp = fopen(fn1, "r")) == NULL){
		fprintf(stderr, "Unknown File = %s\n", fn1);
		exit(1);
	}
	mdsVecNCap = 1000000;
	mdsVecN = (char *) malloc(sizeof(char)*mdsVecNCap);
	mdsVecC = (int *) malloc(sizeof(int)*mdsDm);
	mdsMatF = (char **) malloc(sizeof(char *)*mdsDm);
	for(i = j = 0; i < mdsDm; i++){
		fscanf(fp, "%d ", &mdsVecC[i]);
		while((c = getc(fp)) != '\n'){
			char *newBuf;
			if(j + 1 >= mdsVecNCap){
				mdsVecNCap *= 2;
				newBuf = (char *)realloc(mdsVecN, sizeof(char)*mdsVecNCap);
				if(newBuf == NULL){
					fprintf(stderr, "Memory allocation failed\n");
					exit(1);
				}
				mdsVecN = newBuf;
			}
			mdsVecN[j++] = c;
		}
		mdsMatF[i] = (char *) malloc(sizeof(char)*(j+1));
		for(k = 0; k < j; k++) mdsMatF[i][k] = mdsVecN[k]; 
		mdsMatF[i][j] = '\0'; j = 0; 
	}
	fclose(fp);
	//printf("%d\n", mdsDm);
}
void	mdsCalDistance()
{
	int		d, i, j, k, h1, h2, s, f;
	double		v;
	mdsMatG = (double **) malloc(sizeof(double *)*mdsDm);
	for(i = 0; i < mdsDm; i++)
		mdsMatG[i] = (double *) malloc(sizeof(double)*mdsDm);
	mdsVecT = (int *) malloc(sizeof(int)*mdsDm);
	for(i = 0, f = 1; i < mdsDm; i++){ 
		for(j = 0; j < mdsDm; j++) mdsMatG[i][j] = -1; 
		for(d = 1, mdsMatG[i][i] = 0, mdsVecT[0] = i, h1 = 0, h2 = 1; ; d++){ 
			for(j = h1, h1 = h2; j < h2; j++){ 
				for(k = 0; k < mdsVecA[mdsVecT[j]]; k++){ 
					s = mdsMatA[mdsVecT[j]][k];
					if(mdsMatG[i][s] == -1){ 
						mdsMatG[i][s] = d;
						mdsVecT[h1++] = s;
					}
				}
			}
			if(h1 == mdsDm || h1 == h2) break;
			j = h1; h1 = h2; h2 = j; 
		}
		for(j = k = 0, v = 0.0; j < mdsDm; j++) 
			if(mdsMatG[i][j] > 0){ v += mdsMatG[i][j]; k++;}
		if(k != mdsDm-1) f = 0; 
	}
	if(f == 0) fprintf(stderr, "*** Warning Disconnected Graph ***\n"); 
}
void	mdsInitWeight()
{
	int		i, j;
	double		v;
	for(i = 0; i < mdsDm; i++){
		for(j = 0, v = 0.0; j < mdsDm; j++) v += mdsMatG[i][j]; 
		for(j = 0, v = v/mdsDm; j < mdsDm; j++) mdsMatG[i][j] -= v; 
	}
	for(i = 0; i < mdsDm; i++){
		for(j = 0, v = 0.0; j < mdsDm; j++) v += mdsMatG[j][i]; 
		for(j = 0, v = v/mdsDm; j < mdsDm; j++) mdsMatG[j][i] -= v; 
	}
	mdsVecE = (double *) malloc(sizeof(double)*mdsDs);
	for(i = 0, mdsValE = 0.0; i < mdsDm; i++) mdsValE -= mdsMatG[i][i]; 
	mdsVecW = (double *) malloc(sizeof(double)*mdsDm);
	mdsMatW = (double **) malloc(sizeof(double *)*mdsDs);
	for(i = 0; i < mdsDs; i++){ 
		mdsMatW[i] = (double *) malloc(sizeof(double)*mdsDm);
		for(j = 0; j < mdsDm; j++){ 
			v = (rand()%10000)/10000.0; 
			if(rand()%2 == 0) v = -v; 
			mdsMatW[i][j] = v; 
		}
		for(j = 0, v = 0.0; j < mdsDm; j++) v += mdsMatW[i][j]*mdsMatW[i][j];
		for(j = 0, v = sqrt(1.0/v); j < mdsDm; j++) mdsMatW[i][j] *= v;
	}
}
double	mdsCalVector(int k)
{
	int		i, j;
	double		v, err;
	for(i = 0; i < mdsDm; i++){ 
		for(j = 0, v = 0.0; j < mdsDm; j++) v -= mdsMatG[i][j]*mdsMatW[k][j]; 
		mdsVecW[i] = v; 
	}
	for(j = 0, mdsVecE[k] = 0.0; j < mdsDm; j++) mdsVecE[k] += mdsVecW[j]*mdsMatW[k][j];
	for(j = 0, v = 0.0; j < mdsDm; j++) v += mdsVecW[j]*mdsVecW[j];
	for(j = 0, v = sqrt(1.0/v); j < mdsDm; j++) mdsVecW[j] *= v;
	for(j = 0, err = 0.0; j < mdsDm; j++){
		v = fabs(mdsMatW[k][j] - mdsVecW[j]);
		if(i == 0 || v > err) err = v;
		mdsMatW[k][j] = mdsVecW[j];
	}
	for(i = 0; i < k; i++){
		for(j = 0, v = 0.0; j < mdsDm; j++) v += mdsMatW[k][j]*mdsMatW[i][j];
		for(j = 0; j < mdsDm; j++) mdsMatW[k][j] -= v*mdsMatW[i][j];
	}
	for(j = 0, v = 0.0; j < mdsDm; j++) v += mdsMatW[k][j]*mdsMatW[k][j];
	for(j = 0, v = sqrt(1.0/v); j < mdsDm; j++) mdsMatW[k][j] *= v;
	return(err); 
}
void	mdsPrintValue(const char *fn1, const char *fn2)
{
	FILE	*fp;
	int		i, j, k, x = 1600, y = 700;
	char color[8];
	double	v, w; 
	for(i = 0; i < mdsDm; i++) if(i == 0) v = w = mdsMatW[0][i]; else if(v > mdsMatW[0][i]) v = mdsMatW[0][i]; else if(w < mdsMatW[0][i]) w = mdsMatW[0][i];
	for(i = 0, w = w-v; i < mdsDm; i++) mdsMatW[0][i] = (0.98*x*((mdsMatW[0][i]-v)/w))+(0.01*x);
	for(i = 0; i < mdsDm; i++) if(i == 0) v = w = mdsMatW[1][i]; else if(v > mdsMatW[1][i]) v = mdsMatW[1][i]; else if(w < mdsMatW[1][i]) w = mdsMatW[1][i];
	for(i = 0, w = w-v; i < mdsDm; i++) mdsMatW[1][i] = (0.98*y*((mdsMatW[1][i]-v)/w))+(0.01*y);
	
	// エッジの座標を生成
	fp = fopen(fn1, "w"); 
	for(i = 0; i < mdsDm; i++){
		for(j = 0; j < mdsVecA[i]; j++){
			if((k = mdsMatA[i][j]) <= i) continue; 
			fprintf(fp, "%f %f %f %f\n", mdsMatW[0][i], mdsMatW[1][i], mdsMatW[0][k], mdsMatW[1][k]); 
		}
	}
	fclose(fp);

	// ノードの座標を生成
	fp = fopen(fn2, "w");
	mdsCategoryToColor(mdsVecC[0], color);
	fprintf(fp, "%d %s %f %f 12 %s\n", mdsVecC[0], mdsMatF[0], mdsMatW[0][0], mdsMatW[1][0], color); 
	for(i = 1; i < mdsDm; i++){
		mdsCategoryToColor(mdsVecC[i], color);
		fprintf(fp, "%d %s %f %f 6 %s\n", mdsVecC[i], mdsMatF[i], mdsMatW[0][i], mdsMatW[1][i], color); 
	}
	fclose(fp); 
}
int	mds(const char **argv)
{
	int		i, k;
	double		err, v;
	mdsDs = 2;
	mdsReadValue(argv[0]);
	mdsReadUid(argv[1]);
	mdsCalDistance();
	mdsInitWeight();
	for(k = 0, v = 0.0; k < mdsDs; k++){
		for(i = 0; i < 100; i++){	
			if((err = mdsCalVector(k)) < 1.0e-8) break;
//			printf("%d %e %e\n", i+1, err, mdsVecE[k]); 
		}  
		v += mdsVecE[k]; 
		//printf("%d %e %e %e\n", i+1, err, mdsVecE[k], 100.0*v/mdsValE); 
	}  
	mdsPrintValue(argv[2], argv[3]);
	return 0;
}
