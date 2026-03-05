#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cJSON.h"
#include "copyFile.h"
#include "generateGraph.h"
#include "generateNetwork.h"
#include "preprocess.h"
#define Dn "0001"
#define MAX_KEYWORD_LEN 512
#define MAX_FILENAME_LEN 4096
#define MAX_CONTEXT_LEN 262144
#define MAX_COLOR_LEN 32

// ネットワーク生成時のオプション
typedef struct SelectedOption {
  char *network;    // 可視化方法
  char *graph;      // グラフ構築方法
  char *searchNum;  // 検索文書数
} selectedOption;

selectedOption init(char *network, char *graph, char *searchNum) {
  selectedOption co;
  co.network = network;
  co.graph = graph;
  co.searchNum = searchNum;
  return co;
}

selectedOption option;

// *json は {name: string, normalText: string, wakachiText: string} を保持する
// Json データ
void getRequestJson(cJSON *json, const char *fn1, const char *fn2,
                    const char *fn3) {
  // JSONオブジェクトから各項目を取得する
  const char *name = cJSON_GetObjectItem(json, "name")->valuestring;
  const char *normalText = cJSON_GetObjectItem(json, "normalText")->valuestring;
  const char *wakachiText =
      cJSON_GetObjectItem(json, "wakachiText")->valuestring;
  option = init(cJSON_GetObjectItem(json, "networkType")->valuestring,
                cJSON_GetObjectItem(json, "graphType")->valuestring,
                cJSON_GetObjectItem(json, "searchNum")->valuestring);

  // クエリ文書を追加した文書データセットを作成
  copyFile(fn1, "./data/copy_uid.txt", name);
  copyFile(fn2, "./data/copy_doc.txt", normalText);
  copyFile(fn3, "./data/copy_wakachi.txt", wakachiText);
}

void loadNetworkData(const char *fn1, const char *fn2, const char *fn3,
                     const char *searchNum) {
  cJSON *root = cJSON_CreateObject();
  cJSON *edges = cJSON_AddArrayToObject(root, "edges");
  cJSON *nodes = cJSON_AddArrayToObject(root, "nodes");

  int category, id, total, r;
  double x1, y1, x2, y2, val1, val2;
  char keyword[MAX_KEYWORD_LEN], fileName[MAX_FILENAME_LEN],
      context[MAX_CONTEXT_LEN], color[MAX_COLOR_LEN];

  // エッジの座標をロード
  FILE *fp = fopen(fn1, "r");
  if (!fp) {
    fprintf(stderr, "Unknown file = %s\n", fn1);
    return;
  }
  while (fscanf(fp, "%lf %lf %lf %lf", &x1, &y1, &x2, &y2) == 4) {
    cJSON *edge = cJSON_CreateObject();
    cJSON_AddNumberToObject(edge, "x1", x1);
    cJSON_AddNumberToObject(edge, "y1", y1);
    cJSON_AddNumberToObject(edge, "x2", x2);
    cJSON_AddNumberToObject(edge, "y2", y2);
    cJSON_AddItemToArray(edges, edge);
  }
  fclose(fp);

  // ノードの座標をロード
  fp = fopen(fn2, "r");
  if (!fp) {
    fprintf(stderr, "Unknown file = %s\n", fn2);
    return;
  }
  for (int i = 0; i < atoi(searchNum); i++) {
    fscanf(fp, "%d %511s %d %4095s %262143s %lf %lf %d %31s", &category,
           keyword, &id,
           fileName, context, &val1, &val2, &r, color);
    cJSON *node = cJSON_CreateObject();
    cJSON_AddNumberToObject(node, "category", category);
    cJSON_AddStringToObject(node, "keyword", keyword);
    cJSON_AddNumberToObject(node, "id", id);
    cJSON_AddStringToObject(node, "fileName", fileName);
    cJSON_AddStringToObject(node, "title", context);
    cJSON_AddNumberToObject(node, "cx", val1);
    cJSON_AddNumberToObject(node, "cy", val2);
    cJSON_AddNumberToObject(node, "r", r);
    cJSON_AddStringToObject(node, "color", color);
    cJSON_AddItemToArray(nodes, node);
  }
  fclose(fp);

  char *jsonString = cJSON_Print(root);
  if (jsonString == NULL) {
    fprintf(stderr, "Failed to print JSON.\n");
    return;
  }
  printf("%s", jsonString);
  cJSON_Delete(root);
  free(jsonString);
}

char *toLowerCase(char *str) {
  for (int i = 0; str[i]; i++) str[i] = tolower((unsigned char)str[i]);
  return str;
}

void handlePreflight() {
  printf("Access-Control-Allow-Origin: *\r\n");
  printf("Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n");
  printf("Access-Control-Allow-Headers: Content-Type\r\n");
  printf("Access-Control-Max-Age: 86400\r\n");
  printf("Content-Type: text/plain\r\n\r\n");
}

void handleRequest() {
  printf("Access-Control-Allow-Origin: *\r\n");
  printf("Content-Type: application/json\r\n\r\n");

  char *input, *clen;
  long length;
  clen = (char *)getenv("CONTENT_LENGTH");
  if (!clen) {
    fprintf(stderr, "Missing CONTENT_LENGTH.\n");
    return;
  }
  length = atol(clen);
  input = (char *)malloc(length + 1);
  if (!input) {
    fprintf(stderr, "Memory allocation failed.\n");
    return;
  }
  if (fgets(input, length + 1, stdin) == NULL) {
    fprintf(stderr, "Failed to read input.\n");
    free(input);
    return;
  }
  cJSON *json = cJSON_Parse(input);
  if (json == NULL) {
    fprintf(stderr, "Invalid JSON: %s", input);
    free(input);
    return;
  }
  getRequestJson(json, "./data/uid.txt", "./data/doc.txt",
                 "./data/wakachi.txt");

  // 検索文書数
  char *searchNum = option.searchNum;

  // グラフ構築方法
  char upperGraphName[100], lowerGraphName[100];
  snprintf(upperGraphName, sizeof(upperGraphName), "%s", option.graph);
  snprintf(lowerGraphName, sizeof(lowerGraphName), "%s",
           toLowerCase(option.graph));

  // モジュールの引数
  char args1[512];  // ファイル名なので多めに
  snprintf(args1, sizeof(args1), "./result/%s.txt",
           lowerGraphName);  // 選択されたファイルに応じてロードファイルを変更.
  const char *mkwidArgs[] = {"./data/copy_wakachi.txt", "./data/wid.txt"};
  const char *mklblArgs[] = {"./data/wid.txt", "./data/copy_wakachi.txt",
                             "./data/lbl.txt"};
  const char *nnsk5Args[] = {"./data/lbl.txt",
                             "./data/copy_uid.txt",
                             "./data/wid.txt",
                             "./data/copy_doc.txt",
                             Dn,
                             searchNum,
                             "./result/uidk.txt",
                             "./result/lblk.txt"};
  const char *mstArgs[] = {"./result/lblk.txt", "./result/mst.txt"};
  const char *hmlArgs[] = {"./result/lblk.txt", "./result/mst.txt", "1",
                           "./result/hml.txt"};
  const char *knnArgs[] = {"./result/lblk.txt", "./result/knn.txt"};
  const char *netArgs[] = {args1, "./result/uidk.txt", "./result/edge.txt",
                           "./result/node.txt", "./result/category.txt"};

  // モジュールを実行
  preprocess(mkwidArgs, mklblArgs, nnsk5Args);
  generateGraph(mstArgs, knnArgs, hmlArgs, upperGraphName);
  generateNetwork(netArgs, option.network);
  loadNetworkData("./result/edge.txt", "./result/node.txt",
                  "./result/category.txt", searchNum);
  cJSON_Delete(json);
  free(input);
}

int main() {
  const char *requestMethod = getenv("REQUEST_METHOD");
  if (requestMethod && strcmp(requestMethod, "OPTIONS") == 0) {
    // プリフライトリクエストの処理
    handlePreflight();
  } else {
    // 通常のリクエストの処理
    handleRequest();
  }
  return 0;
}
