/* クロスエントロピー法 */

#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<math.h>
#include	<time.h>

char	*ceVecN, **ceMatF; 
unsigned char	ceLabel[50000][64], **ceIn;
int	ceDm, ceDn, ceDk, ceDc, ceDh, ceDs, ceNumL, *ceNumV, **ceAdjM, ceCntV[4], *ceRanV, *ceVecD, **MatD, *ceVecC;
double	*ceVecW, *ceVecW2, *ceDecV, **ceMatW, **ceMatW2, **ceMatG, ceValE[2], **ceMatA, *ceVecB, *ceVecE, *ceVecT, *ceVecR, *ceVecF, ceValR;
int ceVecNCap;

void	ceAllocVector(double **ptr, int dim)
{
	*ptr = (double *) malloc(sizeof(double)*dim);
}	
void	ceAllocMatrix(double ***ptr, int dc, int dr)
{
	int		i;
	*ptr = (double **) malloc(sizeof(double *)*dc);
	for(i = 0; i < dc; i++) 
		(*ptr)[i] = (double *) malloc(sizeof(double)*dr);
}	
double	ceCalNorm(double *vp, int dim)
{
	int		i;
	double	v;
	for(i = 0, v = 0.0; i < dim; i++, vp++) v += (*vp)*(*vp); 
	return(v);
}
double	ceCalProd(double *vp, double *wp, int dim)
{
	int		i;
	double	v;
	for(i = 0, v = 0.0; i < dim; i++, vp++, wp++) v += (*vp)*(*wp); 
	return(v);
}
double	ceCalDist(double *vp, double *wp, int dim)
{
	int		i;
	double	v;
	for(i = 0, v = 0.0; i < dim; i++, vp++, wp++) 
		v += ((*vp)-(*wp))*((*vp)-(*wp)); 
	return(v);
}
int	ceLLdecompose(double **LL, int dim)
{
	int		i, j, k;
	double	t;
	static double Eps = 1.0e-4;
	for(k = 0; k != dim; k++){
		for(i = 0; i != k; i++){
			for(j = 0, t = LL[k][i]; j != i; j++) 
				t -= LL[i][j]*LL[k][j];
			LL[k][i] = t/LL[i][i];
		}
		for(i = 0, t = LL[k][k]; i < k; i++) t -= LL[k][i]*LL[k][i];
		if(t < Eps) return(0); 
		LL[k][k] = sqrt(t);
	}
	return(1);
}
void	ceLLsolve(double **LL, double *B, double *X, int dim)
{
	int		i, j;
	double	t;
	for(i = 0; i != dim; i++){
		for(j = 0, t = B[i]; j != i; j++) t -= LL[i][j]*X[j];
		X[i] = t/LL[i][i];
	}
	for(i = dim-1; i != -1; i--){
		for(j = i+1, t = X[i]; j != dim; j++) t -= LL[j][i]*X[j];
		X[i] = t/LL[i][i];
	}
}
void    cePrintMatrix(double **mp, int dim)
{
	int		i, j;
	for(i = 0; i < dim; i++){
		//for(j = 0; j < dim; j++) printf("%f ", mp[i][j]);
		//printf("\n");
	}
}
void	cePrintValue(const char *fn1, const char *fn2, const char **nodeColor)
{
	FILE	*fp;
	int		i, j, k, x = 1600, y = 700;
	double	v, w; 
	for(i = 0; i < ceDm; i++) if(i == 0) v = w = ceMatW[i][0]; else if(v > ceMatW[i][0]) v = ceMatW[i][0]; else if(w < ceMatW[i][0]) w = ceMatW[i][0];
	for(i = 0, w = w-v; i < ceDm; i++) ceMatW[i][0] = (0.98*x*((ceMatW[i][0]-v)/w))+(0.01*x);
	for(i = 0; i < ceDm; i++) if(i == 0) v = w = ceMatW[i][1]; else if(v > ceMatW[i][1]) v = ceMatW[i][1]; else if(w < ceMatW[i][1]) w = ceMatW[i][1];
	for(i = 0, w = w-v; i < ceDm; i++) ceMatW[i][1] = (0.98*y*((ceMatW[i][1]-v)/w))+(0.01*y);
	
	// エッジの座標を生成
	fp = fopen(fn1, "w");
	for(i = 0; i < ceDm; i++){
		for(j = 0; j < ceVecD[i]; j++){
			if((k = MatD[i][j]) <= i) continue; 
			fprintf(fp, "%f %f %f %f\n", ceMatW[i][0], ceMatW[i][1], ceMatW[k][0], ceMatW[k][1]); 
		}
	}
	fclose(fp);

	// ノードの座標を生成
	fp = fopen(fn2, "w");
	fprintf(fp, "%d %s %f %f 12 %s\n", ceVecC[0], ceMatF[0], ceMatW[0][0], ceMatW[0][1], nodeColor[(ceVecC[0]-1)%64]);
	for(i = 1; i < ceDm; i++)
		fprintf(fp, "%d %s %f %f 6 %s\n", ceVecC[i], ceMatF[i], ceMatW[i][0], ceMatW[i][1], nodeColor[(ceVecC[i]-1)%65]);
	fclose(fp); 
}
void    ceReadValue(const char *fn1)
{
	FILE		*fp;
	int		i, j, k, max;
	double		v;
	if((fp = fopen(fn1, "r")) == NULL) {
		fprintf(stderr, "Unknown File = %s\n", fn1);
		exit(1);
	}
	fscanf(fp, "%d %d %d", &ceDm, &ceDk, &ceDc); 
	ceIn = (unsigned char **) malloc(sizeof(unsigned char *)*ceDm);
	for(i = 0; i < ceDm; i++){
		ceIn[i] = (unsigned char *) malloc(sizeof(unsigned char)*ceDm);
		for(j = 0; j < ceDm; j++) ceIn[i][j] = 0;
	}
	ceVecD = (int *) malloc(sizeof(int)*ceDm);
	MatD = (int **) malloc(sizeof(int *)*ceDm);
	for(i = 0; i < ceDm; i++){
		fscanf(fp, "%d", &ceVecD[i]);
		MatD[i] = (int *) malloc(sizeof(int)*ceVecD[i]);
		for(j = 0; j < ceVecD[i]; j++){ 
			fscanf(fp, "%d:%lf", &k, &v);
			MatD[i][j] = k-1;
			ceIn[i][k-1] = 1;
		}
		fscanf(fp, "%d %d", &j, &k);
	}
	fclose(fp);
	//printf("%d %d %d\n", ceDm, ceDk, ceDc);
}
void    ceReadUid(const char *fn1)
{
	FILE		*fp;
	char	c; 
	int		i, j, k;
	if((fp = fopen(fn1, "r")) == NULL){
		fprintf(stderr, "Unknown File = %s\n", fn1);
		exit(1);
	}
	ceVecNCap = 1000000;
	ceVecN = (char *) malloc(sizeof(char)*ceVecNCap);
	ceVecC = (int *) malloc(sizeof(int)*ceDm);
	ceMatF = (char **) malloc(sizeof(char *)*ceDm);
	for(i = j = 0; i < ceDm; i++){
		fscanf(fp, "%d ", &ceVecC[i]);
		while((c = getc(fp)) != '\n'){
			char *newBuf;
			if(j + 1 >= ceVecNCap){
				ceVecNCap *= 2;
				newBuf = (char *)realloc(ceVecN, sizeof(char)*ceVecNCap);
				if(newBuf == NULL){
					fprintf(stderr, "Memory allocation failed\n");
					exit(1);
				}
				ceVecN = newBuf;
			}
			ceVecN[j++] = c;
		}
		ceMatF[i] = (char *) malloc(sizeof(char)*(j+1));
		for(k = 0; k < j; k++) ceMatF[i][k] = ceVecN[k]; 
		ceMatF[i][j] = '\0'; j = 0; 
	}
	fclose(fp);
	//printf("%d\n", ceDm);
}
void	ceInitWeight(double v, const char * file)
{
	int		h, i, j, k;
	FILE *POS;
	char buff[128];
	char name[64];
	double	value[64];

	ceNumV = (int *) malloc(sizeof(int)*ceDm); 
	for(i = 0; i < ceDm; i++) {
		ceIn[i][i] = 0; 
		for(j = 0, ceNumV[i] = 0; j < ceDm; j++) ceNumV[i] += (int) ceIn[i][j];
	}
	for(i = 1, k = ceNumV[0]; i < ceDm; i++) if(k < ceNumV[i]) k = ceNumV[i]; 
	for(i = ceNumL = 0; i < ceDm; i++) 
		for(j = i+1; j < ceDm; j++) ceNumL += ceIn[i][j]; 
	//printf("link = %d max = %d\n", ceNumL, k); 
	ceAllocVector(&ceVecW, ceDm); ceAllocVector(&ceVecW2, ceDm); 
	for(i = 0; i < ceDm; i++) ceVecW[i] = 1.0; 
	ceRanV = (int *) malloc(sizeof(int)*ceDm); 
	ceAllocMatrix(&ceMatW, ceDm, ceDn); ceAllocMatrix(&ceMatG, ceDm, ceDn); 
	ceAllocMatrix(&ceMatA, ceDn, ceDn); ceAllocVector(&ceVecB, ceDn); 
	ceAllocVector(&ceVecE, ceDm); ceAllocMatrix(&ceMatW2, ceDm, ceDn); 
	ceAllocVector(&ceVecT, ceDn); ceAllocVector(&ceDecV, ceDn); 
	ceAllocVector(&ceVecR, ceDm); ceAllocVector(&ceVecF, ceDm); 
	for(h = 0; h < ceDn; h++) ceDecV[h] = v;
	if(file == NULL){
		for(i = 0; i < ceDm; i++){
			for(h = 0; h < ceDn; h++){
				ceMatW[i][h] = (rand()%10000)/10000.0;
				ceMatW[i][h] *= (rand()%2 == 0) ? 1.0: -1.0;
			}
		}
	} else {
		for(i = 0; i < ceDm; i++){
			for(h = 0; h < ceDn; h++){
				ceMatW[i][h] = (rand()%10000)/10000.0;
				ceMatW[i][h] *= (rand()%2 == 0) ? 1.0: -1.0;
			}
		}
		if((POS = fopen(file, "r")) == NULL){
			exit(0);
		}
		while(fscanf(POS, "%s", name) != EOF){
			for(h = 0; h < ceDn; h++){
				fscanf(POS, "%lf", &value[h]);
			}
			/*printf("%s %f %f %f\n", name, value[0], value[1], value[2]);*/
			for(k = 0; k < ceDm; k++){
				if(strcmp(ceLabel[k], name) == 0){
					break;
				}
			}
			if(k >= ceDm){
				continue;
			}
			for(h = 0; h < ceDn; h++){
				ceMatW[k][h] = value[h];
			}
		}
		fclose(POS);
	}
}
double	ceCalValue()
{
	int		h, i, j;
	double		v, w, z, x, y;
	for(i = 0, z = 0.0; i < ceDm; i++){ 
		for(h = 0; h < ceDn; h++){ 
			z += 0.5*ceDecV[h]*ceMatW[i][h]*ceMatW[i][h];
			ceMatG[i][h] = -ceDecV[h]*ceMatW[i][h]; 
		}
	}
	for(i = 0; i < ceDm; i++){
		for(j = i+1; j < ceDm; j++){
			for(h = 0, v = 0.0; h < ceDn; h++){
				ceVecT[h] = ceMatW[i][h]-ceMatW[j][h]; 
				v += ceVecT[h]*ceVecT[h]; 
			}
			x = ceVecW[i]+ceVecW[j]; v = 0.5*x*v; 
			z += (ceIn[i][j] == 1) ? v: -log(1.0-exp(-v)); 
			w = (ceIn[i][j] == 1) ? -x: x/(exp(v)-1.0); 
			for(h = 0; h < ceDn; h++){
				ceMatG[i][h] += w*ceVecT[h];
				ceMatG[j][h] += -w*ceVecT[h];
			}
		}
	}
	return(z);
}
double	ceCalBenefit(int i)
{
	int		h, j, k;
	double		v, z, x, y;
	for(h = 0, z = 0.0; h < ceDn; h++)
		z += -ceDecV[h]*ceVecB[h]*(ceMatW[i][h]+0.5*ceVecB[h]);
	for(j = 0; j < ceDm; j++){
		if(i == j) continue;
		for(h = 0, v = 0.0; h < ceDn; h++){
			ceVecT[h] = ceMatW[i][h]-ceMatW[j][h]; 
			v += ceVecT[h]*ceVecT[h]; 
		}
		v = 0.5*(ceVecW[i]+ceVecW[j])*v; 
		z += (ceIn[i][j] == 1) ? v: -log(1.0-exp(-v)); 
		for(h = 0, v = 0.0; h < ceDn; h++){
			ceVecT[h] += ceVecB[h]; 
			v += ceVecT[h]*ceVecT[h]; 
		}
		v = 0.5*(ceVecW[i]+ceVecW[j])*v; 
		z -= (ceIn[i][j] == 1) ? v: -log(1.0-exp(-v)); 
	}
	return(z);
}
void	ceUpdateInfo(int i)
{
	int		h, j, k;
	double		v, w, x, y;
	for(h = 0; h < ceDn; h++) ceMatG[i][h] = -ceDecV[h]*(ceMatW[i][h]+ceVecB[h]); 
	for(j = 0; j < ceDm; j++){
		if(i == j) continue;
		for(h = 0, v = 0.0; h < ceDn; h++){
			ceVecT[h] = ceMatW[i][h]-ceMatW[j][h]; 
			v += ceVecT[h]*ceVecT[h]; 
		}
		x = ceVecW[i]+ceVecW[j]; 
		w = (ceIn[i][j] == 1) ? -x: x/(exp(0.5*x*v)-1.0); 
		for(h = 0; h < ceDn; h++) ceMatG[j][h] -= -w*ceVecT[h]; 
		for(h = 0, v = 0.0; h < ceDn; h++){
			ceVecT[h] += ceVecB[h]; 
			v += ceVecT[h]*ceVecT[h]; 
		}
		w = (ceIn[i][j] == 1) ? -x: x/(exp(0.5*x*v)-1.0); 
		for(h = 0; h < ceDn; h++){
			v = w*ceVecT[h]; ceMatG[i][h] += v; ceMatG[j][h] += -v; 
		}
	}
	for(h = 0; h < ceDn; h++) ceMatW[i][h] += ceVecB[h]; 
}
int	ceUpdateWeight(double *max)
{
	int		h, i, j, k;
	double		u, v, w, z, x, y;
	for(i = 0, *max = ceCalNorm(ceMatG[0], ceDn), j = 1; j < ceDm; j++)
		if((v = ceCalNorm(ceMatG[j], ceDn)) > *max){ i = j; *max = v; }
	if(*max < 1.0e-4) return(-1);
	for(h = 0; h < ceDn; h++){
		ceMatA[h][h] = ceDecV[h]; 
		for(k = 0; k < h; k++) ceMatA[h][k] = 0.0; 
	}
	for(j = 0, u = 0.0; j < ceDm; j++){
		if(i == j) continue;
		for(h = 0, v = 0.0; h < ceDn; h++){
			ceVecT[h] = ceMatW[i][h]-ceMatW[j][h]; 
			v += ceVecT[h]*ceVecT[h]; 
		}
		x = ceVecW[i]+ceVecW[j]; v = 0.5*x*v; 
		w = (ceIn[i][j] == 1) ? x: -x/(exp(v)-1.0); 
		u += (ceIn[i][j] == 1) ? w: -w; 
		for(h = 0; h < ceDn; h++) ceMatA[h][h] += w; 
		if(ceIn[i][j] == 1) continue; 
		w = x*x/(exp(v)+exp(-v)-2.0); 
		for(h = 0; h < ceDn; h++)
			for(k = 0; k <= h; k++) 
				ceMatA[h][k] += w*ceVecT[h]*ceVecT[k];
	}
	if(ceLLdecompose(ceMatA, ceDn) == 1){ 
		ceLLsolve(ceMatA, ceMatG[i], ceVecB, ceDn); 
		v = ceCalBenefit(i); 
		if(v > 0.0 && fabs(v) < ceValE[0]){
			ceCntV[0] += 1; 
			return(i); 
		}
		ceCntV[1] += 1; 
	}
	ceCntV[2] += 1;
	for(h = 0; h < ceDn; h++) ceVecB[h] = ceMatG[i][h]/(u+ceDecV[h]);
	return(i);
}
double	ceCalFball()
{
	int		h, i, j, k;
	double		u, v, x, y, max, p, q;
	ceAdjM = (int **) malloc(sizeof(int *)*ceDm); 
	for(i = 0; i < ceDm; i++)
		for(j = 0, ceNumV[i] = 0; j < ceDm; j++) ceNumV[i] += (int) ceIn[i][j];
	for(i = 0, u = 0.0; i < ceDm; i++){
		for(j = k = 0; j < ceDm; j++){
			if(i == j) continue;
			ceRanV[k] = j;
			ceVecE[k++] = ceCalDist(ceMatW[i], ceMatW[j], ceDn); 
		}
		for(j = 1; j < ceDm-1; j++){
			if(ceVecE[j-1] <= ceVecE[j]) continue; 
			v = ceVecE[j-1]; ceVecE[j-1] = ceVecE[j]; ceVecE[j] = v;
			h = ceRanV[j-1]; ceRanV[j-1] = ceRanV[j]; ceRanV[j] = h;
			for(k = j-1; k > 0; k--){
				if(ceVecE[k-1] <= ceVecE[k]) break; 
				v = ceVecE[k-1]; ceVecE[k-1] = ceVecE[k]; ceVecE[k] =v;
				h = ceRanV[k-1]; ceRanV[k-1] = ceRanV[k]; ceRanV[k] =h;
			}
		}
		for(j = k = 0, x = max = 0.0, y = ceNumV[i]; j < ceDm-1; j++){
			x += (int) ceIn[i][ceRanV[j]]; y++; 
			if((j < ceDm-2) && (ceVecE[j] == ceVecE[j+1])) continue; 
			if((v = 2.0*x/y) > max){
				max = v; ceVecR[i] = ceVecE[j]; k = j;
			}
		}
		for(j = 0, p = q = 0.0; j < ceDm; j++){
			if(i == j) continue;
			for(h = 0, v = 0.0; h < ceDn; h++){
				ceVecT[h] = ceMatW[i][h]-ceMatW[j][h]; 
				v += 0.5*ceVecT[h]*ceVecT[h]; 
			}
			if(ceIn[i][j] == 1) p += v; else q -= log(1.0-exp(-v)); 
		}
		/*
		printf("%s\t%2d %f %f %f\n", ceLabel[i], ceNumV[i], max, p, q); 
		if(max != 1.0){
			for(j = 0; j <= k; j++) printf("%s ", ceLabel[ceRanV[j]]);
			printf("\n");
		}
		*/
		ceVecF[i] = max; 
		u += max; 
		ceNumV[i] = k+2; 
		ceAdjM[i] = (int *) malloc(sizeof(int)*ceNumV[i]); 
		for(j = 0; j <= k; j++) ceAdjM[i][j] = ceRanV[j];
		ceAdjM[i][k+1] = i;
	}
	return(u/ceDm);
}
int	ceMyMember(int i, int *ip, int k)
{
	int		h;
	for(h = 0; h < k; h++) if(i == ip[h]) return(1);
	return(0);
}
int	ceSetCovering(double v)
{
	int		h, i, j, k, s;
	double		max;
	for(k = 0; k < ceDm; k++){
		for(i = s = 0, max = 0.0; i < ceDm; i++)
			if(ceMyMember(i, ceRanV, k) == 0 && max < ceVecF[i]*ceNumV[i]){
				max = ceVecF[i]*ceNumV[i]; s = i; 
			}
		if(max == 0.0) break; 
		ceRanV[k] = s; 
		for(i = 0; i < ceDm; i++){
			if(ceMyMember(i, ceRanV, k+1) == 1) continue;
			for(j = h = 0; j < ceNumV[i]; j++)
				if(ceMyMember(ceAdjM[i][j], ceAdjM[s], ceNumV[s]) == 0
				   && ceMyMember(ceAdjM[i][j], ceRanV, k+1) == 0)
					ceAdjM[i][h++] = ceAdjM[i][j];
			ceNumV[i] = h;
		}
	}
	/*
	for(i = 0; i < ceDm; i++)
		printf("%s: %f\n", ceLabel[i], ceVecF[i]); 
	printf("Num = %d\n", k); 
	for(i = 0; i < k; i++){
		printf("%s: (%f)", ceLabel[ceRanV[i]], ceVecF[ceRanV[i]]); 
		for(j = 0; j < ceNumV[ceRanV[i]]; j++)
			printf("%s ", ceLabel[ceAdjM[ceRanV[i]][j]]); 
		printf("\n"); 
	}
	*/
	return(k);
}
int	ce(const char **argv, const char** commonNodeColor)
{
	char		name[256];
	int		h, i, j, k, itr, argc = 4;
	clock_t		tv;
	double		v, w, x, y, z, max;
	srand(time(0));
	tv = clock();
	ceValR = 1.0;
	ceDh = 0; 
	ceDn = 2; /* [2 or 3] */
	v = 0.0;  /* 1.0e-8 */
	ceReadValue(argv[0]);
	ceReadUid(argv[1]);
	if(argc == 5){
		ceInitWeight(v, argv[3]);
	} else {
		ceInitWeight(v, NULL);
	}
	ceValE[1] = -1.0;
	for(h = 0; h < 1; h++){
		ceValE[0] = ceCalValue();
		//printf("E = %f\n", ceValE[0]); 
		fflush(stdout);
		ceCntV[0] = ceCntV[1] = ceCntV[2] = ceCntV[3] = 0;
		for(i = 0; i < 10000000; i++){
			if((k = ceUpdateWeight(&max)) == -1) break; 
			ceUpdateInfo(k);
			/*
			ceValE[0] = ceCalValue();
			if((i%10000) == 0){
				printf("No.%d D = %.8f (%d) %d %d %d\n", 
				i, max, k+1, ceCntV[0], ceCntV[1], ceCntV[2]);
				fflush(stdout);
			}
			*/
		}
		ceValE[0] = ceCalValue();
		y = 1.0*(clock()-tv)/CLOCKS_PER_SEC;
		z =  ceCalFball();
		k = ceSetCovering(z);
		for(i = 0, x = 0.0; i < k; i++) x += ceVecF[ceRanV[i]]; 
		//printf("Result: %d %f %f %f %d %f\n", ceDn, ceValE[0], z, y, k, x/k);
	}
	cePrintValue(argv[2], argv[3], commonNodeColor);
	return 0; 
}
