// DxLib のヘッダファイル（描画や画面制御に使用）
#include "DxLib.h"

#include <cmath>    // 数学関数（sin, cos など）用
#include <vector>   // ベクタ型を使用するため
#include <random>   // 乱数生成用

// 画面サイズ（描画するピクセル領域）
const int WIDTH = 1280;   // 横幅（ピクセル）
const int HEIGHT = 720;  // 高さ（ピクセル）

// グリッドの間隔（パーリンノイズの基本単位サイズ）
const int GRID_SIZE = 32;

// 補間関数：スムーズステップ（S字カーブ）
// t: [0.0〜1.0] の補間パラメータ
// 戻り値: tを滑らかにした値
float fade(float t) {
    return t * t * t * (t * (t * 6 - 15) + 10);
}

// 線形補間関数
// a, b: 補間元の2値
// t: 補間係数（0〜1）
// 戻り値: aとbをtで補間した値
float lerp(float a, float b, float t) {
    return a + t * (b - a);
}

// ドット積計算：グリッドの勾配ベクトルと対象点からの距離ベクトルの内積を求める
// ix, iy: 勾配ベクトルのグリッド座標
// x, y: 対象点の座標（連続空間）
// gradients: 勾配ベクトルの2次元配列
// 戻り値: 点(x,y)とグリッド(ix,iy)のベクトルの内積
float dotGridGradient(int ix, int iy, float x, float y,
    const std::vector<std::vector<std::pair<float, float>>>& gradients) {

    // 点とグリッドの差分（距離ベクトル）
    float dx = x - ix;
    float dy = y - iy;

    // 勾配ベクトルの取得（ランダムに与えられている）
    auto grad = gradients[iy][ix];

    // 距離ベクトルと勾配ベクトルの内積
    return dx * grad.first + dy * grad.second;
}

// パーリンノイズ計算関数（1オクターブ）
// x, y: ノイズ空間上の座標（float）
// gradients: 勾配ベクトルの2次元配列
// 戻り値: パーリンノイズ値（範囲は概ね -1.0〜1.0）
float perlin(float x, float y,
    const std::vector<std::vector<std::pair<float, float>>>& gradients) {

    // 対象座標の左上整数グリッド
    int x0 = (int)x;
    int y0 = (int)y;

    // 右隣・下隣のグリッド
    int x1 = x0 + 1;
    int y1 = y0 + 1;

    // 小数部分を補間パラメータにする（S字補間に備える）
    float sx = fade(x - x0);
    float sy = fade(y - y0);

    // 4つのグリッド点に対する内積計算
    float n0 = dotGridGradient(x0, y0, x, y, gradients);
    float n1 = dotGridGradient(x1, y0, x, y, gradients);
    float ix0 = lerp(n0, n1, sx); // 上辺補間

    float n2 = dotGridGradient(x0, y1, x, y, gradients);
    float n3 = dotGridGradient(x1, y1, x, y, gradients);
    float ix1 = lerp(n2, n3, sx); // 下辺補間

    // 上下を補間して最終ノイズ値にする
    return lerp(ix0, ix1, sy);
}

// 勾配ベクトルをランダムに生成
// gen: 乱数エンジン
// 戻り値: 単位長の2Dベクトル（cosθ, sinθ）
std::pair<float, float> randomGradient(std::mt19937& gen) {
    std::uniform_real_distribution<float> dist(0.0f, 2.0f * 3.1415926f);
    float angle = dist(gen); // 0〜2πのランダム角度
    return { std::cos(angle), std::sin(angle) }; // 単位ベクトル
}


int WINAPI WinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR lpCmdLine,
    _In_ int nShowCmd) 
{
    // DxLib 初期化（失敗時は終了）
    if (DxLib_Init() == -1) return -1;

    // 描画先を裏画面に設定（ダブルバッファリング）
    SetDrawScreen(DX_SCREEN_BACK);

    // 勾配ベクトルを格納する2次元ベクトル
    // グリッド点は (WIDTH / GRID_SIZE + 2) × (HEIGHT / GRID_SIZE + 2)
    int gridW = WIDTH / GRID_SIZE + 2;
    int gridH = HEIGHT / GRID_SIZE + 2;

    // 乱数生成器（種は固定して毎回同じパターンに）
    std::mt19937 rng(123);

    // 勾配ベクトルを各グリッド点に割り当てる
    std::vector<std::vector<std::pair<float, float>>> gradients(
        gridH, std::vector<std::pair<float, float>>(gridW));
    for (int y = 0; y < gridH; y++) {
        for (int x = 0; x < gridW; x++) {
            gradients[y][x] = randomGradient(rng);
        }
    }

    // 全画素に対してノイズを計算し、画面に描画
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            // 現在のピクセルをノイズ空間に正規化（グリッドサイズで割る）
            float fx = (float)x / GRID_SIZE;
            float fy = (float)y / GRID_SIZE;

            // パーリンノイズ値（-1.0〜1.0 程度）
            float n = perlin(fx, fy, gradients);

            // 値を 0〜255 にマッピング（グレースケール）
            n = (n + 1.0f) / 2.0f;     // -1〜1 → 0〜1
            int gray = (int)(n * 255); // 0〜255

            // ピクセルを描画（RGB同値でグレースケール）
            DrawPixel(x, y, GetColor(gray, gray, gray));
        }
    }

    // 終了までメッセージループ（ESCキーで終了）
    while (!ProcessMessage() && !CheckHitKey(KEY_INPUT_ESCAPE)) {
        ScreenFlip(); // 裏画面を表画面に反映（表示）
    }

    // DxLib 終了処理
    DxLib_End();
    return 0;
}
