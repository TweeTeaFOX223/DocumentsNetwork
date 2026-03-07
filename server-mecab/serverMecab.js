import express from "express";
import MeCab from "mecab-async";

/* ==== Begin Function ==== */
function preprocessing(text) {
  /*
        文章を以下の形式に整形する関数
        ・ 数値を 0 に統一
        ・ 「,」を「、」に置換
        ・ 「\t」を削除
        ・ 「\n」を削除
        ・ 不要な連続空白を 1 つに統一
        ・ アルファベットを小文字に統一
        ・ 記号を削除
        ・ 英数字連結語は前後に空白を入れて境界を維持
    */
  const punctuation = `!"#$%&'()*+\\-./:;<=>?@[\\]^_{|}~「」〔〕“”〈〉『』【】＆＊・（）＄＃＠。、？！｀＋￥％■※`;
  const regexPunctuation = new RegExp(`[${punctuation}]`, "g");
  text = text.replace(/[0-9]+/g, "0");
  text = text.replace(/,/g, "、");
  text = text.replace(/\t/g, "");
  text = text.replace(/[\n\r]/g, "");
  text = text.toLowerCase();
  text = text.replace(regexPunctuation, "");
  // 英数字列を日本語から分離して、未知語が1文字ずつ崩れにくい状態にする
  text = text.replace(/([a-z0-9]+)/g, " $1 ");
  text = text.replace(/[ \u3000]+/g, " ").trim();
  return text;
}

function parseToWakachi(text, mecab) {
  /* わかち書き文章を生成する関数 */
  return new Promise((resolve, reject) => {
    mecab.wakachi(text, (err, result) => {
      if (err) {
        reject(err);
      } else {
        resolve(result);
      }
    });
  });
}
/* ==== End Function ==== */

/* ==== Begin MeCab Setting ==== */
// const dicDir  = "/usr/lib/x86_64-linux-gnu/mecab/dic/mecab-ipadic-neologd";
const mecab = new MeCab();
// mecab.command = `mecab -d ${dicDir}`;
/* ==== End MeCab Setting ==== */

/* ==== Begin Server Setting ==== */
const app = express();
const port = process.env.PORT || 4000;
const allowedOrigin = process.env.ALLOWED_ORIGIN || "http://localhost:3000";

app.use(express.json());
app.use((req, res, next) => {
  res.setHeader("Access-Control-Allow-Origin", allowedOrigin);
  res.setHeader("Access-Control-Allow-Methods", "*");
  res.setHeader("Access-Control-Allow-Headers", "*");
  res.setHeader("Access-Control-Allow-Credentials", "true");
  if (req.method === "OPTIONS") {
    return res.sendStatus(204); // No Content
  }
  next();
});
/* ==== End Server Setting ==== */

/* ==== Begin POST Server ==== */
app.post("/", async (req, res) => {
  const text = req.body.text;
  try {
    let words = await preprocessing(text);
    words = await parseToWakachi(words, mecab);
    res.json({ wakachiSentence: words.join(" ") });
  } catch (err) {
    console.error(err);
    res.status(500).send("わかち書き中にエラーが発生しました");
  }
});
/* ==== End POST Server ==== */

/* ==== Begin PORT Setting ==== */
app.listen(port, () => {
  console.log(`Express server listening on port ${port}`);
});
/* ==== End PORT Setting ==== */
