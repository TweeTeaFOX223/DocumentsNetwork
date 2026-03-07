#include <ctype.h>
#include <math.h>
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
#define MAX_GRAPH_LINE_LEN 65536

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

typedef struct SparseDocument {
  int termCount;
  int *termIds;
  double *weights;
} sparseDocument;

typedef struct SimilarityContext {
  int docCount;
  sparseDocument *docs;
} similarityContext;

typedef struct NodeMeta {
  int category;
  int id;
  char keyword[MAX_KEYWORD_LEN];
  char fileName[MAX_FILENAME_LEN];
  char context[MAX_CONTEXT_LEN];
  double cx;
  double cy;
  int r;
  char color[MAX_COLOR_LEN];
} nodeMeta;

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
                     const char *fnGraph, const char *fnLblk,
                     const char *searchNum) {
  cJSON *root = cJSON_CreateObject();
  cJSON *edges = cJSON_AddArrayToObject(root, "edges");
  cJSON *nodes = cJSON_AddArrayToObject(root, "nodes");

  int category, id, total, r;
  double x1, y1, x2, y2, val1, val2;
  char keyword[MAX_KEYWORD_LEN], fileName[MAX_FILENAME_LEN],
      context[MAX_CONTEXT_LEN], color[MAX_COLOR_LEN];

  similarityContext simCtx = {0};

  int graphDocCount = 0;
  int **graphAdj = NULL;
  int *graphAdjCount = NULL;
  double *edgeSimilarities = NULL;
  int edgeSimilarityCount = 0;
  nodeMeta *nodeMetas = NULL;

  char line[MAX_GRAPH_LINE_LEN];

  FILE *graphFp = fopen(fnGraph, "r");
  if (!graphFp) {
    fprintf(stderr, "Unknown file = %s\n", fnGraph);
    return;
  }
  if (fgets(line, sizeof(line), graphFp) == NULL ||
      sscanf(line, "%d", &graphDocCount) != 1 || graphDocCount <= 0) {
    fclose(graphFp);
    fprintf(stderr, "Invalid graph file header = %s\n", fnGraph);
    return;
  }
  graphAdj = (int **)malloc(sizeof(int *) * graphDocCount);
  graphAdjCount = (int *)malloc(sizeof(int) * graphDocCount);
  if (!graphAdj || !graphAdjCount) {
    fclose(graphFp);
    fprintf(stderr, "Memory allocation failed.\n");
    free(graphAdj);
    free(graphAdjCount);
    return;
  }
  for (int i = 0; i < graphDocCount; i++) {
    graphAdj[i] = NULL;
    graphAdjCount[i] = 0;
    if (fgets(line, sizeof(line), graphFp) == NULL) {
      fclose(graphFp);
      fprintf(stderr, "Invalid graph file body = %s\n", fnGraph);
      goto cleanup;
    }
    char *token = strtok(line, " \t\r\n");
    if (!token) {
      fclose(graphFp);
      fprintf(stderr, "Invalid graph row = %s\n", fnGraph);
      goto cleanup;
    }
    int degree = atoi(token);
    graphAdjCount[i] = degree;
    graphAdj[i] = (int *)malloc(sizeof(int) * degree);
    if (!graphAdj[i] && degree > 0) {
      fclose(graphFp);
      fprintf(stderr, "Memory allocation failed.\n");
      goto cleanup;
    }
    for (int j = 0; j < degree; j++) {
      token = strtok(NULL, " \t\r\n");
      if (!token) {
        fclose(graphFp);
        fprintf(stderr, "Invalid graph edge row = %s\n", fnGraph);
        goto cleanup;
      }
      int neighbor = 0;
      double weight = 0.0;
      if (sscanf(token, "%d:%lf", &neighbor, &weight) != 2) {
        fclose(graphFp);
        fprintf(stderr, "Invalid graph edge token = %s\n", token);
        goto cleanup;
      }
      graphAdj[i][j] = neighbor - 1;
    }
  }
  fclose(graphFp);
  graphFp = NULL;

  FILE *lblkFp = fopen(fnLblk, "r");
  if (!lblkFp) {
    fprintf(stderr, "Unknown file = %s\n", fnLblk);
    goto cleanup;
  }
  int lblkDocCount = 0, lblkTermCount = 0, lblkCategoryCount = 0;
  if (fscanf(lblkFp, "%d %d %d", &lblkDocCount, &lblkTermCount,
             &lblkCategoryCount) != 3 ||
      lblkDocCount != graphDocCount) {
    fclose(lblkFp);
    fprintf(stderr, "Invalid lblk file header = %s\n", fnLblk);
    goto cleanup;
  }
  simCtx.docCount = lblkDocCount;
  simCtx.docs = (sparseDocument *)calloc((size_t)lblkDocCount, sizeof(sparseDocument));
  if (!simCtx.docs) {
    fclose(lblkFp);
    fprintf(stderr, "Memory allocation failed.\n");
    goto cleanup;
  }

  for (int i = 0; i < lblkDocCount; i++) {
    int termCount = 0;
    if (fscanf(lblkFp, "%d", &termCount) != 1 || termCount < 0) {
      fclose(lblkFp);
      fprintf(stderr, "Invalid lblk row = %s\n", fnLblk);
      goto cleanup;
    }
    simCtx.docs[i].termCount = termCount;
    simCtx.docs[i].termIds = (int *)malloc(sizeof(int) * termCount);
    simCtx.docs[i].weights = (double *)malloc(sizeof(double) * termCount);
    if ((termCount > 0) &&
        (!simCtx.docs[i].termIds || !simCtx.docs[i].weights)) {
      fclose(lblkFp);
      fprintf(stderr, "Memory allocation failed.\n");
      goto cleanup;
    }
    double norm = 0.0;
    for (int j = 0; j < termCount; j++) {
      int termId = 0;
      double value = 0.0;
      if (fscanf(lblkFp, "%d:%lf", &termId, &value) != 2) {
        fclose(lblkFp);
        fprintf(stderr, "Invalid lblk token = %s\n", fnLblk);
        goto cleanup;
      }
      simCtx.docs[i].termIds[j] = termId - 1;
      simCtx.docs[i].weights[j] = value;
      norm += value * value;
    }
    if (norm > 0.0) {
      double invNorm = 1.0 / sqrt(norm);
      for (int j = 0; j < termCount; j++) {
        simCtx.docs[i].weights[j] *= invNorm;
      }
    }
  }
  fclose(lblkFp);

  // ノードの座標をロード
  FILE *fp = fopen(fn2, "r");
  if (!fp) {
    fprintf(stderr, "Unknown file = %s\n", fn2);
    return;
  }
  nodeMetas = (nodeMeta *)calloc((size_t)atoi(searchNum), sizeof(nodeMeta));
  if (!nodeMetas) {
    fclose(fp);
    fprintf(stderr, "Memory allocation failed.\n");
    goto cleanup;
  }
  for (int i = 0; i < atoi(searchNum); i++) {
    fscanf(fp, "%d %511s %d %4095s %262143s %lf %lf %d %31s", &category,
           keyword, &id,
           fileName, context, &val1, &val2, &r, color);
    nodeMetas[i].category = category;
    nodeMetas[i].id = id;
    snprintf(nodeMetas[i].keyword, sizeof(nodeMetas[i].keyword), "%s", keyword);
    snprintf(nodeMetas[i].fileName, sizeof(nodeMetas[i].fileName), "%s", fileName);
    snprintf(nodeMetas[i].context, sizeof(nodeMetas[i].context), "%s", context);
    nodeMetas[i].cx = val1;
    nodeMetas[i].cy = val2;
    nodeMetas[i].r = r;
    snprintf(nodeMetas[i].color, sizeof(nodeMetas[i].color), "%s", color);
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

  // エッジの座標をロード
  fp = fopen(fn1, "r");
  if (!fp) {
    fprintf(stderr, "Unknown file = %s\n", fn1);
    return;
  }
  for (int i = 0; i < graphDocCount; i++) {
    for (int j = 0; j < graphAdjCount[i]; j++) {
      int k = graphAdj[i][j];
      if (k > i && k >= 0 && k < simCtx.docCount) {
        edgeSimilarityCount++;
      }
    }
  }
  edgeSimilarities = (double *)malloc(sizeof(double) * edgeSimilarityCount);
  if (!edgeSimilarities && edgeSimilarityCount > 0) {
    fclose(fp);
    fprintf(stderr, "Memory allocation failed.\n");
    goto cleanup;
  }

  int edgeIndex = 0;
  for (int i = 0; i < graphDocCount; i++) {
    sparseDocument *leftDoc = &simCtx.docs[i];
    for (int j = 0; j < graphAdjCount[i]; j++) {
      int k = graphAdj[i][j];
      if (k <= i || k < 0 || k >= simCtx.docCount) {
        continue;
      }
      sparseDocument *rightDoc = &simCtx.docs[k];
      int leftIndex = 0;
      int rightIndex = 0;
      double similarity = 0.0;
      while (leftIndex < leftDoc->termCount && rightIndex < rightDoc->termCount) {
        int leftTermId = leftDoc->termIds[leftIndex];
        int rightTermId = rightDoc->termIds[rightIndex];
        if (leftTermId == rightTermId) {
          similarity += leftDoc->weights[leftIndex] * rightDoc->weights[rightIndex];
          leftIndex++;
          rightIndex++;
        } else if (leftTermId < rightTermId) {
          leftIndex++;
        } else {
          rightIndex++;
        }
      }
      edgeSimilarities[edgeIndex++] = similarity;
    }
  }

  edgeIndex = 0;
  for (int i = 0; i < graphDocCount; i++) {
    for (int j = 0; j < graphAdjCount[i]; j++) {
      int k = graphAdj[i][j];
      if (k <= i || k < 0 || k >= simCtx.docCount) {
        continue;
      }
      if (fscanf(fp, "%lf %lf %lf %lf", &x1, &y1, &x2, &y2) != 4) {
        fclose(fp);
        fprintf(stderr, "Invalid edge file body = %s\n", fn1);
        goto cleanup;
      }
      cJSON *edge = cJSON_CreateObject();
      cJSON_AddNumberToObject(edge, "x1", x1);
      cJSON_AddNumberToObject(edge, "y1", y1);
      cJSON_AddNumberToObject(edge, "x2", x2);
      cJSON_AddNumberToObject(edge, "y2", y2);
      if (edgeIndex < edgeSimilarityCount) {
        cJSON_AddNumberToObject(edge, "similarity", edgeSimilarities[edgeIndex]);
      }
      cJSON_AddNumberToObject(edge, "sourceId", nodeMetas[i].id);
      cJSON_AddStringToObject(edge, "sourceFileName", nodeMetas[i].fileName);
      cJSON_AddNumberToObject(edge, "targetId", nodeMetas[k].id);
      cJSON_AddStringToObject(edge, "targetFileName", nodeMetas[k].fileName);
      edgeIndex++;
      cJSON_AddItemToArray(edges, edge);
    }
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

cleanup:
  for (int i = 0; i < simCtx.docCount; i++) {
    free(simCtx.docs[i].termIds);
    free(simCtx.docs[i].weights);
  }
  free(simCtx.docs);
  if (graphAdj) {
    for (int i = 0; i < graphDocCount; i++) {
      free(graphAdj[i]);
    }
  }
  free(graphAdj);
  free(graphAdjCount);
  free(edgeSimilarities);
  free(nodeMetas);
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
                  "./result/category.txt", args1, "./result/lblk.txt",
                  searchNum);
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
