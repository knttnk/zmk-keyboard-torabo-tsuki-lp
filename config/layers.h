// レイヤーの番号。config/keymap.keymapのレイヤーの並び順と一致させる。
// snippets/knttnk-pointing/knttnk-pointing.overlayでも、この番号を使っている。
// 読み込むのはキーマップだけにする。2通りのパスから読み込むと、ビルドが止まる。
// 0から3はknttnk/keyballのmykeymapと同じ番号にしている。

#define L_BASE 0   /* 文字入力 */
#define L_NAV 1    /* ナビゲーションと記号 */
#define L_FN 2     /* ファンクションキー、画面の明るさ、音量 */
#define L_MOUSE 3  /* 矢印キーとマウスボタン */
#define L_BT 4     /* Bluetoothの操作 */
#define L_SCROLL 5 /* スクロール専用 */
