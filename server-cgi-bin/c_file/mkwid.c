#include	<stdio.h>
#include	<stdlib.h>
#include	<math.h>
#include	<string.h>
#include	<unistd.h>

char	*mkwidVecN, **mkwidMatN; 
int		mkwidDm, mkwidDn, *mkwidVecI, *mkwidVecJ; 
int   mkwidVecNCap;


/* doc.txtから単語を記録 */
void    mkwidReadValue(char *fn1)
{
	FILE		*fp;
	int		i, j, k, c;
	if((fp = fopen(fn1, "r")) == NULL) { fprintf(stdout, "Unknown File = %s\n", fn1); exit(1);}
	mkwidDm = mkwidDn = 0;
	while((c = getc(fp)) != EOF){ if(c == ' ') mkwidDn +=1; if(c == '\n') mkwidDm +=1; }
	fclose(fp); 
	mkwidDn += mkwidDm;
	fp = fopen(fn1, "r"); 
	mkwidMatN = (char **) malloc(sizeof(char *)*mkwidDn);
	mkwidVecNCap = 4096;
	mkwidVecN = (char *) malloc(sizeof(char)*mkwidVecNCap);
	i = j = 0; 
	while((c = getc(fp)) != EOF){
		if(c == ' ' || c == '\n'){ 
			if(j == 0) continue; 
			mkwidMatN[i] = (char *) malloc(sizeof(char)*(j+1));
			for(k = 0; k < j; k++) mkwidMatN[i][k] = mkwidVecN[k]; 
			mkwidMatN[i++][j] = '\0'; j = 0; 
		}
		else {
			if(j + 1 >= mkwidVecNCap){
				char *newBuf;
				mkwidVecNCap *= 2;
				newBuf = (char *)realloc(mkwidVecN, sizeof(char)*mkwidVecNCap);
				if(newBuf == NULL){
					fprintf(stderr, "Memory allocation failed\n");
					exit(1);
				}
				mkwidVecN = newBuf;
			}
			mkwidVecN[j++] = c;
		}
	}
	fclose(fp);
	mkwidDn = i;
}
void	mkwidSort(int *ip, int n)
{
	int		i, j, k, h;
	if((h = n/2) == 0) return;
	mkwidSort(&ip[0], h);
	mkwidSort(&ip[h], n-h); 
	for(i = j = 0, k = h; i < n; i++){
		if(j < h && (k == n || strcmp(mkwidMatN[ip[j]], mkwidMatN[ip[k]]) <= 0)) mkwidVecJ[i] = ip[j++]; 
		else mkwidVecJ[i] = ip[k++]; 
	}
	for(i = 0; i < n; i++) ip[i] = mkwidVecJ[i]; 
}

/* wid.txtに単語辞書を登録 */
void	mkwidPrintValue(char *fn1)
{
	FILE	*fp;
	int		h, i, j, k, s;
	mkwidVecI = (int *) malloc(sizeof(int)*mkwidDn); 
	mkwidVecJ = (int *) malloc(sizeof(int)*mkwidDn); 
	for(i = 0; i < mkwidDn; i++) mkwidVecI[i] = i; 
	mkwidSort(&mkwidVecI[0], mkwidDn); 
	fp = fopen(fn1, "w"); 
	for(h = i = 0; i < mkwidDn; ){ 
		s = mkwidVecI[i]; 
		for(j = i+1, k = 1; j < mkwidDn; j++) if(strcmp(mkwidMatN[s], mkwidMatN[mkwidVecI[j]]) == 0) k++; else break; 
		fprintf(fp, "%d %s %d\n", h+1, mkwidMatN[s], k); 
		i = j; h++; 
	}
	fclose(fp);
}
int		mkwid(char **argv)
{
	mkwidReadValue(argv[0]);  //doc.txt
	mkwidPrintValue(argv[1]); //wid.txt
	return 0; 
}

/*

doc.txt(分かち書きされた7367件の文章)

友人 代表 の スピーチ 、 独 女 は どう こなし て いる ？ もうすぐ ジューン ・ ブライド と 呼ば れる ６月 。 独 女
願い さ れる スピーチ なら 準備 が できる し まだ いい です よ 。 一番 嫌 な の は 何 と いっ て も サプライズス
ネット で 断ち切れ ない 元 カレ と の 縁 携帯 電話 が 普及 する 以前 、 恋人 へ の 連絡 ツール は 一般 電話 が 普通 だっ た 。 
・
・
・


wid.txt(単語リスト)
・
・
・
36997 プレーンタイプ 2
36998 プレーンチョコレート 1
36999 プレーンノット 1
37000 プレーンヨーグルト 4
37001 プレーン・ 4
37002 プロ 828
37003 プロアスリート 2
37004 プロキオン 1
37005 プロキャディ 2
37006 プロクオリティー 1
37007 プログラマー 7
37008 プログラミング 19
37009 プログラミングキャンプ 1
37010 プログラム 187
37011 プログラムミング 1
37012 プログレス 1
37013 プログレッシブ 4
37014 プログレッソ 3
37015 プロゲステロン 4
37016 プロサッカーリーグ 1
37017 プロサーファー 4
37018 プロシューマー 8
37019 プロシューマークラス 1
37020 プロショップ 2
37021 プロジェクション 3
37022 プロジェクタ 3
37023 プロジェクター 131
37024 プロジェクト 341
・
・
・


*/
