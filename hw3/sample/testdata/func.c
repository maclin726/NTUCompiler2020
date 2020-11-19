void f() {}
int g() { return 1; }
int b;
int h() { return b; }
int f1(int x) { return x * 2; }
int f2(int x, int y) { return x - y; }
float g1() { return 1.2; }
float g2(float x) { return 1.0 / x; }
int f3(int a[2]) { return a[1]; }
int f4(int a[2][3]) { return a[0][1]; }
int f5(int a[][10]) { return a[0][0] + g2(0); }
void f6(int a[]) {}

int fib(int n) {
  if (n <= 1)
    return 1;
  return fib(n - 1) + fib(n - 2);
}

int main() {
  int b[2], c[2][3], d[100][10];
  int y;
  f3(b);
  f4(c);
  f5(d);
  f6(b);
  f();
  y = g2(f1(f1(0)));
  y = f2(y, fib(y));
}
