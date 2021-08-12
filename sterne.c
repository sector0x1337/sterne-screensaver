#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <math.h>
#include <windows.h>

char *clsName = "Sterne-1.0";
int xmax, ymax;
POINT maus;

struct stern {
    int visible;
    int mitteX, mitteY;
    int d;
    double grad;
    double s, c;
    double x, y;
    int steps;
    uint8_t col[3];
};

#define MAX_STERNE 100
struct stern screen[MAX_STERNE];
int step=0;

void sterneInit(struct stern *s) {
    int i;
    int vk, nk;

    for (i=0;i<MAX_STERNE;i++) {
        s[i].steps=0;
        s[i].visible=0;
        s[i].d=(rand()%40)+10;
        s[i].mitteX=rand()%xmax;
        s[i].mitteY=rand()%ymax;
        s[i].x=1.0;
        s[i].y=0.0;

        vk=rand()%4;
        nk=rand()%10000;
        s[i].grad=(1.0*vk)+(nk/0.0001);

        s[i].s=sin(s[i].grad);
        s[i].c=cos(s[i].grad);

        s[i].col[0]=rand()&0xff;
        s[i].col[1]=rand()&0xff;
        s[i].col[2]=rand()&0xff;
    }
}


int sterneStep(struct stern *s, HDC hdc) {
    int i;
    double nx, ny, norm;
    HPEN hpen;
    int x,y;
    int x1,y1,x2,y2;
    char col[3];


    if ((step%20)==0) {
        for (i=0;i<MAX_STERNE;i++) {
            if (s[i].visible==0) {
                s[i].visible=1;
                break;
            }
        }
        if (i==MAX_STERNE) {
            RECT rct;
            HBRUSH hbr = CreateSolidBrush(RGB(0,0,0));
            if (hbr==NULL) {
                return 0;
            }
            rct.top=0;
            rct.left=0;
            rct.right=xmax;
            rct.bottom=ymax;
                
            if (FillRect(hdc, &rct, hbr)==0) {
                return 0;
            }
            DeleteObject(hbr);
            sterneInit(s);
            s[0].visible=1;
        }
    }
    step++;

    for (i=0;i<MAX_STERNE && s[i].visible!=0;i++) {
        HGDIOBJ obj;

        if (s[i].steps>50) {
            continue;
        }
        nx = s[i].c*s[i].x - s[i].s*s[i].y;
        ny = s[i].c*s[i].y + s[i].s*s[i].x;
        norm = sqrt(nx*nx+ny*ny);
        nx=nx/norm;
        ny=ny/norm;
        s[i].x=nx;
        s[i].y=ny;
        x = ceil(nx*s[i].d);
        y = ceil(ny*s[i].d);
        x1=s[i].mitteX;
        y1=s[i].mitteY;
        x2=s[i].mitteX+x;
        y2=s[i].mitteY+y;
        s[i].col[0]=s[i].col[0]^(rand()&0x3f);
        s[i].col[1]=s[i].col[1]^(rand()&0x3f);
        s[i].col[2]=s[i].col[2]^(rand()&0x3f);

        hpen=CreatePen(PS_SOLID, 1, RGB(s[i].col[0], s[i].col[1], s[i].col[2]));
        if (hpen==NULL) {
            return 0;
        }
        obj=SelectObject(hdc, hpen);
        if (obj==NULL || obj==HGDI_ERROR) {
            return 0;
        }
        if (MoveToEx(hdc, x1, y1, NULL)==0) {
            return 0;
        }
        if (LineTo(hdc, x2, y2)==0) {
            return 0;
        }
        DeleteObject(hpen);

        s[i].steps++;
    }
    return 1;
}

enum runtype { SCR_PREV, SCR_CONFIG, SCR_RUN, SCR_ERRPREV };

LRESULT CALLBACK wndProc(HWND, UINT, WPARAM, LPARAM);

enum runtype getRunType(LPSTR args, HWND *hWnd) {
    long long res;

    if (strlen(args)<2) {
        return SCR_RUN;
    }
    if (*args!='/') {
        return SCR_CONFIG;
    }
    args++;
    switch (*args) {
        case 'c':
        case 'C':
            return SCR_CONFIG;
        
        case 'p':
        case 'P':
            args++;
            if (strlen(args)==0) {
                return SCR_ERRPREV;
            }
            res=0;
            while (*args==' ') {
                args++;
            }
            while (*args!='\0') {
                char c=*args;
                int ziff;
                args++;
                if (c<'0' || c>'9') {
                    return SCR_ERRPREV;
                }
                ziff=c-'0';
                res=10*res+ziff;
            }
            *hWnd=(HWND)res;
            return SCR_PREV;
    }

    return SCR_RUN;
}

int WINAPI WinMain(HINSTANCE hThisInstance, HINSTANCE hPrevInstance, LPSTR szCmdLine, int show) {
    MSG messages;
    WNDCLASSEX wndcl;
    HWND hWnd;

    wndcl.cbClsExtra=0;
    wndcl.cbSize=sizeof(WNDCLASSEX);
    wndcl.cbWndExtra=0;
    wndcl.hbrBackground=(HBRUSH)CreateSolidBrush(RGB(0,0,0));
    wndcl.hCursor=NULL;
    wndcl.hIcon=LoadIcon(NULL, IDI_APPLICATION);
    wndcl.hIconSm=LoadIcon(NULL, IDI_APPLICATION);
    wndcl.hInstance=hThisInstance;
    wndcl.lpfnWndProc=wndProc;
    wndcl.lpszClassName=clsName;
    wndcl.lpszMenuName=NULL;
    wndcl.style=0;

    srand(time(NULL));

    switch (getRunType(szCmdLine, &hWnd)) {
        case SCR_RUN:
            xmax = GetSystemMetrics(SM_CXSCREEN);
            ymax = GetSystemMetrics(SM_CYSCREEN);

            if (xmax==0 || ymax==0) {
                MessageBox(NULL, "Konnte Bildschirmdimension nicht abfragen.", "GetSystemMetrics", MB_ICONERROR|MB_OK);
                return -1;
            }

            if (RegisterClassEx(&wndcl)==0) {
                MessageBox(NULL, "Konnte Fensterklasse nicht registrieren.", "RegisterClassEx", MB_ICONERROR|MB_OK);
                return -1;
            }

            hWnd=CreateWindowEx(
                WS_EX_APPWINDOW, clsName, NULL, WS_VISIBLE, 0, 0,
                xmax, ymax, HWND_DESKTOP, NULL, hThisInstance,  NULL
            );
            if (hWnd==NULL) {
                MessageBox(NULL, "Fenster konnte nicht erstellt werden.", "CreateWindowEx", MB_ICONERROR|MB_OK);
                return -1;
            }

            if (GetCursorPos(&maus)==0) {
                MessageBox(NULL, "Kursorposition konnte nicht ermittelt werden.", "GetCursorPos",MB_ICONERROR|MB_OK);
                return -1;
            }
            if (ShowWindow(hWnd, show)==0) {
                MessageBox(NULL, "Bildschirmschoner konnte nicht hergestellt werden", "ShowWindow", MB_ICONERROR|MB_OK);
                return -1;
            }
            break;

        case SCR_PREV: {
            return 0; // Vorschau bisher nicht implementiert.
        }
        case SCR_CONFIG:
            MessageBox(0,"Dieser Bildschirmschoner ist nicht konfigurierbar.",0 , 0);
            return 1;

        case SCR_ERRPREV:
            MessageBox(0,"Ungueltige Parameter.", 0, 0);
            break;
    }

    sterneInit(screen);
    while (GetMessage(&messages, NULL, 0, 0)) {
        TranslateMessage(&messages);
        DispatchMessage(&messages);
    }

    return 0;
}

LRESULT CALLBACK wndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    RECT rct;

    switch (msg) {
        case WM_CREATE: {
            UINT_PTR tid=1;
            DWORD dw;

            dw=GetWindowLong(hWnd, GWL_STYLE);
            dw=dw&(~(WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX|WS_MAXIMIZEBOX));
            SetWindowLong(hWnd, GWL_STYLE, dw);
            SetTimer(hWnd, tid, 50, NULL);
            break;
        }

        case WM_TIMER: {
            HDC hdc;
            HBRUSH col;
            int x, y;
            unsigned p=0;

            hdc=GetDC(hWnd);
            if (sterneStep(screen, hdc)==0) {
                PostQuitMessage(-1);
                break;
            }
            ReleaseDC(hWnd, hdc);
            break;
        }

        case WM_CLOSE:
            PostQuitMessage(0);
            break;


        //DefScreenSaverProc:
        case WM_ACTIVATE:
        case WM_ACTIVATEAPP:
        case WM_NCACTIVATE:
            if (wParam==FALSE) {
                SendMessage(hWnd, WM_CLOSE, 0, 0);
            }
            break;

        case WM_SETCURSOR:
            SetCursor(NULL);
            return TRUE;

        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_KEYDOWN:
        case WM_KEYUP:
            PostQuitMessage(0);
            break;

        case WM_MOUSEMOVE: {
            DWORD lmp=(maus.y<<16)|maus.x;
            if (lmp!=lParam) {
                PostQuitMessage(0);
            }
            return 0;
        }

        case WM_DESTROY:
            SendMessage(hWnd, WM_CLOSE, 0, 0);
            break;

        case WM_SYSCOMMAND:
            if (wParam==SC_CLOSE || wParam==SC_SCREENSAVE) {
                return FALSE;
            }
            break;

        default:
            return DefWindowProc(hWnd, msg, wParam, lParam);
    }

    return 0;
}