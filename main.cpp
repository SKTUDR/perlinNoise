#include "DxLib.h" // DxLib のヘッダファイルをインクルード
#include <cmath>   // 数学関数（cos, sin, floor など）に必要
#include <vector>  // std::vector（動的配列）を使うため
#include <random>  // 乱数生成用

// 描画サイズ（ピクセル）
const int WIDTH = 1280;
const int HEIGHT = 720;

// パーリンノイズのグリッドサイズ（細かさ）
const int GRID_SIZE = 64;  // 小さいほど細かいノイズになる

// スムーズステップ関数（Perlin の補間用）
float fade(float t) {
    // 6t^5 - 15t^4 + 10t^3：滑らかに0→1に補間されるS字カーブ
    return t * t * t * (t * (t * 6 - 15) + 10);
}

// 線形補間関数（基本の補間）
float lerp(float a, float b, float t) {
    // aからbまで、割合tで補間（0<=t<=1）
    return a + t * (b - a);
}

// 内積計算（2D）
// 勾配ベクトルと距離ベクトルの内積を計算する
float dotGridGradient(int ix, int iy, float x, float y,
    const std::vector<std::vector<std::pair<float, float>>>& gradients) {

    // 距離ベクトル：グリッド点(ix, iy) から対象点(x, y) への差
    float dx = x - ix;
    float dy = y - iy;

    // グリッド点の勾配ベクトルを取得（すでにランダムで割り当て済み）
    auto grad = gradients[iy][ix];

    // 内積を返す：滑らかな変化のための基本値になる
    return dx * grad.first + dy * grad.second;
}

// パーリンノイズ計算関数
// グリッド空間座標 (x, y) に対してノイズ値を返す
float perlin(float x, float y,
    const std::vector<std::vector<std::pair<float, float>>>& gradients) {

    // グリッドの左上整数座標
    int x0 = (int)x;
    int y0 = (int)y;

    // グリッドの右下座標（次のグリッド点）
    int x1 = x0 + 1;
    int y1 = y0 + 1;

    // 小数部分を使って補間率を決定
    float sx = fade(x - x0); // X方向補間率
    float sy = fade(y - y0); // Y方向補間率

    // 4隅のグリッド点との内積を計算（影響度）
    float n0 = dotGridGradient(x0, y0, x, y, gradients);
    float n1 = dotGridGradient(x1, y0, x, y, gradients);
    float ix0 = lerp(n0, n1, sx); // 左上と右上をX方向に補間

    float n2 = dotGridGradient(x0, y1, x, y, gradients);
    float n3 = dotGridGradient(x1, y1, x, y, gradients);
    float ix1 = lerp(n2, n3, sx); // 左下と右下をX方向に補間

    // 上下をY方向に補間して最終ノイズ値を得る
    return lerp(ix0, ix1, sy);
}

// ランダムな単位ベクトルを生成（2D）
// 勾配ベクトルとして使用
std::pair<float, float> randomGradient(std::mt19937& gen) {
    std::uniform_real_distribution<float> dist(0.0f, 2.0f * 3.1415926f);
    float angle = dist(gen);  // 0〜360度（ラジアン）
    return { std::cos(angle), std::sin(angle) }; // 単位ベクトル（方向のみ）
}

// メイン関数（Win32 API形式）
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    // DxLibの初期化（失敗したら終了）
    if (DxLib_Init() == -1) return -1;

    // 裏画面に描画（ダブルバッファリング）
    SetDrawScreen(DX_SCREEN_BACK);
    SetGraphMode(WIDTH, HEIGHT, 32);
    ChangeWindowMode(TRUE);

    // 勾配ベクトルグリッドのサイズを決定
    int gridW = WIDTH / GRID_SIZE + 2; // ピクセルに合わせて+2余裕持たせる
    int gridH = HEIGHT / GRID_SIZE + 2;

    // 勾配ベクトルを格納する2次元ベクトル配列
    std::vector<std::vector<std::pair<float, float>>> gradients(
        gridH, std::vector<std::pair<float, float>>(gridW));

    // 乱数生成器（固定シードで再現可能に）
    std::mt19937 rng(42);

    // グリッド全体にランダムな勾配ベクトルを割り当て
    for (int y = 0; y < gridH; y++) {
        for (int x = 0; x < gridW; x++) {
            gradients[y][x] = randomGradient(rng);
        }
    }

    // 描画処理：各ピクセルごとにパーリンノイズ値を計算
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            // グリッド空間に変換（ピクセル座標を小数化）
            float fx = (float)x / GRID_SIZE;
            float fy = (float)y / GRID_SIZE;

            // ノイズ値を取得（-1〜1）
            float n = perlin(fx, fy, gradients);

            // ノイズ値を0〜255のグレースケールに変換
            n = (n + 1.0f) / 2.0f;  // -1〜1 → 0〜1
            int gray = (int)(n * 255);

            // ピクセルを描画（グレースケール）
            DrawPixel(x, y, GetColor(gray, gray, gray));
        }
    }

    // メインループ（ESCキーが押されるまで）
    while (!ProcessMessage() && !CheckHitKey(KEY_INPUT_ESCAPE)) {
        ScreenFlip(); // 裏画面を表画面に反映
    }

    // DxLibの終了処理（リソース解放）
    DxLib_End();
    return 0;
}
