int fn1() { return 1; }
float fn3() { return 1.0; }
int fn4(int a, int b) { return 1; }
int expr(int a, int b, int c, int d, float fa, float fb, float fc) {
  int i, j, k, l;
  float fi = 1.0, fj = 2.0, fk = 3.0, fl = 4.0;

  fi = 1.2 * fi + -fj * (fl - fk * fn3());
  fi = -fn3() - (-(-(4)));
  fi = !fn3() - (!(!(4)));
  i = !fn1();
  i = 1 < 2;
  i = 1 > 2;
  i = 1 >= 2;
  i = 1 <= 2;
  i = 1 != 2;
  i = 1 == 2;
  i = fn4(1 + 3 * 4 * fn4(2, 3), 3);
  return 1;
}
int i, j, k;
void f() {
  int a, b, c, d, e;
  i = i == 0;
  i = (i == 0);
  j = (k * 100) / 100;
  j = (i * 5 == 0 && i * 3 != 0) || (i * 5 != 0 && i * 3 == 0);
  e = (a - c) / (b - d) == c / d;
  a = (b * b - 4 *a * c == 0);
  d = (2 * (a * b + b * c + c * a) == a *b * c);
  a = -a;
  a = 2 * (b + c);
  a = (b + c) * 2;
  a = 0;
  a = -1;
  a = 1;
  if (a + b > c && c > d) i = i;
  else i = 0;
}
int main() {
  int a = 0;
  int b, c, d;
  a = 1 + 2;
  a = 1 * 2;
  a = 1 / 2;
  a = 1 - 2;

  a = a == 1;
  a = a != 1;
  a = a > 1;
  a = a < 1;
  a = a >= 1;
  a = a <= 1;
  a = 0 && 1;
  a = 1 || 0;

  a = !1;

  a = (1 + 2);
  a = 1 + !1;
  a = 1 * !1;
  a = !(b == c + 123 * 5 + d && d);
  a = b && c + a || c + !a || (c + !(a || c) + a * !2) + (((a)) + !(b) && fn1());

  a = 1 + 2 * 3;
  a = (1 + 2) * 3;
}
