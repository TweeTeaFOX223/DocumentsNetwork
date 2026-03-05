/* スペクトラル法 */

#include	<stdio.h>
#include	<stdlib.h>
#include	<math.h>
#include	<time.h>

char	*seVecN, **seMatF; 
int	seDm, seDn, seDc, seDk, *seVecA, **seMatA, *seVecC;
double	**seMatX, *seVecX, *seVecU, **seMatW, *seVecW, *seVecE;
int seVecNCap;

void seCategoryToColor(int category, char color[8])
{
	unsigned int x = (unsigned int)category * 2654435761u;
	unsigned int r = 64u + (x & 0x7Fu);
	unsigned int g = 64u + ((x >> 8) & 0x7Fu);
	unsigned int b = 64u + ((x >> 16) & 0x7Fu);
	snprintf(color, 8, "#%02X%02X%02X", r, g, b);
}

void    seReadValue(const char *fn1)
{
	FILE		*fp;
	int		i, j, k;
	double		v;
	if((fp = fopen(fn1, "r")) == NULL){
		fprintf(stderr, "Unknown File = %s\n", fn1);
		exit(1);
	}
	fscanf(fp, "%d %d %d", &seDm, &seDn, &seDc); 
	seVecA = (int *) malloc(sizeof(int)*seDm);
	seMatA = (int **) malloc(sizeof(int *)*seDm);
	seMatX = (double **) malloc(sizeof(double *)*seDm);
	for(i = 0; i < seDm; i++){
		fscanf(fp, "%d", &seVecA[i]);
		seMatA[i] = (int *) malloc(sizeof(int)*seVecA[i]);
		seMatX[i] = (double *) malloc(sizeof(double)*seVecA[i]);
		for(j = 0; j < seVecA[i]; j++){ 
			fscanf(fp, "%d:%lf", &k, &v);
			seMatA[i][j] = k-1;
			seMatX[i][j] = v;
		}
		fscanf(fp, "%d %d", &j, &k);
	}
	fclose(fp);
	//printf("%d %d %d\n", seDm, seDn, seDc);
}
void    seReadUid(const char *fn1)
{
	FILE		*fp;
	char	c; 
	int		i, j, k;
	if((fp = fopen(fn1, "r")) == NULL){
		fprintf(stderr, "Unknown File = %s\n", fn1);
		exit(1);
	}
	seVecNCap = 1000000;
	seVecN = (char *) malloc(sizeof(char)*seVecNCap);
	seVecC = (int *) malloc(sizeof(int)*seDm);
	seMatF = (char **) malloc(sizeof(char *)*seDm);
	for(i = j = 0; i < seDm; i++){
		fscanf(fp, "%d ", &seVecC[i]);
		while((c = getc(fp)) != '\n'){
			char *newBuf;
			if(j + 1 >= seVecNCap){
				seVecNCap *= 2;
				newBuf = (char *)realloc(seVecN, sizeof(char)*seVecNCap);
				if(newBuf == NULL){
					fprintf(stderr, "Memory allocation failed\n");
					exit(1);
				}
				seVecN = newBuf;
			}
			seVecN[j++] = c;
		}
		seMatF[i] = (char *) malloc(sizeof(char)*(j+1));
		for(k = 0; k < j; k++) seMatF[i][k] = seVecN[k]; 
		seMatF[i][j] = '\0'; j = 0; 
	}
	fclose(fp);
	//printf("%d\n", seDm);
}
void	seNormalizeVector(double *vp, int dim)
{
	int		i;
	double		v;
	for(i = 0, v = 0.0; i < dim; i++) v += vp[i]*vp[i];
	for(i = 0, v = 1.0/sqrt(v); i < dim; i++) vp[i] *= v;
}
void	seInitData()
{
	int		i, j, k;
	double		v, max; 
	seVecX = (double *) malloc(sizeof(double)*seDm);
	for(i = 0, max = 0.0; i < seDm; i++){
		for(j = 0, v = 0.0; j < seVecA[i]; j++) v += seMatX[i][j]; 
		if((seVecX[i] = v) > max) max = v; 
	}
	for(i = 0; i < seDm; i++) seVecX[i] = max - seVecX[i]; 
	seMatW = (double **) malloc(sizeof(double *)*seDk);
	for(k = 0; k < seDk; k++){
		seMatW[k] = (double *) malloc(sizeof(double)*seDm);
		for(i = 0; i < seDm; i++) 
			seMatW[k][i] = (rand()%20000)/10000.0 - 1.0;
		for(i = 0, v = 0.0; i < seDm; i++) v += seMatW[k][i]; 
		for(i = 0, v = v/seDm; i < seDm; i++) seMatW[k][i] -= v; 
		seNormalizeVector(seMatW[k], seDm); 
	}
	seVecU = (double *) malloc(sizeof(double)*seDm);
	seVecE = (double *) malloc(sizeof(double)*seDk);
	//printf("init OK\n"); 
}
double	seCalWeight(int k)
{
	int		i, j;
	double		v, err;
	for(i = 0; i < seDm; i++) seVecU[i] = seVecX[i]; 
	for(i = 0; i < seDm; i++)
		for(j = 0; j < seVecA[i]; j++) 
			seVecU[i] += seMatX[i][j]*seMatW[k][seMatA[i][j]];
	for(i = 0, v = 0.0; i < seDm; i++) v += seVecU[i]; 
	for(i = 0, v = v/seDm; i < seDm; i++) seVecU[i] -= v; 
	for(i = 0, seVecE[k] = 0.0; i < seDm; i++) seVecE[k] += seMatW[k][i]*seVecU[i]; 
	seNormalizeVector(seVecU, seDm);
	for(i = 0, err = 0.0; i < seDm; i++){
		if((v = fabs(seMatW[k][i] - seVecU[i]))) err = v;
		seMatW[k][i] = seVecU[i];
	}
	for(j = 0; j < k; j++){
		for(i = 0, v = 0.0; i < seDm; i++) v += seMatW[k][i]*seMatW[j][i];
		for(i = 0; i < seDm; i++) seMatW[k][i] -= v*seMatW[j][i];
	}
	return(err);
}
void	sePrintValue(const char *fn1, const char *fn2)
{
	FILE	*fp;
	int		i, j, k, x = 1600, y = 700;
	char color[8];
	double	v, w; 
	for(i = 0; i < seDm; i++) if(i == 0) v = w = seMatW[0][i]; else if(v > seMatW[0][i]) v = seMatW[0][i]; else if(w < seMatW[0][i]) w = seMatW[0][i];
	for(i = 0, w = w-v; i < seDm; i++) seMatW[0][i] = (0.98*x*((seMatW[0][i]-v)/w))+(0.01*x);
	for(i = 0; i < seDm; i++) if(i == 0) v = w = seMatW[1][i]; else if(v > seMatW[1][i]) v = seMatW[1][i]; else if(w < seMatW[1][i]) w = seMatW[1][i];
	for(i = 0, w = w-v; i < seDm; i++) seMatW[1][i] = (0.98*y*((seMatW[1][i]-v)/w))+(0.01*y);
	
	// エッジの座標を生成
	fp = fopen(fn1, "w");
	for(i = 0; i < seDm; i++){
		for(j = 0; j < seVecA[i]; j++){
			if((k = seMatA[i][j]) <= i) continue; 
			fprintf(fp, "%f %f %f %f\n", seMatW[0][i], seMatW[1][i], seMatW[0][k], seMatW[1][k]); 
		}
	}
	fclose(fp);
	
//	ノードの座標を生成
	fp = fopen(fn2, "w");
	seCategoryToColor(seVecC[0], color);
	fprintf(fp, "%d %s %f %f 12 %s \n", seVecC[0], seMatF[0], seMatW[0][0], seMatW[1][0], color); 
	for(i = 1; i < seDm; i++){
		seCategoryToColor(seVecC[i], color);
		fprintf(fp, "%d %s %f %f 6 %s \n", seVecC[i], seMatF[i], seMatW[0][i], seMatW[1][i], color);
	}
	fclose(fp); 
}
int	se(const char **argv)
{
	int		i, k;
	double		v, err, err2;
	seDk = 2;
	seReadValue(argv[0]);
	seReadUid(argv[1]);
	seInitData(); 
	for(k = 0, v = 0.0; k < seDk; k++){
		for(i = 0; i < 10000; i++){	
			if((err = seCalWeight(k)) < 1.0e-8) break;
			if(i > 0 && fabs(err-err2) < 1.0e-16) break;
			err2 = err; 
		}  
		v += seVecE[k];   
		//printf("%d %d %e %e\n", k+1, i+1, err, seVecE[k]);
	}  
	sePrintValue(argv[2], argv[3]); 
}
