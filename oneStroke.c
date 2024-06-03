#include <windows.h>
#include <scrnsave.h>
#include <gl/gl.h>
#include <stdio.h>
#include <sys/types.h>
#include <process.h>
#define N 20

#define WIDTH 256 //画像の横のピクセル数
#define HEIGHT 256 //画像の縦のピクセル数

void EnableOpenGL(void);
void DisableOpenGL(HWND);
HDC hDC;
HGLRC hRC;
int wx, wy;
GLubyte * bits;
int finish = 0;

void readBits (GLubyte * bits, char * fileName, int width, int height) {
  FILE *fp;

  if ((fp=fopen(fileName,"rb"))==NULL) { /* 画像データ読み込み */
    fprintf (stderr, "file not found.\n");
    exit(0);
  }
  fseek(fp,54,SEEK_SET);/* ヘッダ54バイトを読み飛ばす */

  /* 画像を反転させずに読み込み */
  for (int i=0; i<width*height; i++){ /* データをBGRをRGBの順に変えて読み込み */
    fread(&bits[2], 1, 1, fp);
    fread(&bits[1], 1, 1, fp);
    fread(&bits[0], 1, 1, fp);
    bits=bits+3;
  }
  fclose(fp);
}

void readBitsAlpha (GLubyte * bits, char * fileName, int width, int height) {
  FILE *fp;

  if ((fp=fopen(fileName,"rb"))==NULL) { /* 画像データ読み込み */
    fprintf (stderr, "file not found.\n");
    exit(0);
  }
  fseek(fp,54,SEEK_SET);/* ヘッダ54バイトを読み飛ばす */

/* 画像を反転させずに読み込み */
  for (int i=0; i<width*height; i++){ /* データをBGRをRGBの順に変えて読み込み、alpha値を追加 */
    fread(&bits[2], 1, 1, fp);
    fread(&bits[1], 1, 1, fp);
    fread(&bits[0], 1, 1, fp);
    if(bits[2]==255&&bits[1]==255&&bits[0]==255)
      bits[3]=0; // 白なら透明
    else
      bits[3]=255; // それ以外は不透明
    bits=bits+4;
  }
  fclose(fp);
}

void readBitsRev (GLubyte * bits, char * fileName, int width, int height) {
  FILE *fp;

  if ((fp=fopen(fileName,"rb"))==NULL) { /* 画像データ読み込み */
    fprintf (stderr, "file not found.\n");
    exit(0);
  }
  fseek(fp,54,SEEK_SET);/* ヘッダ54バイトを読み飛ばす */

  // 画像を上下反転させて読み込み。左右は反転させない。
  for (int h=height-1; h>=0; h--) { // 上下反転
    for (int w=0; w<width; w++){ // 左右は反転させないが、BGRをRGBの順に変えて読み込み
      int i;
      i=(width*h+w)*3;
      fread(&bits[i+2], 1, 1, fp);
      fread(&bits[i+1], 1, 1, fp);
      fread(&bits[i], 1, 1, fp);
    }
  }
  fclose(fp);
}

void drawbackground(GLubyte *bits) {
  glDrawPixels(1920, 1080, GL_RGB, GL_UNSIGNED_BYTE, bits);
}

void display (char * ssd, GLuint * textureID)
{
  static int i=0;
  char direction;
  // 左上の座標(+7.0, +5.0)
  static double locx = -19.18;
  static double locy = 15.0;
  static double vertices[5][2] = { // 五角形の頂点座標
    {0.0, 30.0},     // 上部の頂点
    {-26.18, 10.0},  // 左上の頂点
    {-16.18, -22.36},// 左下の頂点
    {16.18, -22.36}, // 右下の頂点
    {26.18, 10.0}    // 右上の頂点
  };


  static int index=0;

  i=i % 100;
  direction = ssd[i];
  i++;
  switch (direction) {
  case 'U':
      locx += (vertices[4][0]-vertices[1][0])/N;
      locy += (vertices[4][1]-vertices[1][1])/N;
      break;
  case 'D':
      locx += (vertices[2][0]-vertices[4][0])/N;
      locy += (vertices[2][1]-vertices[4][1])/N;
      break;
  case 'R':
      locx += (vertices[0][0]-vertices[2][0])/N;
      locy += (vertices[0][1]-vertices[2][1])/N;
      break;
  case 'L':
      locx += (vertices[3][0]-vertices[0][0])/N;
      locy += (vertices[3][1]-vertices[0][1])/N;
      break;
  case 'A':
      locx += (vertices[1][0]-vertices[3][0])/N;
      locy += (vertices[1][1]-vertices[3][1])/N;
      break;
  default: // 未知の文字が来たら終了
      fprintf(stderr, "Invalid character.\n");
      exit(0);
  }

  glClear(GL_COLOR_BUFFER_BIT);
  drawbackground(bits); // 背景描画

  glEnable(GL_TEXTURE_2D);
  if (i==1) {
    index=(index+1)%2;
    glBindTexture(GL_TEXTURE_2D, textureID[index]);
  }

  glBegin(GL_POLYGON);
  glTexCoord2f(0, 0); glVertex2f(locx-20.0, locy-20.0);
  glTexCoord2f(0, 1); glVertex2f(locx-20.0, locy);
  glTexCoord2f(1, 1); glVertex2f(locx, locy);
  glTexCoord2f(1, 0); glVertex2f(locx, locy-20.0);
  glEnd();
  glDisable(GL_TEXTURE_2D);
}

unsigned __stdcall disp (void *arg) {
  GLuint textureID[2];
  static char ssd[100];
  static FILE *fp=NULL;

  EnableOpenGL(); // OpenGL設定

  /* OpenGL初期設定 */
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(-50.0*wx/wy, 50.0*wx/wy, -50.0, 50.0, -50.0, 50.0); // 座標系設定

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  glClearColor(0.1f, 0.2f, 0.3f, 0.0f); // glClearするときの色設定
  glColor3f(0.9, 0.8, 0.7); // glRectf等で描画するときの色設定
  glViewport(0,0,wx,wy);
  /* ここまで */

  glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA); // 透明の計算方法の設定
  glEnable(GL_BLEND); // 透明処理をONに設定

  /* スクリーンセーバ記述プログラム読み込み */
  fp=fopen ("oneStroke", "r");
  if (fp==NULL) {
    fprintf (stderr, "ファイルオープン失敗\n");
    exit(0);
  }
  for (int i=0; i<100; i++)
    if (fscanf (fp, "%c", &ssd[i]) == EOF){ // 100文字なかったら終了
      fprintf (stderr, "The length of the input is less than 100.\n");
      exit(0);
    }
  fclose(fp);
  /* ここまで */

  /* bitmap画像を読み込み、textureに割り当てる */
  if ((bits = malloc (WIDTH*HEIGHT*4))==NULL) { /*  画像読み込み用領域確保 */
    fprintf (stderr, "allocation failed.\n");
    exit(0);
  }
  glGenTextures (2, textureID); /* texture idを2つ作成し、配列textureIDに格納する */


  /* 1つ目のtexture */
  glBindTexture(GL_TEXTURE_2D, textureID[0]);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  readBitsAlpha(bits, "star1.bmp", WIDTH, HEIGHT);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, WIDTH, HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, bits);
  /* ここまで */

  /* 2つ目のtexture */
  glBindTexture(GL_TEXTURE_2D, textureID[1]);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  readBitsAlpha(bits, "star2.bmp", WIDTH, HEIGHT);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, WIDTH, HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, bits);
  /* ここまで */

  if ((bits = realloc (bits, 1920*1080*3))==NULL) { /*  背景画像読み込み用領域確保 */
    fprintf (stderr, "allocation failed.\n");
    exit(0);
  }
  readBits(bits, "background.bmp", 1920, 1080);

  /* 表示関数呼び出しの無限ループ */
  while(1) {
    Sleep(15); // 0.001秒待つ
    display(ssd, textureID); // 描画関数呼び出し
    glFlush(); // 画面描画強制
    SwapBuffers(hDC); // front bufferとback bufferの入れ替え
    if (finish == 1) // finishが1なら描画スレッドを終了させる
      return 0;
  }
}

LRESULT WINAPI ScreenSaverProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  static HANDLE thread_id1;
  RECT rc;

  switch(msg) {
  case WM_CREATE:
    hDC = GetDC(hWnd);
    GetClientRect(hWnd, &rc);
    wx = rc.right - rc.left;
    wy = rc.bottom - rc.top;

    thread_id1=(HANDLE)_beginthreadex(NULL, 0, disp, NULL, 0, NULL); // 描画用スレッド生成
    if(thread_id1==0){
      fprintf(stderr,"pthread_create : %s", strerror(errno));
      exit(0);
    }
    break;
  case WM_ERASEBKGND:
    return TRUE;
  case WM_DESTROY:
    finish=1; // 描画スレッドを終了させるために1を代入
    WaitForSingleObject(thread_id1, INFINITE); // 描画スレッドの終了を待つ
    CloseHandle(thread_id1);
    DisableOpenGL(hWnd);
    PostQuitMessage(0);
    free(bits);
    return 0;
  default:
    break;
  }
  return DefScreenSaverProc(hWnd, msg, wParam, lParam);
}

BOOL WINAPI ScreenSaverConfigureDialog(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
  return TRUE;
}

BOOL WINAPI RegisterDialogClasses(HANDLE hInst)
{
  return TRUE;
}

void EnableOpenGL(void) {
  int format;
  PIXELFORMATDESCRIPTOR pfd = {
    sizeof(PIXELFORMATDESCRIPTOR), // size of this pfd
    1,                     // version number
    0
    | PFD_DRAW_TO_WINDOW   // support window
    | PFD_SUPPORT_OPENGL   // support OpenGL
    | PFD_DOUBLEBUFFER     // double buffered
    ,
    PFD_TYPE_RGBA,         // RGBA type
    24,                    // 24-bit color depth
    0, 0, 0, 0, 0, 0,      // color bits ignored
    0,                     // no alpha buffer
    0,                     // shift bit ignored
    0,                     // no accumulation buffer
    0, 0, 0, 0,            // accum bits ignored
    32,                    // 32-bit z-buffer
    0,                     // no stencil buffer
    0,                     // no auxiliary buffer
    PFD_MAIN_PLANE,        // main layer
    0,                     // reserved
    0, 0, 0                // layer masks ignored
  };

  /* OpenGL設定 */
  format = ChoosePixelFormat(hDC, &pfd);
  SetPixelFormat(hDC, format, &pfd);
  hRC = wglCreateContext(hDC);
  wglMakeCurrent(hDC, hRC);
  /* ここまで */
}

void DisableOpenGL(HWND hWnd)
{
  wglMakeCurrent(NULL, NULL);
  wglDeleteContext(hRC);
  ReleaseDC(hWnd, hDC);
}
