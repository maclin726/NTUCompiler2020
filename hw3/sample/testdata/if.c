int f(int x) { return 0; }
int g(int x, int y) { return x + y; }
int main() {
  int x = 0;
  int y = 0;
  int z, k;
  if (1 == 1) {}
  if (x == 1) {}
  if (1 == y) {}
  if (1 == 1);
  if (x == 1);
  if (1 == y);

  if (1 >= 2) {
    if (2 <= 3) {
      if (x == x) {}
      if (y == y) {}
    }
  }

  if (x > y) {

  } else {

  }

  if (x < y) {
    
  } else if (x > y) {

  } else {

  }

  if (x < y) 
    x = x + 1;
  else
    if (x > y)
      x = x + 1;
    else
      y = y - 1;

  if (x < y) 
    x = x + 1;
  else {
    if (x > y)
      x = x + 1;
    else
      y = y - 1;
  }

  if (x == 0) {
    if (y < 100);
    else if (y == x) {}

    if (y > 100);
    else if (y != 100) {
      if (1) {}
    }
  }

  if (z >= 10 && z < 20) {
    z = z + 1;
    k = f(z);
  } else if (f(z) * 10 + k >= g(z, k)) {
    if (!(z < 5)) {}
  }
}
