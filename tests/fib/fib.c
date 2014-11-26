void print_int(int i);
void print_string(char *s);
int atoi(char *s);

int fib(int n) {
  return (n < 2 ? n : fib(n - 1) + fib(n - 2));
}

int main(int argc, char *argv[]) {
  print_int(fib(atoi(argv[1])));
  print_string("\\n");
  return 0;
}
