void print_int(int i);
void print_string(char *s);
int atoi(char *s);

int main(int argc, char *argv[]) {
  int x;
  x = atoi(argv[1]);

  print_string("\\nx + 0 = ");
  print_int(x + 0);

  print_string("\\n0 + x = ");
  print_int(0 + x);

  print_string("\\nx - 0 = ");
  print_int(x - 0);

  print_string("\\nx * 1 = ");
  print_int(x * 1);

  print_string("\\n1 * x = ");
  print_int(1 * x);

  print_string("\\nx / 1 = ");
  print_int(x / 1);

  print_string("\\nx % 1 = ");
  print_int(x % 1);

  print_string("\\nx * 0 = ");
  print_int(x * 0);

  print_string("\\n0 * x = ");
  print_int(0 * x);

  print_string("\\n(x * 0) + x = ");
  print_int((x * 0) + x);

  print_string("\\nx << 0 = ");
  print_int(x << 0);

  print_string("\\n0 << x = ");
  print_int(0 << x);

  print_string("\\nx >> 0 = ");
  print_int(x >> 0);

  print_string("\\n0 >> x = ");
  print_int(0 >> x);

  print_string("\\n");
  return 0;
}