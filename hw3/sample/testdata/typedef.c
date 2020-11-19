typedef int GLOBAL_INT;

GLOBAL_INT f() { return 0; }
GLOBAL_INT g(GLOBAL_INT x) { return 0; }
GLOBAL_INT h(GLOBAL_INT x[2]) { return 0; }

typedef int GLOBAL_INT_ARRAY[10];

void f2(GLOBAL_INT_ARRAY x) {}
void g2(GLOBAL_INT_ARRAY x[2]) {}

typedef GLOBAL_INT_ARRAY GLOBAL_MULTI_ARRAY[3][3][3];

void f3(GLOBAL_MULTI_ARRAY x) {}
void g3(GLOBAL_MULTI_ARRAY x[3][3][3]) {}

int main() {
  typedef int INT;
  typedef int TNI;
  typedef float FLOAT;
  typedef INT INTINT;
  typedef int INT;

  INT x = 0;
  TNI y = 0;
  INTINT z = 0;
  FLOAT f = 0;

  typedef int X[2];
  X A;
  typedef X Y;
  Y B;

  typedef X Z[2];
  Z zzz;

  typedef Z Q[2][3][4];

  Q asdasd;
  Q QQQ[12312];

  typedef void VOID;
  typedef VOID VOID1, VOID2, VOID3;
  typedef int INT1[1], INT2[2], INT123[1][2][3];
}
