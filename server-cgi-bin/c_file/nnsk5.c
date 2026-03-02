#include <stdio.h>
#include <stdlib.h>
#include <math.h>

char    *nnskVecN, **nnskMatL, **nnskMatM, **nnskMatN;
int	     nnskN, nnskV, nnskC, nnskK, *nnskVecA, **nnskMatA, *nnskVecB, **nnskMatB, *nnskVecC, **nnskMatQ, *nnskVecH, *nnskVecS, *nnskVecT;
double **nnskMatX, **nnskMatY, *nnskVecW, *nnskVecZ;
int      nnskVecNCap;


/*lbl.txtの内容を記録する*/
void nnskReadValue(const char *fn1)
{
	FILE   *fp;
	int    i, j, k;
	double v;

//	ファイルが無い場合は異常終了する
	if((fp = fopen(fn1, "r")) == NULL)
	{
		fprintf(stderr, "Unknown File = %s\n", fn1); 
		exit(1);
	}
	fscanf(fp, "%d %d %d", &nnskN, &nnskV, &nnskC); // 文書数:7367, 単語数:75024, カテゴリー数:1

	nnskVecA = (int *)     malloc(sizeof(int)*nnskN);
	nnskMatA = (int **)    malloc(sizeof(int *)*nnskN);
	nnskMatX = (double **) malloc(sizeof(double *)*nnskN);

//	単語ID,出現回数の割り当て
	for(i = 0; i < nnskN; i++)
	{
		fscanf(fp, "%d", &nnskVecA[i]);
		nnskMatA[i] = (int *)    malloc(sizeof(int)*nnskVecA[i]);
		nnskMatX[i] = (double *) malloc(sizeof(double)*nnskVecA[i]);
		
		for(j = 0; j < nnskVecA[i]; j++)
		{ 
			fscanf(fp, "%d:%lf", &k, &v); // :の左側はint型,:の右側はdouble型で読み込む
			nnskMatA[i][j] = k-1;
			nnskMatX[i][j] = v;
		}
	}

	fclose(fp);
}


/*uid.txtの内容を読み込む*/
void nnskReadName(const char *fn1)
{
	FILE	*fp;
	int		c, i, j, k;
	
	if((fp = fopen(fn1, "r")) == NULL)
	{ 
		fprintf(stderr, "Unknown File = %s\n", fn1); 
		exit(1);
	}

//	配列のサイズを割り当てる(Nは文書数)
	nnskVecNCap = 1000000;
	nnskVecN = (char *)  malloc(sizeof(char)*nnskVecNCap);
	nnskVecC = (int *)   malloc(sizeof(int)*nnskN);
	nnskMatN = (char **) malloc(sizeof(char *)*nnskN);
	
//	VecCにカテゴリー番号1～8を格納
	for(i = j = 0; i < nnskN; i++)
	{
		fscanf(fp, "%d ", &nnskVecC[i]);
/*		
		VecCにはカテゴリー番号1～8が格納される
		
		1 dokujo-tsushin
		2 it-life-hack
		3 kaden-channel
		4 livedoor-homme
		5 movie-enter
		6 peachy
		7 smax
		8 sports-watch
		9 topic-news
*/
		while((c = getc(fp)) != '\n'){
			char *newBuf;
			if(j + 1 >= nnskVecNCap){
				nnskVecNCap *= 2;
				newBuf = (char *)realloc(nnskVecN, sizeof(char)*nnskVecNCap);
				if(newBuf == NULL){
					fprintf(stderr, "Memory allocation failed\n");
					exit(1);
				}
				nnskVecN = newBuf;
			}
			nnskVecN[j++] = c;
		}
		
		nnskMatN[i] = (char *) malloc(sizeof(char)*(j+1));
		for(k = 0; k < j; k++) 
			nnskMatN[i][k] = nnskVecN[k]; 
		nnskMatN[i][j] = '\0'; j = 0; 
	}
	
	fclose(fp);
}


/* 単語を記録する関数 */
void nnskReadWord(const char *fn1)
{
	FILE	*fp;
	int		c, i, j, k;
	
	if((fp = fopen(fn1, "r")) == NULL)
	{ 
		fprintf(stderr, "Unknown File = %s\n", fn1); 
		exit(1);
	}
	
	nnskMatM = (char **) malloc(sizeof(char *)*nnskV);
	for(i = j = 0; i < nnskV; i++)
	{
		fscanf(fp, "%d ", &c);
		while((c = getc(fp)) != ' '){
			char *newBuf;
			if(j + 1 >= nnskVecNCap){
				nnskVecNCap *= 2;
				newBuf = (char *)realloc(nnskVecN, sizeof(char)*nnskVecNCap);
				if(newBuf == NULL){
					fprintf(stderr, "Memory allocation failed\n");
					exit(1);
				}
				nnskVecN = newBuf;
			}
			nnskVecN[j++] = c; 
		}
		
		nnskMatM[i] = (char *) malloc(sizeof(char)*(j+1));
		for(k = 0; k < j; k++) 
			nnskMatM[i][k] = nnskVecN[k]; 
		nnskMatM[i][j] = '\0'; 
		j          = 0; 
		
		fscanf(fp, "%d\n", &c);
	}
	
	fclose(fp);
}


/* 文章を記録する関数 */
void nnskReadDoc(const char *fn1)
{
	FILE	*fp;
	int		c, i, j, k;
	
	if((fp = fopen(fn1, "r")) == NULL)
	{ 
		fprintf(stderr, "Unknown File = %s\n", fn1); 
		exit(1);
	}
	
	nnskMatL = (char **) malloc(sizeof(char *)*nnskN);
	for(i = j = 0; i < nnskN; i++)
	{
		while((c = getc(fp)) != '\n'){
			char *newBuf;
			if(j + 1 >= nnskVecNCap){
				nnskVecNCap *= 2;
				newBuf = (char *)realloc(nnskVecN, sizeof(char)*nnskVecNCap);
				if(newBuf == NULL){
					fprintf(stderr, "Memory allocation failed\n");
					exit(1);
				}
				nnskVecN = newBuf;
			}
			nnskVecN[j++] = c; 
		}
		
		nnskMatL[i] = (char *) malloc(sizeof(char)*(j+1));
		for(k = 0; k < j; k++) 
			nnskMatL[i][k] = nnskVecN[k]; 
		nnskMatL[i][j] = '\0'; j = 0; 
	}
	
	fclose(fp);
}


void nnskInitData()
{
	int	i, j;

//	配列のサイズを割り当てる(Vは全文書の総単語数)
	nnskVecB = (int *)     malloc(sizeof(int)*nnskV);
	nnskMatB = (int **)    malloc(sizeof(int *)*nnskV);
	nnskMatY = (double **) malloc(sizeof(double *)*nnskV);
	
	for(j = 0; j < nnskV; j++) 
		nnskVecB[j] = 0; 
	
	for(i = 0; i < nnskN; i++)
		for(j = 0; j < nnskVecA[i]; j++) 
			nnskVecB[nnskMatA[i][j]]++; // 単語ID nnskMatA[i][j] の単語が出てきたならば、単語の文書頻度(Document Frequency)+1
	 
// 	nnskMatB,MatYの要素にメモリを割り当てる
	for(j = 0; j < nnskV; j++)
	{
		nnskMatB[j] = (int *)    malloc(sizeof(int)*nnskVecB[j]);
		nnskMatY[j] = (double *) malloc(sizeof(double)*nnskVecB[j]); 
	}
	
	for(i = nnskC = 0; i < nnskN; i++)
	{ 
		if(nnskVecC[i] > nnskC)
			nnskC = nnskVecC[i]; 
		nnskVecC[i]--; 
	}
	
//	MatQにメモリを割り当てる
	nnskMatQ = (int **) malloc(sizeof(int *)*nnskC);
	for(i = 0; i < nnskC; i++){ 
		nnskMatQ[i] = (int *) malloc(sizeof(int)*nnskC); 
		for(j = 0; j < nnskC; j++) nnskMatQ[i][j] = 0; 
	}
	
//	nnskVecW, VecZにメモリを割り当てる
	nnskVecW = (double *) malloc(sizeof(double)*nnskV); // 単語数次元
	nnskVecZ = (double *) malloc(sizeof(double)*nnskN); // 文書数次元
}


void nnskCalTFIDF()// ベクトル空間, 線形空間
{
	int	   i, j, z, count = 0;
	double v;
	
//	IDF の計算
	for(j = 0, v = log(1.0*nnskN); j < nnskV; j++) 
		nnskVecW[j] = v;
	
	for(j = 0; j < nnskV; j++) 
		nnskVecW[j] -= log(1.0*nnskVecB[j]); // logの割算を、減算を繰り返すことで実現している
	
//	i < 文書数
	for(i = 0; i < nnskN; i++) // ∑(Ai * Bi * Ci * ...) / (√∑Ai^2 * √∑Bi^2, √∑Ci^2, ... )
	{
//		全単語の TF-IDF を計算
		for(j = 0; j < nnskVecA[i]; j++)
			nnskMatX[i][j] = nnskMatX[i][j] * nnskVecW[nnskMatA[i][j]]; // TF * IDF ← 文書 i における単語 j の TF-IDF

//		ノルムを1にするための正規化(過学習を抑制できる)
		for(j = 0, v = 0.0; j < nnskVecA[i]; j++)
			v += nnskMatX[i][j] * nnskMatX[i][j]; // v = v + MatXi[TF-IDF]^2 (MatXiは文書) ← 文書 i における全単語の TF-IDF を v に加算
		
		v = 1.0 / sqrt(v); // 1.0 / √(∑MatX(i,j)[TF-IDF]^2) (i = 固定値, 0 <= j <= nnskN)
		
//		全単語に対して正規化したTF-IDFベクトルを計算
		for(j = 0; j < nnskVecA[i]; j++)
			nnskMatX[i][j] = nnskMatX[i][j] * v; // (文書 i における単語 j の TF-IDF) / √(文書 i における全単語の TF-IDF)
	}
}


/* 転置ファイルの作成 */
void nnskCalInverted()
{
	int	i, j, k;
	
//	繰り返し(j < 全文書の総単語数)
	for(j = 0; j < nnskV; j++) 
		nnskVecB[j] = 0; 
	
//	繰り返し(i < 文書数)
	for(i = 0; i < nnskN; i++)
	{
//		繰り返し(j < 任意の文書の単語数)
		for(j = 0; j < nnskVecA[i]; j++)
		{
			k                = nnskMatA[i][j]; // k = 単語ID
			nnskMatB[k][nnskVecB[k]] = i;          // 任意の単語が存在する文書ID (最初はMatB[単語ID][0]に代入される)
			nnskMatY[k][nnskVecB[k]] = nnskMatX[i][j]; // 正規化したTF-IDFベクトル (最初はMatY[単語ID][0]に代入される)
			nnskVecB[k]++;                     // 登録した文書IDが上書きされないように、MatB[単語ID][1]を参照させる
		}
	}
	
/*
	nnskVecA : 文書iに出現する単語の種類数
	nnskVecB : 単語 k が出現する文書数
	nnskMatA : 単語ID
	nnskMatB : 文書ID
	nnskMatX : 正規化したTF-IDFベクトル
	nnskMatY : 正規化したTF-IDFベクトル
*/
}


void nnskCalValue(int i)
{
	int		j, k, h, s;
	double	v, w, x;
	
	nnskVecH = (int *) malloc(sizeof(int)*nnskK);
	nnskVecS = (int *) malloc(sizeof(int)*nnskK);
	
	for(j = 0; j < nnskV; j++)
		nnskVecW[j] = 0.0; 
		
//	正規化したTF-IDFベクトルを代入
	for(j = 0; j < nnskVecA[i]; j++) 
		nnskVecW[nnskMatA[i][j]] = nnskMatX[i][j]; // nnskVecW = 正規化したTF-IDFベクトル
		
//	文書数まで繰り返し
	for(j = 0; j < nnskN; j++) 
		nnskVecZ[j] = 0.0; 
		
//	コサイン類似度を計算
	for(j = 0; j < nnskVecA[i]; j++)
	{
		h = nnskMatA[i][j]; // h = 単語ID
		v = nnskMatX[i][j]; // v = 正規化したTF-IDFベクトル
			
//		繰り返し(k < 単語 h が出現する文書数)
		for(k = 0; k < nnskVecB[h]; k++)
			nnskVecZ[nnskMatB[h][k]] += v * nnskMatY[h][k]; // nnskVecZ[DF] = nnskVecZ[DF] + nnskMatX(i,j) * nnskMatY(h,k) ← nnskMatX, nnskMatY は正規化したTF-IDFベクトル
/*
			nnskVecB : 一つの任意の単語が何個の文章に出現するのか登録されている
			nnskVecZ : コサイン類似度
			nnskMatB : i(0 <= i < 文書数)が登録されている
			nnskMatY : 正規化したTF-IDFベクトル
			v    : 正規化したTF-IDFベクトル
*/
	}
		
//	繰り返し(k < 2)
	for(k = 0; k < nnskK; k++)
	{
//		繰り返し(j < 文書数)
		for(j = h = 0, v = 0.0; j < nnskN; j++)
		{
//			よりコサイン類似度が高い文書が見つかった場合
			if(nnskVecZ[j] > v)
			{
				v = nnskVecZ[j]; // よりコサイン類似度が高いベクトルに変更
				h = j;       // よりコサイン類似度が高い文書IDに変更
			}
		}

//		繰り返し(j < 単語数)
		for(w = 0.0, j = s = 0; j < nnskVecA[h]; j++)
		{
			x = nnskVecW[nnskMatA[h][j]] * nnskMatX[h][j]; // nnskVecW[単語ID] * 正規化したTF-IDFベクトル
			
//			既存のTF-IDF(w)よりも大きいTF-IDF(x)を持つ単語が現れたら単語ID(s)を更新
			if(x > w)
			{
				w = x;          // より類似度貢献度が高いTF-IDFベクトルに変更
				s = nnskMatA[h][j]; // より類似度貢献度が高い単語IDに変更
			}
		}
			
		nnskVecZ[h] = 0.0; // 既に記録した類似度を0に更新(2回記録されるのを防ぐため)
		nnskVecH[k] = h;   // 最もコサイン類似度が高い文書ID
		nnskVecS[k] = s;   // 最も類似度貢献度が高い単語ID
	}
}


void nnskPrintValue(const char *fn1, const char *fn2)
{
	FILE	*fp1;
	int		h, i, j, k;
	
	nnskVecT = (int *) malloc(sizeof(int)*nnskV);
	for(j = 0; j < nnskV; j++) 
		nnskVecT[j] = -1; 
	
//	uid0011k100.txt を作成する処理
	fp1 = fopen(fn1, "w");
	for(i = k = 0; k < nnskK; k++)
	{
		h = nnskVecH[k]; // 最もコサイン類似度が高い文書ID
		j = nnskVecS[k]; // 最も類似度貢献度が高い単語ID
		
		if(nnskVecT[j] == -1)
		{
			nnskVecT[j] = i;
			
			if(i < 24)
				i++; 
		}
		
		fprintf(fp1, "%d %s %d %s %s\n", nnskVecT[j]+1, nnskMatM[j], h+1, nnskMatN[h], nnskMatL[h]); 
	}
	fclose(fp1);
	
//	lbl0011k100.txt を作成する処理
	fp1 = fopen(fn2, "w"); 
	fprintf(fp1, "%d %d %d\n", nnskK, nnskV, i); 
	
	for(k = 0; k < nnskK; k++)
	{
		h = nnskVecH[k]; // 文書ID
		fprintf(fp1, "%d ", nnskVecA[h]); // 文書hに出現する単語の種類数
		
//		類似度貢献度の値算出
		for(j = 0; j < nnskVecA[h]; j++) 
			fprintf(fp1, "%d:%e ", nnskMatA[h][j]+1, nnskVecW[nnskMatA[h][j]]*nnskMatX[h][j]); // 単語ID, 正規化したTF-IDF
		fprintf(fp1, "\n");
	}
	
	fclose(fp1); 
}


int nnsk5(const char **argv)
{
	int		i; 
	nnskReadValue(argv[0]);	//引数はlbl.txt
	nnskReadName(argv[1]); 	//引数はuid.txt
	nnskReadWord(argv[2]); 	//引数はwid.txt
	nnskReadDoc(argv[3]); 	//引数はdoc.txt
	nnskInitData();
	nnskCalTFIDF();
	nnskCalInverted();
	i = atoi(argv[4]);
	nnskK = atoi(argv[5]);
	fprintf(stderr, "num = %d\n", nnskK);
	nnskCalValue(i-1); 
	nnskPrintValue(argv[6], argv[7]);  // uid0011k100.txt lbl0011k100.txt
	return 0;
}
