int f() { return 0; }
float g() { return 0.0; }
int h(int x, int y) { return x + y; }
int p(int x) { return x * read(); }
void output(float Z) { write(Z); }
int main() {
  int x;
  float y;
  write("hello world\n");
  write(x);
  write(y);
  write(f());
  write(g());

  x = read();
  write(x);
  y = read();
  write(y);
  write(h(read(), read()));
}
