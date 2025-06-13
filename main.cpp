#include "DxLib.h"
#include <cmath>
#include <vector>
#include <random>

// === 設定定数 ===
static constexpr int WIDTH = 1280;        // 描画領域の横幅（ピクセル）
static constexpr int HEIGHT = 720;       // 描画領域の縦幅（ピクセル）
static constexpr int GRID_SIZE = 40;     // パーリンノイズのグリッドサイズ  値が大きいほど粗いノイズ　（現在は上記二つの定数の公約数でないと機能しない）


static constexpr int OCTAVES = 5;        // フラクタルノイズのオクターブ数（重ねる層の数）
static constexpr float PERSISTENCE = 0.5f; // 各オクターブの振幅の減衰率（次第に小さくなる）


// 補間用のスムージング関数（fade 関数、Perlin の定義に基づく）`
float fade(float t) {
    // 6t^5 - 15t^4 + 10t^3
    // イーズ曲線
	// t の値を 0.0 ～ 1.0 の範囲で滑らかに変化させる関数
	// 3次のスムージング関数で、t の値を滑らかに変化させる  
	// これにより、ノイズの変化が滑らかになり、ギザギザ感が減少する
    return t * t * t * (t * (t * 6 - 15) + 10);
}

// 線形補間
// a と b の間を t で補間する関数（t は 0.0 ～ 1.0 の範囲）
// tが0.0 のとき a、1.0 のとき b を返す
// t が 0.5 のときは a と b の中間値を返す
// これにより、ノイズの値を滑らかに変化させることができる
// 例えば、a = 0.0, b = 1.0, t = 0.5 のとき、0.5 を返す
float lerp(float a, float b, float t) {
    return a + t * (b - a);
}

// グリッド点と入力座標との距離ベクトルと、グリッドの勾配ベクトルの内積を計算
// 影響力の値を決定
// ix, iy はグリッドの整数座標、x, y は入力座標
float dotGridGradient(
    int ix, int iy, float x, float y,
    const std::vector<std::vector<std::pair<float, float>>>& gradients
) {
	// グリッドセルの左上の座標 (ix, iy) と入力座標 (x, y) の距離ベクトル
    float dx = x - ix;
    float dy = y - iy;

    const auto& grad = gradients[iy][ix];  // (ix, iy) のランダムな勾配ベクトル

	// 勾配ベクトルと距離ベクトルの内積を計算し、返す。
    return dx * grad.first + dy * grad.second;
}

// 単一のパーリンノイズの値を計算
float perlin(
    float x, float y,
    const std::vector<std::vector<std::pair<float, float>>>& gradients
) {
    // 対応するグリッドセルの整数座標
    int x0 = (int)x;
    int y0 = (int)y;
    int x1 = x0 + 1;
    int y1 = y0 + 1;

    // 補間用の係数
    float sx = fade(x - x0);
    float sy = fade(y - y0);

    // 各角の内積計算
    float n0 = dotGridGradient(x0, y0, x, y, gradients);
    float n1 = dotGridGradient(x1, y0, x, y, gradients);
    float ix0 = lerp(n0, n1, sx);

    float n2 = dotGridGradient(x0, y1, x, y, gradients);
    float n3 = dotGridGradient(x1, y1, x, y, gradients);
    float ix1 = lerp(n2, n3, sx);

    return lerp(ix0, ix1, sy);  // 最終的な補間結果（-1〜1）
}

// フラクタルノイズ（オクターブ付きパーリンノイズ）
// 複数スケールのパーリンノイズを合成
float fractalPerlin(float x, float y,
    const std::vector<std::vector<std::pair<float, float>>>& gradients) {

    float total = 0.0f;      // ノイズの合計値
    float frequency = 1.0f;  // 周波数（空間の細かさ）
    float amplitude = 1.0f;  // 振幅（影響の強さ）
    float maxValue = 0.0f;   // 振幅の合計（正規化用）

    for (int i = 0; i < OCTAVES; ++i) {
      
		// 周波数を上げ、振幅を下げていく
		// これにより、粗いノイズから細かいノイズへと変化する
		// 各オクターブで異なるスケールのノイズを生成
		// 例えば、最初のオクターブでは粗いノイズ、次のオクターブでは細かいノイズを生成する
		// これにより、フラクタルなノイズが生成される

          // パーリンノイズを加算
		// perlin 関数を呼び出して、現在の周波数と振幅でノイズを計算
		// ここでは、x と y の座標を周波数でスケーリングして、異なるスケールのノイズを生成
		// これにより、異なるスケールのノイズが合成され、フラクタルなパターンが生成される
        total += perlin(x * frequency, y * frequency, gradients) * amplitude;

        // 次のオクターブでは細かく・弱くする
		// 振幅の合計を更新
        maxValue += amplitude;

		// 振幅を減衰させ、周波数を倍にする
		// 振幅は PERSISTENCE で減衰し、周波数は倍増する
        amplitude *= PERSISTENCE;
        frequency *= 2.0f;
    }

    return total / maxValue; // -1〜1 に正規化
}


// ランダムな単位ベクトル（勾配）を生成
std::pair<float, float> randomGradient(std::mt19937& gen) {

	// 0〜2πの範囲でランダムな角度を生成し、単位ベクトルに変換
    std::uniform_real_distribution<float> dist(0.0f, 2.0f * 3.1415926f);
    float angle = dist(gen);
    return { std::cos(angle), std::sin(angle) };
}

// 地形の高さに応じて「森っぽい色」を返す関数
unsigned int getForestColor(float n) {
    if (n < 0.3f) return GetColor(20, 40, 100);         // 深い湖
    else if (n < 0.4f) return GetColor(60, 100, 100);   // 湿地帯・浅い湖
    else if (n < 0.5f) return GetColor(100, 180, 100);  // 草原
    else if (n < 0.65f) return GetColor(40, 100, 40);   // 森林（濃緑）
    else if (n < 0.8f) return GetColor(100, 80, 50);    // 岩場・丘
    else return GetColor(220, 220, 220);                // 山頂・雪
}

// メイン関数（DxLibのエントリーポイント）
int WINAPI WinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR lpCmdLine,
    _In_ int nShowCmd) 
{
    // DxLib 初期化
    if (DxLib_Init() == -1) return -1;
    SetDrawScreen(DX_SCREEN_BACK); // 描画先を裏画面に設定（ダブルバッファリング）
	SetGraphMode(WIDTH, HEIGHT, 16); // 描画領域のサイズと色深度を設定
	ChangeWindowMode(TRUE); // ウィンドウモードに変更（フルスクリーンではなく）

    // グリッドのサイズ（ノイズ用ベクトル配列）
    int gridW = WIDTH / GRID_SIZE * (1 << (OCTAVES - 1)) + 2;
    int gridH = HEIGHT / GRID_SIZE * (1 << (OCTAVES - 1)) + 2;

    // 乱数生成器（固定シードで毎回同じ）
    std::mt19937 rng(1234);

	// 各グリッドセルに対してランダムな勾配ベクトルを生成
	// 2次元ベクトルの配列を使用して、各グリッドセルの勾配を格納
	// ここでは、グリッドの幅と高さを計算して、各セルにランダムな勾配を割り当てる
    std::vector<std::vector<std::pair<float, float>>>  gradients(gridH, std::vector<std::pair<float, float>>(gridW));

	// 各グリッドセルにランダムな勾配を設定
	// 乱数生成器を使用して、各セルにランダムな単位ベクトルを割り当てる
	// これにより、各セルのノイズ計算に使用される勾配が決定される
    for (int y = 0; y < gridH; y++) {
        for (int x = 0; x < gridW; x++) {
            gradients[y][x] = randomGradient(rng);
        }
    }

    // ピクセル毎にノイズを計算して描画
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            float fx = (float)x / GRID_SIZE;
            float fy = (float)y / GRID_SIZE;

            // フラクタルパーリンノイズ（-1～1）を 0～1 に変換
            float n = fractalPerlin(fx, fy, gradients);
            n = (n + 1.0f) / 2.0f;

            // 色を取得して描画
            unsigned int color = getForestColor(n);
            DrawPixel(x, y, color);
        }
    }

    // ESC キーが押されるまで画面表示
    while (!ProcessMessage() && !CheckHitKey(KEY_INPUT_ESCAPE)) {
        ScreenFlip();  // 画面更新
    }

    DxLib_End(); // DxLib を終了
    return 0;
}
