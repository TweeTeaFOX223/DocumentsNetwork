#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<math.h>
#include	<time.h>
#define MAX_LINE_LENGTH 1024
#define MAX_TOKENS 100

char	*kkVecN, **kkMatF, **kkMatC; 
int     kkDm, kkDn, kkDc, kkCntV[4], *kkNumV, **kkAdjM, *kkNumC, *kkVecC, *kkVecA, *kkVecT1, *kkVecT2, *kkVecS;
double	**kkIn, *kkDecV, **kkMatW, **kkMatG, kkValE[2], **kkMatA, *kkVecB, *kkVecT;
int kkVecNCap;

/* knn3000k100.txt をロード */
void    kkReadValue(const char *fn1)
{
	/*
		kkNumV[i]    : ノード i に接続しているエッジ数
		kkAdjM[i][j] : ノード i に接続しているノード番号  (j < kkNumV[i])
		kkIn[i][j]   : ノード i に接続しているノードの距離 (j < kkNumV[i])
		kkNumC       : 1
	*/
	
	FILE   *fp;
	int		i, j, k;
	double	v;
	
	if((fp = fopen(fn1, "r")) == NULL)
		fprintf(stderr, "Unknown File = %s\n", fn1);
	
	fscanf(fp, "%d %d %d", &kkDm, &k, &kkDc);
	kkNumV = (int *)     malloc(sizeof(int)*kkDm);
	kkAdjM = (int **)    malloc(sizeof(int *)*kkDm);
	kkNumC = (int *)     malloc(sizeof(int)*kkDm);
	kkIn   = (double **) malloc(sizeof(double *)*kkDm);
	
	for(i = 0; i < kkDm; i++)
	{
		fscanf(fp, "%d", &kkNumV[i]);
		kkAdjM[i] = (int *)    malloc(sizeof(int)*kkNumV[i]);
		kkIn[i]   = (double *) malloc(sizeof(double)*kkNumV[i]);
		
		for(j = 0; j < kkNumV[i]; j++)
		{ 
			fscanf(fp, "%d:%lf", &k, &v);
			kkAdjM[i][j] = k-1;
			kkIn[i][j]   = v;
		}
		
		fscanf(fp, "%d", &kkNumC[i]);
		for(j = 0; j < kkNumC[i]; j++) 
			fscanf(fp, "%lf", &v);
	}
	
	fclose(fp);
}


void kkReadUid(const char *fn1)
{
	FILE   *fp;
	char	c; 
	int		i, j, k;
	
	if((fp = fopen(fn1, "r")) == NULL)
	{
		fprintf(stderr, "Unknown File = %s\n", fn1); 
		exit(1);
	}
	
	kkVecNCap = 1000000;
	kkVecN = (char *)  malloc(sizeof(char)*kkVecNCap);
	kkVecC = (int *)   malloc(sizeof(int)*kkDm);
	kkMatF = (char **) malloc(sizeof(char *)*kkDm);
	kkMatC = (char **) malloc(sizeof(char *)*kkDm);
	
	for(i = j = 0; i < kkDm; i++)
	{
		fscanf(fp, "%d ", &kkVecC[i]);
		
		while((c = getc(fp)) != '\n'){
			char *newBuf;
			if(j + 1 >= kkVecNCap){
				kkVecNCap *= 2;
				newBuf = (char *)realloc(kkVecN, sizeof(char)*kkVecNCap);
				if(newBuf == NULL){
					fprintf(stderr, "Memory allocation failed\n");
					exit(1);
				}
				kkVecN = newBuf;
			}
			kkVecN[j++] = c;
		}
		
		kkMatF[i] = (char *) malloc(sizeof(char)*(j+1));
		for(k = 0; k < j; k++) 
			kkMatF[i][k] = kkVecN[k]; 
		kkMatF[i][j] = '\0'; j = 0; 
	}
	
	fclose(fp);
}


/* メモリの割り当て */
void kkAllocVector(double **ptr, int dim)
{
	*ptr = (double *) malloc(sizeof(double)*dim);
}	


/* 行列の初期化 */
void kkAllocMatrix(double ***ptr, int dc, int dr)
{
	int		i;
	*ptr = (double **) malloc(sizeof(double *)*dc);
	for(i = 0; i < dc; i++) 
		(*ptr)[i] = (double *) malloc(sizeof(double)*dr);
}


/* ベクトルのノルムを計算 */
double kkCalNorm(double *vp, int dim)
{
	int		i;
	double	v;
	for(i = 0, v = 0.0; i < dim; i++, vp++) 
		v += (*vp)*(*vp); 
	return(v);
}


/* ベクトルの内積を計算 */
double kkCalProd(double *vp, double *wp, int dim)
{
	int		i;
	double	v;
	for(i = 0, v = 0.0; i < dim; i++, vp++, wp++) 
		v += (*vp)*(*wp); 
	return(v);
}


/* ベクトル間の距離を計算 */
double kkCalDist(double *vp, double *wp, int dim)
{
	int		i;
	double	v;
	for(i = 0, v = 0.0; i < dim; i++, vp++, wp++) 
		v += ((*vp)-(*wp))*((*vp)-(*wp)); 
	return(v);
}


/* 正定値対称行列を LL 分解する Cholesky 分解関数 */
int	kkLLdecompose(double **LL, int dim)
{
	/*
		LL分解は、与えられた対称行列を下三角行列(LL)とその転置の積に分解する。
	
		LL      : 対称行列
		i, j, k : ループ変数
		t       : 一時的な値を保持するために使用。
		Eps     : 許容誤差。この値以下の値を持つ対角要素が現れた場合、計算を終了。
		
		参考文献
		http://nalab.mind.meiji.ac.jp/~mk/labo/text/cholesky.pdf
	*/
	
	int		i, j, k;
	double	t;
	static double Eps = 1.0e-4;
	
	for(k = 0; k != dim; k++)
	{
//		L[k][i] (i < k) の計算。 k 行 k 列の値 LL[k][k] と、 k 行 k 列目より上の要素 LL[k][i] （i < k） に基づいて、 k 行 i 列目の値を決定
		for(i = 0; i != k; i++)
		{
//			繰り返し(j < i−1) t に LL[k][i] 成分を代入し、 LL[i][j]成分から LL[k][j]成分を減算する
			for(j = 0, t = LL[k][i]; j != i; j++) 
				t -= LL[i][j]*LL[k][j];
			
//			LL[k][i] を対角成分 LL[i][i] で除算して正規化
			LL[k][i] = t/LL[i][i];
		}
		
//		L[k][k] の計算。 対称行列 LL[k][k] を k 行 k 列よりも前の要素 LL[k][i] の二乗和で減算
		for(i = 0, t = LL[k][k]; i < k; i++)
			t -= LL[k][i]*LL[k][i];
		
//		対称行列が許容誤差より小さい場合は正定値性が失われているため 0 を返却
		if(t < Eps)
			return(0);
		
//		t は 三角成分の2乗で求めているため、三角成分を導出する場合は t の平方根を計算する
		LL[k][k] = sqrt(t);
	}
	
//	LL分解が成功した場合は 1 を返却
	return(1);
}


/* 分解した LL に対して (LL^T)X = b を計算する関数 */
void kkLLsolve(double **LL, double *B, double *X, int dim)//kkMatA, kkMatG[i], kkVecB, kkDn
{
/*
	dim : 単語種類数(89993)
	
	参考文献
	https://manabitimes.jp/math/1170
*/
	
	int		i, j;
	double	t;
	
//	前進代入
	for(i = 0; i != dim; i++)
	{
		for(j = 0, t = B[i]; j != i; j++) 
			t -= LL[i][j]*X[j];
		X[i] = t/LL[i][i];
	}
	
//	後退代入
	for(i = dim-1; i != -1; i--)
	{
		for(j = i+1, t = X[i]; j != dim; j++) 
			t -= LL[j][i]*X[j];
		X[i] = t/LL[i][i];
	}
}


void kkPrintValue(const char *fn1, const char *fn2, const char **nodeColor)
{
	FILE	*fp;
	int		i, j, k, x = 1600, y = 700;
	double	v, w;
	
//	ノードの座標の最大値と最小値(横軸)を求める
	for(i = 0; i < kkDm; i++)
	{
		if(i == 0) 
			v = w = kkMatW[i][0]; 
		
		else if(v > kkMatW[i][0]) 
			v = kkMatW[i][0]; 
		
		else if(w < kkMatW[i][0]) 
			w = kkMatW[i][0];
	}
	
//	x~yの範囲に収まるようにする。
	for(i = 0, w = w-v; i < kkDm; i++)
		kkMatW[i][0] = (0.98*x*((kkMatW[i][0]-v)/w))+(0.01*x);
	
//	ノードの座標の最大値と最小値(縦軸)を求める
	for(i = 0; i < kkDm; i++)
	{
		if(i == 0) 
			v = w = kkMatW[i][1]; 
		
		else if(v > kkMatW[i][1]) 
			v = kkMatW[i][1]; 
		
		else if(w < kkMatW[i][1]) 
			w = kkMatW[i][1];
	}
	
//	x~yの範囲に収まるようにする。
	for(i = 0, w = w-v; i < kkDm; i++) 
		kkMatW[i][1] = (0.98*y*((kkMatW[i][1]-v)/w))+(0.01*y);
	
//	エッジの座標を生成
	fp = fopen(fn1, "w");
	for(i = 0; i < kkDm; i++)
	{
		for(j = 0; j < kkNumV[i]; j++)
		{
			if((k = kkAdjM[i][j]) <= i) 
				continue; 
			fprintf(fp, "%f %f %f %f\n", kkMatW[i][0], kkMatW[i][1], kkMatW[k][0], kkMatW[k][1]); 
		}
	}
	fclose(fp);
	
//	ノードの座標を生成
	fp = fopen(fn2, "w");
	fprintf(fp, "%d %s %f %f 12 %s\n", kkVecC[0], kkMatF[0], kkMatW[0][0], kkMatW[0][1], nodeColor[(kkVecC[0]-1)%25]); 
	for(i = 1; i < kkDm; i++){
		fprintf(fp, "%d %s %f %f 6 %s\n", kkVecC[i], kkMatF[i], kkMatW[i][0], kkMatW[i][1], nodeColor[(kkVecC[i]-1)%25]);
	}
	fclose(fp);
}


void kkPrintWeight(char *fn1)
{
    FILE *fp;
    int   i, j, k;
	
	fp = fopen(fn1, "w"); 
	fprintf(fp, "X = [\n");
	
	for(i = 0; i < kkDm; i++)
	{
		for(k = 0; k < kkDn; k++) 
			fprintf(fp, "%f ", kkMatW[i][k]);
		fprintf(fp, "\n"); 
	}
	
	fprintf(fp, "];\n");
	fprintf(fp, "plot3(X(:,1),X(:,2),X(:,3),'r.');\n");
	fclose(fp);
}


/* 行列を初期化 */
void kkInitWeight(double v)
{
	int		h, i;
	kkVecA  = (int *) malloc(sizeof(int)*kkDm);
	kkVecT1 = (int *) malloc(sizeof(int)*kkDm);
	kkVecT2 = (int *) malloc(sizeof(int)*kkDm);
	
	kkAllocMatrix(&kkMatW, kkDm, kkDn); kkAllocMatrix(&kkMatG, kkDm, kkDn); 
	kkAllocMatrix(&kkMatA, kkDn, kkDn); kkAllocVector(&kkVecB, kkDn); 
	kkAllocVector(&kkVecT, kkDn); kkAllocVector(&kkDecV, kkDn); 
	
	for(h = 0; h < kkDn; h++) 
		kkDecV[h] = 0.0;
	
	for(i = 0; i < kkDm; i++)
	{
		for(h = 0; h < kkDn; h++)
		{
//			0 から 1 までの浮動小数点数を乱数で設定
			kkMatW[i][h]  = (rand()%10000)/10000.0;
			
//			正負を設定
			kkMatW[i][h] *= (rand()%2 == 0) ? 1.0: -1.0;
		}
	}
}


/* ある頂点から他の頂点への最短経路を計算する関数 */
void kkCalPath(int i)
{
/*
	kkVecA[i]    : ノード i までの距離を保持する配列
	kkAdjM[i][j] : ノード i の隣接ノード j
	h1         : 次の探索レベルで探索されるノード数
	h2         : kkVecT の要素数
*/
	
	int	d, j, k, h1, h2, s;
	
//	頂点への距離が未定義であることを示す -1 を設定
	for(j = 0; j < kkDm; j++) 
		kkVecA[j] = -1; 
	
	for(d = 1, kkVecA[i] = 0, kkVecT1[0] = i, h1 = 1; ; d++)
	{
		for(j = 0, h2 = 0; j < h1; j++)
		{
			for(k = 0; k < kkNumV[kkVecT1[j]]; k++)
			{
//				kkVecT[j] に隣接しており、探索対象であるノードを s に設定
				s = kkAdjM[kkVecT1[j]][k];
				
//				隣接ノード s を既に探索している場合は処理をスキップ
				if(kkVecA[s] != -1) 
					continue;
				
//				未探索隣接ノード s に kkVecT[j] との距離を設定
				kkVecA[s] = d;
				
//				kkVecT[j] に新たに見つかった隣接ノード s を設定 (h2 は kkVecT の要素数)
				kkVecT2[h2++] = s;
			}
		}
		
//		新たなノードが発見されなくなった場合は探索を終了
		if(h2 == 0) 
			break;
		
//		kkVecT2 に格納された頂点を次の探索レベルで探索するために kkVecT1 にコピーし、探索を続行
		for(j = 0; j < h2; j++) 
			kkVecT1[j] = kkVecT2[j]; 
		
//		次の探索レベルで探索される頂点の数を設定
		h1 = h2;
	}
}


double kkCalValue()
{
	/*
		kkMatW[i][h] : ノード i の座標
	*/
	
	int		h, i, j;
	double	v, w, z, x, y;
	
	for(i = 0, z = 0.0; i < kkDm; i++)
	{ 
		for(h = 0; h < kkDn; h++)
		{ 
			z         += 0.5*kkDecV[h]*kkMatW[i][h]*kkMatW[i][h];
			kkMatG[i][h] = -kkDecV[h]*kkMatW[i][h]; 
		}
	}
	
	for(i = 0, z = 0.0; i < kkDm; i++)
	{
		kkCalPath(i); 
		for(j = i+1; j < kkDm; j++)
		{
			for(h = 0, v = 0.0; h < kkDn; h++)
			{
				kkVecT[h] = kkMatW[i][h]-kkMatW[j][h]; 
				v      += kkVecT[h]*kkVecT[h]; 
			}
			
			w  = (v == 0.0) ? 1.0: 1.0 - sqrt(v)/kkVecA[j]; 
			z += w*w; 
			w  = (v == 0.0) ? 1.0/kkVecA[j]: w/(kkVecA[j]*sqrt(v)); 
			
			for(h = 0; h < kkDn; h++)
			{
				kkMatG[i][h] += w*kkVecT[h];
				kkMatG[j][h] += -w*kkVecT[h];
			}
		}
	}
	
	return(z);
}


double kkCalBenefit(int i)
{
	int		h, j, k;
	double	v, w, z, x, y;
	
	for(h = 0, z = 0.0; h < kkDn; h++)
		z += -kkDecV[h]*kkVecB[h]*(kkMatW[i][h]+0.5*kkVecB[h]);
	
	for(j = 0; j < kkDm; j++)
	{
		if(i == j) 
			continue;
		
		for(h = 0, v = 0.0; h < kkDn; h++)
		{
			kkVecT[h] = kkMatW[i][h]-kkMatW[j][h]; 
			v      += kkVecT[h]*kkVecT[h]; 
		}
		
		w  = (v == 0.0) ? 1.0: 1.0 - sqrt(v)/kkVecA[j]; 
		z += w*w; 
		
		for(h = 0, v = 0.0; h < kkDn; h++)
		{
			kkVecT[h] += kkVecB[h]; 
			v       += kkVecT[h]*kkVecT[h]; 
		}
		
		w  = (v == 0.0) ? 1.0: 1.0 - sqrt(v)/kkVecA[j]; 
		z -= w*w; 
	}
	
	return(z);
}


void kkUpdateInfo(int i)
{
	int		h, j, k;
	double	v, w, x, y;
	
	for(h = 0; h < kkDn; h++) 
		kkMatG[i][h] = -kkDecV[h]*(kkMatW[i][h]+kkVecB[h]);
	
	for(j = 0; j < kkDm; j++)
	{
		if(i == j) 
			continue;
		
		for(h = 0, v = 0.0; h < kkDn; h++)
		{
			kkVecT[h] = kkMatW[i][h]-kkMatW[j][h]; 
			v      += kkVecT[h]*kkVecT[h]; 
		}
		
		w = (v == 0.0) ? 1.0: 1.0 - sqrt(v)/kkVecA[j]; 
		w = (v == 0.0) ? 1.0/kkVecA[j]: w/(kkVecA[j]*sqrt(v)); 
		
		for(h = 0; h < kkDn; h++) 
			kkMatG[j][h] -= -w*kkVecT[h]; 
		
		for(h = 0, v = 0.0; h < kkDn; h++)
		{
			kkVecT[h] += kkVecB[h]; 
			v       += kkVecT[h]*kkVecT[h]; 
		}
		
		w = 1.0 - sqrt(v)/kkVecA[j]; 
		w = w/(kkVecA[j]*sqrt(v)); 
		
		for(h = 0; h < kkDn; h++)
		{
			v           = w*kkVecT[h]; 
			kkMatG[i][h] += v; kkMatG[j][h] += -v; 
		}
	}
	
	for(h = 0; h < kkDn; h++) 
		kkMatW[i][h] += kkVecB[h]; 
}


int	kkUpdateWeight(double *max)
{
/*
	kkVecB : 連立方程式 x = ay + b の x に相当
*/
	int		h, i, j, k;
	double	u, v, w, z, x, y;
	
	for(i = 0, *max = kkCalNorm(kkMatG[0], kkDn), j = 1; j < kkDm; j++)
	{
//		最大ノルムの計算
		if((v = kkCalNorm(kkMatG[j], kkDn)) > *max)
		{
//			ノード i が更新の起点となる
			i    = j;
			*max = v; 
		}
	}
		
//	最大のノルムが十分小さい場合（閾値以下の場合）
	if(*max < 1.0e-4) 
		return(-1);
	
//	対称行列 kkMatA の初期化
	for(h = 0; h < kkDn; h++)
	{
//		対角成分の値を kkDecV[h], それ以外の値を 0 に初期化
		kkMatA[h][h] = kkDecV[h]; 
		for(k = 0; k < h; k++) 
			kkMatA[h][k] = 0.0; 
	}
	
//	最短経路の計算(ノード i に接続する全頂点への最短経路長 kkVecA を計算)
	kkCalPath(i); 
	
//	kkMatA の対角成分と非対角成分を更新
	for(j = 0, u = 0.0; j < kkDm; j++)
	{
		if(i == j) 
			continue;
		
//		(kkMatW[i][h],kkMatW[j][h])間の距離行列の2乗和を計算
		for(h = 0, v = 0.0; h < kkDn; h++)
		{
			kkVecT[h] = kkMatW[i][h]-kkMatW[j][h]; 
			v      += kkVecT[h]*kkVecT[h]; 
		}
		
//		重み付け係数 w, x の生成
		w = 1.0 - sqrt(v)/kkVecA[j]; // (kkMatW[i][h],kkMatW[j][h])間の距離が大きいほど w の値は小さくなる
		x = w/(kkVecA[j]*sqrt(v));   // (kkMatW[i][h],kkMatW[j][h])間の距離が大きいほど x の値は大きくなる
		
//		対称行列 kkMatA の対角成分を更新 (更新すると対角成分の値が小さくなる)
		for(h = 0; h < kkDn; h++) 
			kkMatA[h][h] += -x; 
		
//		対称行列 kkMatA の非対角成分を更新
		w = ((1.0/(kkVecA[j]*kkVecA[j])) + x)/v; 
		for(h = 0; h < kkDn; h++)
			for(k = 0; k <= h; k++) 
				kkMatA[h][k] += w*kkVecT[h]*kkVecT[k];
	}

//	対称行列 kkMatA と重みベクトル kkMatG の内積を計算
	for(h = 0, w = 0.0; h < kkDn; h++)
	{
		for(k = 0, v = 0.0; k < kkDn; k++) 
			v += kkMatA[h][k]*kkMatG[i][k]; 
		w += v*kkMatG[i][h];
	}
	
//	LL 分解に成功した場合
	if(kkLLdecompose(kkMatA, kkDn) == 1)
	{
//		LL 分解で得られた連立方程式を計算
		kkLLsolve(kkMatA, kkMatG[i], kkVecB, kkDn); 
		v = kkCalBenefit(i); 
		
//		解が望ましい条件を満たすか検査
		if(v > 0.0 && fabs(v) < kkValE[0])
		{
			kkCntV[0] += 1; 
			return(i); 
		}
		
		kkCntV[1] += 1; 
	}
	
	if(w > 1.0e-4)
	{
//		ノード i の重みベクトル kkMatG[i] のノルム計算
		for(k = 0, v = 0.0; k < kkDn; k++) 
			v += kkMatG[i][k]*kkMatG[i][k]; 
		
//		重みベクトル kkVecB を再設定 (重みベクトル kkMatG[i] を w で除算して正規化した値を代入)
		for(k = 0, w = v/w; k < kkDn; k++) 
			kkVecB[k] = w*kkMatG[i][k]; 
		
//		再設定された重みベクトル kkVecB のノルムを再計算
		for(k = 0, w = 0.0; k < kkDn; k++) 
			w += kkVecB[k]*kkVecB[k]; 
		
//		再設定された重みに基づいて利益を計算
		v = kkCalBenefit(i); 
		
//		利益が一定の条件を満たすか検査
		if(w > 1.0e-4 && v > 0.0 && fabs(v) < kkValE[0])
		{
			kkCntV[3] += 1; 
			return(i); 
		}
	}
	
//	更新が失敗したことを示すために、kkCntV 配列の適切な要素に1を追加 (この配列は更新が成功したのか示す統計情報を保持している)
	kkCntV[2] += 1;
	
//	更新が失敗した場合はランダムな値 v を生成し、新たな重み値とする
	v = ((rand()%100)+1)/1000.0;
	
//	kkVecB を再設定
	for(h = 0; h < kkDn; h++) 
		kkVecB[h] = v*kkMatG[i][h];
	
	return(i);
}


int kk(const char **argv, const char** commonNodeColor)
{
	char   name[256];
    int    i, j, k, m;
    double v, err, max;
	
	srand(time(0)); 
	kkDn = 2;
	kkReadValue(argv[0]); // ネットワークを読み込む
	kkReadUid(argv[1]);   // カーソルを当てると表示される文章を読み込む
	kkInitWeight(0.0);    // 初期値の設定
	
	kkValE[1] = -1.0;
	kkValE[0] = kkCalValue();
	kkCntV[0] = kkCntV[1] = kkCntV[2] = kkCntV[3] = 0;
	
//	反復学習
	for(i = 0; i < 10000000; i++)
	{
//		ノードの座標を更新する
		if((k = kkUpdateWeight(&max)) == -1)
			break;
		
//		ベクトルを更新するのに必要な情報を更新
		kkUpdateInfo(k);
		
//		kkValE[0] = kkCalValue();
		
		if((i%10000) == 0)
		{
			sprintf(name, "%s.%d", argv[2], i/10000);
			kkPrintWeight(name);
		}
	}
	
	kkValE[0] = kkCalValue();
	kkPrintValue(argv[2], argv[3], commonNodeColor);
	
	return 0;
}
