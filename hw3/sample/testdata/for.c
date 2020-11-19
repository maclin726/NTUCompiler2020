int A;
int f(int x) { return x < 10; }
int g(int x) { return x + 1; }
void h() { A = 100; }
int h2() { A = A + 1; }
int main() {
  int i, j, k, N;

  for (i = 0; i < 10; i = i + 1) { }
  for (i = 0; i < 10; i = i + 1);

  for (i = 0; i < 10; i = i + 1) {
    for (j = 0; j < N; j = j + 1);
    for (j = N - 1; j >= 0; j = j - 1) {

    }
    for (j = 0; ; ) {}
    for (; ; ) {}
    for (j = 0; j < N; i = i + 1) 
      for (k = j; k < N; k = k + 1) {}
  }

  for (i = 0; i < 10; i = i + 1)
    for (i = 10; i > 0; i = i - 1) {
      for (j = 3; j < 7; j = j * 2 + 1);
    }

  for (; j < N; j = j + 1);
  for (i = 0, j = 0; i < N; i = i + 1, j = j - 1) {
    for (k = i; k >= 0; k = k - 1, k = k - 1) ;
  }

  for (i = 0; i < N && i >= 0; i = !i && 1 + 1) {
    A = A + 1; 
  }

  for (; 1; ) {}

  for (i = 0; f(i); i = g(i))
    i = !i;

  for (h(), h2(); i < 10; h(), h2(), i = i + 1) {
    i = h2();
    i = h2();
    i = h2();
  }
}
